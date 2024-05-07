#!/usr/bin/env python
"""
    Run fio latency test

    Will either keep blocksize or io depth size variable.

"""
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

LATENCY_TEST_SIZE: str = "1G"
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
        return {"-xnvme_be": f"{self.be}", "-xnvme_async": f"{self.async_}"}


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
    fio_bin: Path

    def env(self) -> Dict[str, str]:
        return {"LD_PRELOAD": f"{self.path}"}

    def binary(self, bin: str) -> str:
        return f"{self.fio_bin}"

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
            "-xnvme_be": f"{self.be}",
            "-xnvme_async": f"{self.async_}",
            "-xnvme_dev_nsid": str(self.device.nsid),
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
        return {"-spdk_json_conf": f"{spdk_json_conf}"}

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
    device = list(
        filter(lambda device: device.key == definition["device"], devices)
    ).pop()
    name = engine_identifier
    group = definition["group"]
    if "xnvme" in engine_identifier:
        name = "xnvme"
        be = definition["be"]
        async_ = definition["async"]
        engine_args = [name, group, device, cijoe, be, async_]
    elif "spdk" in engine_identifier:
        path = Path(definition["path"])
        fio_bin = Path(definition["fio_path"])
        if "spdk_nvme" == engine_identifier:
            name = "spdk"
        engine_args = [name, group, device, cijoe, path, fio_bin]
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
    repetitions: str
    numjobs: str
    output_format: str
    io_depth: str
    time_based: str
    runtime: str
    eta_newline: str
    ramp_time: str
    norandommap: str
    thread: str

    def cmd(self) -> List[str]:
        return [
            f"{self.binary}",
            f"--name={self.name}",
            f'--filename="{self.engine.filename}"',
            f"--ioengine={self.engine.name}",
            f"--direct={self.engine.direct}",
            f"--rw={self.rw}",
            f"--size={self.size}",
            f"--bs={self.bs}",
            f"--iodepth={self.io_depth}",
            f"--numjobs={self.repetitions}",
            f"--output-format={self.output_format}",
            f"--time_based={self.time_based}",
            "--group_reporting",
            f"--runtime={self.runtime}",
            f"--eta-newline={self.eta_newline}",
            f"--ramp_time={self.ramp_time}",
            f"--norandommap={self.norandommap}",
            f"--thread={self.thread}",
        ] + [f'-{flag}="{value}"' for flag, value in self.engine.extra_args().items()]

    def env(self) -> Dict[str, str]:
        return self.engine.env()

    @property
    def binary(self) -> str:
        return self.engine.binary(self.bin)


def main(args, cijoe: Cijoe, step: Dict[str, Any]):
    fio_bin = cijoe.config.options.get("fio", {}).get("bin")
    repetitions = step.get("with", {}).get("repetitions", 1)

    io_sizes = step.get("with", {}).get("io_sizes", [])
    io_depths = step.get("with", {}).get("io_depths", [])
    io_engines: CijoeEngines = cijoe.config.options.get("fio", {}).get("engines")
    devices: List[Device] = list(determine_devices(cijoe.config.options.get("devices")))

    artifacts = Path(args.output) / "artifacts"
    os.makedirs(str(artifacts), exist_ok=True)

    err = 0
    try:
        for (
            (eng, setup),
            bs,
            io_depth,
        ) in product(io_engines.items(), io_sizes, io_depths):
            engine = determine_engine(eng, setup, devices, cijoe)
            engine.prepare()

            output_path = (
                artifacts / f"fio-output_BS={bs}_IODEPTH={io_depth}_"
                f"LABEL={engine.name_id}_{repetitions}_"
                f"GROUP={engine.group}.txt"
            )

            # Run fio
            fio = FIO(
                bin=fio_bin,
                engine=engine,
                devices=devices,
                name=f"{engine.name_id}",
                rw="randread",
                size=LATENCY_TEST_SIZE,
                bs=bs,
                repetitions=repetitions,
                io_depth=io_depth,
                numjobs=repetitions,
                output_format="json",
                time_based="1",
                runtime="10",
                eta_newline="1",
                ramp_time="5",
                norandommap="1",
                thread="1",
            )
            cmd = " ".join(fio.cmd())

            log.info(f"Executing the command: {cmd}")

            err, state = cijoe.run(cmd, env=fio.env())
            if err:
                log.error(f"failed: {state}")

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
