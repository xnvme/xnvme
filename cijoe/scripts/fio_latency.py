#!/usr/bin/env python
"""
    Run fio latency test

    Will either keep blocksize or io depth size variable.

"""
import ast
import logging as log
import os
import shutil
import traceback
from dataclasses import dataclass
from enum import Enum
from itertools import product
from pathlib import Path
from typing import Any, Dict, Iterator, List, Optional

from cijoe.core.command import Cijoe

LATENCY_TEST_SIZE: str = "100%"
LATENCY_TEST_ROOT_DIR: Path = Path("/tmp/")

CijoeEngines = Dict[str, Dict[str, str]]
CijoeDevices = List[Dict[str, str | int | List[str]]]


@dataclass
class Device:
    key: str
    nsid: int
    uri: str
    labels: List[str]
    driver_attachment: str


def determine_devices(devices: CijoeDevices) -> Iterator[Device]:
    for device in devices:
        assert "key" in device and isinstance(device["key"], str)
        assert "nsid" in device and isinstance(device["nsid"], int)
        assert "uri" in device and isinstance(device["uri"], str)
        assert "labels" in device and isinstance(device["labels"], list)
        assert "driver_attachment" in device and isinstance(
            device["driver_attachment"], str
        )
        yield Device(
            key=device["key"],
            nsid=device["nsid"],
            uri=device["uri"],
            labels=device["labels"],
            driver_attachment=device["driver_attachment"],
        )


class Engines(Enum):
    NULL = "null"
    IO_URING = "io_uring"
    IO_URING_CMD = "io_uring_cmd"
    LIBAIO = "libaio"
    POSIXAIO = "posixaio"
    NIL = "nil"
    SPDK_NVME = "spdk_nvme"
    SPDK_BDEV = "spdk_bdev"
    WINDOWSAIO = "windowsaio"
    XNVME_IORING = "xnvme_ioring"
    XNVME_IOCP = "xnvme_iocp"
    XNVME_IOCPTH = "xnvme_iocpth"
    XNVME_IO_URING = "xnvme_io_uring"
    XNVME_IO_URING_CMD = "xnvme_io_uring_cmd"
    XNVME_KQUEUE = "xnvme_kqueue"
    XNVME_LIBAIO = "xnvme_libaio"
    XNVME_POSIXAIO = "xnvme_posixaio"
    XNVME_NULL = "xnvme_null"
    XNVME_SPDK = "xnvme_spdk"


@dataclass
class Engine:
    name: str
    group: str
    device: Device
    cijoe: Cijoe

    @property
    def name_id(self) -> str:
        return f"{self.name}"

    @property
    def ioengine(self) -> str:
        return self.name

    def env(self) -> Dict[str, str]:
        """Environment variables used in execution of test."""
        return {}

    @property
    def filename(self) -> str:
        return self.device.uri

    @property
    def direct(self) -> str:
        """Value for the `--direct` fio flag."""
        return "1"

    def binary(self, bin: str) -> str:
        """Path to fio binary"""
        return bin

    def extra_args(self) -> Dict[str, str]:
        """Method that can be overridden to add extra arguments when running fio.
        e.g. xnvme backend engine when it applies.
        """
        return {}

    def prepare(self) -> None:
        """Method that will run in preparation for execution fio test.
        e.g. is used to setup nvme devices.
        """


class KernelEngine(Engine):
    def prepare(self) -> None:
        self.cijoe.run(" ".join(["xnvme-driver", "reset"]))


class UserSpaceEngine(Engine):
    def prepare(self) -> None:
        self.cijoe.run(" ".join(["xnvme-driver"]))


@dataclass
class XnvmeKernelEngine(KernelEngine):
    name: str
    group: str
    device: Device
    cijoe: Cijoe
    be: str
    async_: str

    @property
    def name_id(self) -> str:
        return f"{self.name}_{self.async_}"

    def extra_args(self) -> Dict[str, str]:
        return {"--xnvme_be": f"{self.be}", "--xnvme_async": f"{self.async_}"}


@dataclass
class XnvmeUserSpaceEngine(UserSpaceEngine):
    name: str
    group: str
    device: Device
    cijoe: Cijoe
    be: str
    async_: str


@dataclass
class ExternalPreloader(UserSpaceEngine):
    name: str
    group: str
    device: Device
    cijoe: Cijoe
    path: Path

    def env(self) -> Dict[str, str]:
        return {"LD_PRELOAD": f"{self.path}"}

    @property
    def filename(self) -> str:
        return f"trtype=PCIe traddr={self.device.uri.replace(':', '.')} ns=1"


@dataclass
class XnvmeNullEngine(XnvmeKernelEngine):
    @property
    def name_id(self) -> str:
        return "xnvme_null"


@dataclass
class XnvmeSPDK(XnvmeUserSpaceEngine):
    def extra_args(self) -> Dict[str, str]:
        return {
            "--xnvme_be": f"{self.be}",
            "--xnvme_async": f"{self.async_}",
            "--xnvme_dev_nsid": str(self.device.nsid),
        }

    @property
    def name_id(self) -> str:
        return "xnvme_spdk"

    @property
    def filename(self) -> str:
        return self.device.uri.replace(":", "\:")  # noqa: W605


@dataclass
class XnvmeIOURingCMD(XnvmeKernelEngine):
    @property
    def direct(self) -> str:
        return "0"


@dataclass
class IOURingCmdEngine(KernelEngine):
    @property
    def direct(self) -> str:
        """IO Uring cmd does not support the direct=1 flag"""
        return "0"


@dataclass
class BdevExternalPreloader(ExternalPreloader):
    def extra_args(self) -> Dict[str, str]:
        spdk_json_conf = (
            self.cijoe.config.options.get("spdk", {})
            .get("build", {})
            .get("spdk_json_conf")
        )
        return {"--spdk_json_conf": f"{spdk_json_conf}"}

    @property
    def filename(self) -> str:
        return (
            self.cijoe.config.options.get("spdk", {}).get("build", {}).get("filename")
        )


def determine_engine(
    engine_identifier: str,
    definition: Dict[str, str],
    devices: List[Device],
    cijoe: Cijoe,
) -> Engine:
    device = next(d for d in devices if d.key == definition["device"])
    name = engine_identifier
    group = definition["group"]
    if "xnvme" in engine_identifier:
        name = "xnvme"
        be = definition["be"]
        async_ = definition["async"]
        engine_args = [name, group, device, cijoe, be, async_]
    elif "spdk" in engine_identifier:
        path = Path(definition["path"])
        if "spdk_nvme" == engine_identifier:
            name = "spdk"
        engine_args = [name, group, device, cijoe, path]
    else:
        engine_args = [name, group, device, cijoe]

    id2engine = {
        Engines.NULL.value: KernelEngine,
        Engines.IO_URING.value: KernelEngine,
        Engines.LIBAIO.value: KernelEngine,
        Engines.POSIXAIO.value: KernelEngine,
        Engines.IO_URING_CMD.value: IOURingCmdEngine,
        Engines.XNVME_IO_URING.value: XnvmeKernelEngine,
        Engines.XNVME_KQUEUE.value: XnvmeKernelEngine,
        Engines.XNVME_LIBAIO.value: XnvmeKernelEngine,
        Engines.XNVME_POSIXAIO.value: XnvmeKernelEngine,
        Engines.XNVME_IO_URING_CMD.value: XnvmeIOURingCMD,
        Engines.XNVME_SPDK.value: XnvmeSPDK,
        Engines.XNVME_NULL.value: XnvmeNullEngine,
        Engines.SPDK_NVME.value: ExternalPreloader,
        Engines.SPDK_BDEV.value: BdevExternalPreloader,
        Engines.XNVME_IORING.value: XnvmeKernelEngine,
        Engines.XNVME_IOCP.value: XnvmeKernelEngine,
        Engines.XNVME_IOCPTH.value: XnvmeKernelEngine,
        Engines.WINDOWSAIO.value: KernelEngine,
    }

    if engine_identifier in id2engine:
        return id2engine[engine_identifier](*engine_args)
    else:
        raise ValueError(
            f"Case match not exhaustive for engine value: {engine_identifier}"
        )


@dataclass
class FIO:
    engine: Engine
    bin: str
    devices: List[Device]
    name: str
    rw: str
    size: str
    bs: str
    output_format: str
    iodepth: str
    time_based: str
    runtime: str
    ramp_time: str
    norandommap: str
    thread: str

    def cmd(self, os_name) -> List[str]:
        args = [
            f"{self.binary}",
            f"--name={self.name}",
            (
                f"--filename={self.engine.filename}"
                if os_name == "windows"
                else f'--filename="{self.engine.filename}"'
            ),
            f"--ioengine={self.engine.name}",
            f"--direct={self.engine.direct}",
            f"--rw={self.rw}",
            f"--size={self.size}",
            f"--bs={self.bs}",
            f"--iodepth={self.iodepth}",
            f"--output-format={self.output_format}",
            f"--time_based={self.time_based}",
            f"--runtime={self.runtime}",
            f"--ramp_time={self.ramp_time}",
            f"--norandommap={self.norandommap}",
            f"--thread={self.thread}",
        ]

        extra_args = self.engine.extra_args().items()
        if os_name == "windows":
            args += [f"{flag}={value}" for flag, value in extra_args]
        else:
            args += [f'{flag}="{value}"' for flag, value in extra_args]

        return args

    def env(self) -> Dict[str, str]:
        return self.engine.env()

    @property
    def binary(self) -> str:
        return self.engine.binary(self.bin)


def main(args, cijoe: Cijoe, step: Dict[str, Any]):
    fio_bin = cijoe.config.options.get("fio", {}).get("bin")
    runs = step.get("with", {}).get("runs", [])
    io_engines: CijoeEngines = cijoe.config.options.get("fio", {}).get("engines")
    devices: List[Device] = list(determine_devices(cijoe.config.options.get("devices")))
    os_name = cijoe.config.options.get("os", {}).get("name", "")

    artifacts = Path(args.output) / "artifacts"
    os.makedirs(str(artifacts), exist_ok=True)

    err = 0
    try:
        for i, run in enumerate(runs):
            iosizes = run["iosizes"]
            iodepths = run["iodepths"]
            combinations = list(product(io_engines.items(), iosizes, iodepths))
            print(
                f"run {i+1}/{len(runs)}:",
                "{",
                f"'iosizes': {iosizes}",
                f"'iodepths': {iodepths}",
                f"'engines': {list(io_engines.keys())}",
                "}",
            )
            print(f"0% completed (0/{len(combinations)})", end="\r")

            for j, (
                (eng, setup),
                iosize,
                iodepth,
            ) in enumerate(combinations):
                engine = determine_engine(eng, setup, devices, cijoe)
                if os_name != "windows":
                    engine.prepare()

                output_path = (
                    artifacts / f"fio-output_IOSIZE={iosize}_IODEPTH={iodepth}_"
                    f"LABEL={engine.name_id}_"
                    f"GROUP={engine.group}.txt"
                )

                # Run fio
                fio = FIO(
                    bin=fio_bin,
                    engine=engine,
                    devices=devices,
                    name=f"{engine.name_id}",
                    rw="randread",
                    size=(
                        LATENCY_TEST_SIZE
                        if engine.group not in ["null", "windowsaio"]
                        else "1G"
                    ),
                    bs=iosize,
                    iodepth=iodepth,
                    output_format="json",
                    time_based="1",
                    runtime="10",
                    ramp_time="5",
                    norandommap="1",
                    thread="1",
                )
                cmd = " ".join(fio.cmd(os_name))

                log.info(f"Executing the command: {cmd}")

                err, state = cijoe.run(cmd, env=fio.env())
                if err:
                    log.error(f"failed: {state}")

                print(
                    f"{(j+1)/len(combinations)*100:.0f}% completed ({j+1}/{len(combinations)})",
                    end="\r",
                )

                # Save the output to a file in artifacts directory
                shutil.copyfile(state.output_fpath, output_path)

    except Exception as exc:
        assert exc.__traceback__

        log.error(f"Error occurred: ({exc})")
        log.error("".join(traceback.format_exception(None, exc, exc.__traceback__)))
        print(
            type(exc).__name__,  # TypeError
            __file__,  # /tmp/example.py
            exc.__traceback__.tb_lineno,  # 2
        )

    return err
