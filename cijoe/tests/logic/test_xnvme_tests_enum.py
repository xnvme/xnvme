import pytest

from ..conftest import XnvmeDriver, xnvme_parametrize


@xnvme_parametrize(labels=["dev"], opts=["be"])
def test_open(cijoe, device, be_opts, cli_args):
    if be_opts["be"] == "ramdisk":
        pytest.skip(reason="[be=ramdisk] does not support enumeration")
    if be_opts["be"] == "driverkit":
        pytest.skip(reason="[be=ramdisk] does not support repeatedly enumerating")

    if "fabrics" in device["labels"]:
        err, _ = cijoe.run(
            f"xnvme_tests_enum open --uri {device['uri']} --count 4 --be {be_opts['be']}"
        )
    else:
        err, _ = cijoe.run(f"xnvme_tests_enum open --count 4 --be {be_opts['be']}")
    assert not err


@xnvme_parametrize(labels=["dev"], opts=["be"])
def test_multi(cijoe, device, be_opts, cli_args):
    if be_opts["be"] == "spdk":
        pytest.skip(reason="[be=spdk] does not support repeatedly enumerating")
    if be_opts["be"] == "ramdisk":
        pytest.skip(reason="[be=ramdisk] does not support enumeration")
    if be_opts["be"] == "driverkit":
        pytest.skip(reason="[be=ramdisk] does not support repeatedly enumerating")

    if "fabrics" in device["labels"]:
        err, _ = cijoe.run(
            f"xnvme_tests_enum multi --uri {device['uri']} --count 4 --be {be_opts['be']}"
        )
    else:
        err, _ = cijoe.run(f"xnvme_tests_enum multi --count 4 --be {be_opts['be']}")
    assert not err


@xnvme_parametrize(labels=["dev"], opts=["be"])
def test_backend(cijoe, device, be_opts, cli_args):
    err, _ = cijoe.run("xnvme_tests_enum backend")
    assert not err
