import pytest
import yaml

from ..conftest import (
    XnvmeDriver,
    cijoe_config_get_all_devices,
    get_homi_id,
    xnvme_parametrize,
)


@xnvme_parametrize(labels=["dev"], opts=["be"])
def test_open(cijoe, device, be_opts, cli_args):
    homi_id = get_homi_id()
    flag = "--homi-id" if be_opts["be"].startswith("upcie") else "--shm_id"
    cplane_arg = f"{flag} {homi_id}" if homi_id else ""

    if be_opts["admin"] == "ramdisk":
        pytest.skip(reason=f"[be={be_opts['be']}] does not support enumeration")
    if "fabrics" in device["labels"]:
        err, _ = cijoe.run(
            f"xnvme_tests_enum open --uri {device['uri']} --count 4"
            f" --be {be_opts['be']} {cplane_arg}"
        )
    else:
        err, _ = cijoe.run(
            f"xnvme_tests_enum open --count 4 --be {be_opts['be']} {cplane_arg}"
        )
    assert not err


@xnvme_parametrize(labels=["dev"], opts=["be"])
def test_keep_open(cijoe, device, be_opts, cli_args):
    homi_id = get_homi_id()
    flag = "--homi-id" if be_opts["be"].startswith("upcie") else "--shm_id"
    cplane_arg = f"{flag} {homi_id}" if homi_id else ""

    if be_opts["admin"] == "ramdisk":
        pytest.skip(reason=f"[be={be_opts['be']}] does not support enumeration")

    if "fabrics" in device["labels"]:
        err, _ = cijoe.run(
            f"xnvme_tests_enum keep_open --uri {device['uri']}"
            f" --be {be_opts['be']} {cplane_arg}"
        )
    else:
        err, _ = cijoe.run(
            f"xnvme_tests_enum keep_open --be {be_opts['be']} {cplane_arg}"
        )
    assert not err


@xnvme_parametrize(labels=["dev"], opts=["be"])
def test_multi(cijoe, device, be_opts, cli_args):
    homi_id = get_homi_id()
    flag = "--homi-id" if be_opts["be"].startswith("upcie") else "--shm_id"
    cplane_arg = f"{flag} {homi_id}" if homi_id else ""

    if be_opts["admin"] == "ramdisk":
        pytest.skip(reason=f"[be={be_opts['be']}] does not support enumeration")

    if "fabrics" in device["labels"]:
        err, _ = cijoe.run(
            f"xnvme_tests_enum multi --uri {device['uri']} --count 4"
            f" --be {be_opts['be']} {cplane_arg}"
        )
    else:
        err, _ = cijoe.run(
            f"xnvme_tests_enum multi --count 4 --be {be_opts['be']} {cplane_arg}"
        )
    assert not err


@xnvme_parametrize(labels=["fabrics"], opts=["be"])
def test_fabrics(cijoe, device, be_opts, cli_args):
    """Enumerate a fabrics target and verify at least one namespace is found"""
    err, _ = cijoe.run(
        f"xnvme_tests_enum open --uri {device['uri']} --count 1 --be {be_opts['be']}"
    )
    assert not err


@xnvme_parametrize(labels=["fabrics"], opts=["be"])
def test_fabrics_enum_count(cijoe, device, be_opts, cli_args):
    """Enumerate a fabrics target and verify the device count matches config"""
    err, state = cijoe.run(f"xnvme enum --uri {device['uri']} --be {be_opts['be']}")
    assert not err

    output = yaml.safe_load(state.output())
    entries = output.get("xnvme_cli_enumeration", []) or []

    expected = sum(
        1
        for d in cijoe_config_get_all_devices(["fabrics"])
        if d["uri"] == device["uri"]
    )

    assert len(entries) == expected, (
        f"Enumerated {len(entries)} devices but config has {expected}"
        f" for {device['uri']}"
    )


@xnvme_parametrize(labels=["dev"], opts=[])
def test_backend(cijoe, device, be_opts, cli_args):
    err, _ = cijoe.run("xnvme_tests_enum backend")
    assert not err
