import pytest

from ..conftest import XnvmeDriver, xnvme_parametrize


@xnvme_parametrize(labels=["dev"], opts=["be"])
def test_open(cijoe, device, be_opts, cli_args):
    if be_opts["be"] in ["ramdisk", "vfio"]:
        pytest.skip(reason=f"[be={be_opts['be']}] does not support enumeration")
    if be_opts["be"] == "driverkit":
        pytest.skip(reason="[be=driverkit] does not support repeatedly opening devices")

    if "fabrics" in device["labels"]:
        err, _ = cijoe.run(
            f"xnvme_tests_enum open --uri {device['uri']} --count 4 --be {be_opts['be']}"
        )
    else:
        err, _ = cijoe.run(f"xnvme_tests_enum open --count 4 --be {be_opts['be']}")
    assert not err


@xnvme_parametrize(labels=["dev"], opts=["be"])
def test_keep_open(cijoe, device, be_opts, cli_args):
    if be_opts["be"] in ["ramdisk", "vfio"]:
        pytest.skip(reason=f"[be={be_opts['be']}] does not support enumeration")

    if "fabrics" in device["labels"]:
        err, _ = cijoe.run(
            f"xnvme_tests_enum keep_open --uri {device['uri']} --be {be_opts['be']}"
        )
    else:
        err, _ = cijoe.run(f"xnvme_tests_enum keep_open --be {be_opts['be']}")
    assert not err


@xnvme_parametrize(labels=["dev"], opts=["be"])
def test_multi(cijoe, device, be_opts, cli_args):
    if be_opts["be"] in ["ramdisk", "vfio"]:
        pytest.skip(reason=f"[be={be_opts['be']}] does not support enumeration")
    if be_opts["be"] in ["driverkit", "spdk"]:
        pytest.skip(
            reason=f"[be={be_opts['be']}] does not support repeatedly enumerating"
        )

    if "fabrics" in device["labels"]:
        err, _ = cijoe.run(
            f"xnvme_tests_enum multi --uri {device['uri']} --count 4 --be {be_opts['be']}"
        )
    else:
        err, _ = cijoe.run(f"xnvme_tests_enum multi --count 4 --be {be_opts['be']}")
    assert not err


@xnvme_parametrize(labels=["dev"], opts=[])
def test_backend(cijoe, device, be_opts, cli_args):
    err, _ = cijoe.run("xnvme_tests_enum backend")
    assert not err
