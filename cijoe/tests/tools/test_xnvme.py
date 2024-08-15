import pytest

from ..conftest import XnvmeDriver, xnvme_parametrize


def test_library_info(cijoe):
    err, _ = cijoe.run("xnvme library-info")
    assert not err


def test_enum(cijoe):
    XnvmeDriver.kernel_attach(cijoe)
    err, _ = cijoe.run("xnvme enum")
    assert not err

    XnvmeDriver.kernel_detach(cijoe)
    err, _ = cijoe.run("xnvme enum")
    assert not err


@xnvme_parametrize(labels=["fabrics"], opts=["be"])
def test_enum_fabrics(cijoe, device, be_opts, cli_args):
    err, _ = cijoe.run(f"xnvme enum --uri {device['uri']}")
    assert not err


@xnvme_parametrize(labels=["dev"], opts=["be", "admin"])
def test_info(cijoe, device, be_opts, cli_args):
    err, _ = cijoe.run(f"xnvme info {cli_args}")
    assert not err


@xnvme_parametrize(labels=["dev"], opts=["be", "admin"])
def test_idfy(cijoe, device, be_opts, cli_args):
    err, _ = cijoe.run(
        f"xnvme idfy {cli_args} --cns 0x0 --cntid 0x0 --setid 0x0 --uuid 0x0"
    )
    assert not err


@xnvme_parametrize(labels=["dev"], opts=["be", "admin"])
def test_idfy_ns(cijoe, device, be_opts, cli_args):
    err, _ = cijoe.run(f"xnvme idfy-ns {cli_args} --nsid {device['nsid']}")
    assert not err


@xnvme_parametrize(labels=["dev"], opts=["be", "admin"])
def test_idfy_ctrlr(cijoe, device, be_opts, cli_args):
    err, _ = cijoe.run(f"xnvme idfy-ctrlr {cli_args}")
    assert not err


@xnvme_parametrize(labels=["dev", "idfy_cs"], opts=["be", "admin"])
def test_idfy_cs(cijoe, device, be_opts, cli_args):
    if be_opts["be"] == "linux" and be_opts["admin"] in ["block"]:
        pytest.skip(reason="[admin=block] does not implement idfy-cs")
    if "ramdisk" in device["labels"]:
        pytest.skip(reason="[be=ramdisk] does not implement idfy-cs")

    err, _ = cijoe.run(f"xnvme idfy-cs {cli_args}")
    assert not err


@xnvme_parametrize(labels=["nvm"], opts=["be", "admin"])
def test_format(cijoe, device, be_opts, cli_args):
    if be_opts["be"] == "linux" and be_opts["admin"] in ["block"]:
        pytest.skip(reason="[admin=block] does not implement format")
    elif be_opts["be"] == "spdk" and "fabrics" in device["labels"]:
        pytest.skip(reason="spdk fabrics does not support format")

    if "ramdisk" in device["labels"]:
        pytest.skip(reason="[be=ramdisk] does not implement format")

    err, _ = cijoe.run(f"xnvme format {cli_args}")
    assert not err


@xnvme_parametrize(labels=["nvm", "sanitize"], opts=["be", "admin"])
def test_sanitize(cijoe, device, be_opts, cli_args):
    if be_opts["be"] == "linux" and be_opts["admin"] in ["block"]:
        pytest.skip(reason="[admin=block] does not implement sanitize")
    if "ramdisk" in device["labels"]:
        pytest.skip(reason="[be=ramdisk] does not implement sanitize")

    err, _ = cijoe.run(f"xnvme sanitize --sanact 0x2 {cli_args}")

    assert not err

    # Get log sanitize status log page
    err, _ = cijoe.run(f"xnvme log-sanitize {cli_args} --nsid {device['nsid']}")

    assert not err


@xnvme_parametrize(labels=["dev"], opts=["be", "admin"])
def test_log_erri(cijoe, device, be_opts, cli_args):
    if be_opts["be"] == "linux" and be_opts["admin"] in ["block"]:
        pytest.skip(reason="[admin=block] does not implement error-log")
    if "ramdisk" in device["labels"]:
        pytest.skip(reason="[be=ramdisk] does not implement error-log")

    err, _ = cijoe.run(f"xnvme log-erri {cli_args}")
    assert not err


@xnvme_parametrize(labels=["dev"], opts=["be", "admin"])
def test_log_health_controller(cijoe, device, be_opts, cli_args):
    if be_opts["be"] == "linux" and be_opts["admin"] in ["block"]:
        pytest.skip(reason="[admin=block] does not implement health-log")
    if "ramdisk" in device["labels"]:
        pytest.skip(reason="[be=ramdisk] does not implement health-log")

    err, _ = cijoe.run(f"xnvme log-health {cli_args} --nsid 0xFFFFFFFF")
    assert not err


@xnvme_parametrize(labels=["dev", "log_health_ns"], opts=["be", "admin"])
def test_log_health_namespace(cijoe, device, be_opts, cli_args):
    if be_opts["be"] == "linux" and be_opts["admin"] in ["block"]:
        pytest.skip(reason="[admin=block] does not implement health-log")
    if "ramdisk" in device["labels"]:
        pytest.skip(reason="[be=ramdisk] does not implement health-log")

    err, _ = cijoe.run(f"xnvme log-health {cli_args} --nsid {device['nsid']}")
    assert not err


@xnvme_parametrize(labels=["dev"], opts=["be", "admin"])
def test_log(cijoe, device, be_opts, cli_args):
    if be_opts["be"] == "linux" and be_opts["admin"] in ["block"]:
        pytest.skip(reason="[admin=block] does not implement get-log")
    if "ramdisk" in device["labels"]:
        pytest.skip(reason="[be=ramdisk] does not implement get-log")

    lid, lsp, lpo_nbytes, rae, nbytes, nsid = "0x1", "0x0", 0, 0, 4096, "0xFFFFFFFF"

    err, _ = cijoe.run(
        f"xnvme log {cli_args} --lid {lid} --lsp {lsp} --lpo-nbytes {lpo_nbytes} "
        f"--rae {rae} --data-nbytes {nbytes} --nsid {nsid}"
    )
    assert not err


@xnvme_parametrize(labels=["dev"], opts=["be", "admin"])
def test_feature_get(cijoe, device, be_opts, cli_args):
    if be_opts["be"] == "linux" and be_opts["admin"] in ["block"]:
        pytest.skip(reason="[admin=block] does not implement feature-get")
    if "ramdisk" in device["labels"]:
        pytest.skip(reason="[be=ramdisk] does not implement feature-get")

    for fid, descr in [("0x4", "Temperature threshold"), ("0x5", "Error recovery")]:
        # Get fid without setting select-bit
        err, _ = cijoe.run(f"xnvme feature-get {cli_args} --fid {fid}")
        assert not err

        # Get fid while setting select-bit
        for sbit in ["0x0", "0x1", "0x2", "0x3"]:
            err, _ = cijoe.run(f"xnvme feature-get {cli_args} --fid {fid} --sel {sbit}")
            assert not err


@xnvme_parametrize(labels=["dev"], opts=["be", "admin"])
def test_feature_set(cijoe, device, be_opts, cli_args):
    if "ramdisk" in device["labels"]:
        pytest.skip(reason="[be=ramdisk] does not implement feature-set")

    err, _ = cijoe.run(f"xnvme feature-set {cli_args} --fid 0x4 --feat 0x1")

    if be_opts["admin"] == "block":
        assert err
    else:
        assert not err


@xnvme_parametrize(labels=["dev"], opts=["be", "admin"])
def test_padc(cijoe, device, be_opts, cli_args):
    """Construct and send an admin command (Get Log Page: Error Information)"""

    if be_opts["be"] == "linux" and be_opts["admin"] in ["block"]:
        pytest.skip(reason="[admin=block] does not support admin-passthru")
    if "ramdisk" in device["labels"]:
        pytest.skip(reason="[be=ramdisk] does not implement this admin-opcode")

    # Get Log Page
    opcode = "0x02"
    # Error Information
    lid = "0x1"
    # Controller
    nsid = "0xFFFFFFFF"

    err, state = cijoe.run(f"xnvme info {cli_args}")
    assert not err

    data_nbytes = 0
    for line in state.output().split("\n"):
        if "lba_nbytes" not in line:
            continue
        key, val = line.split(":")
        data_nbytes = int(val.strip())
        break

    assert data_nbytes > 0

    err, _ = cijoe.run(
        f"xnvme padc {cli_args} --opcode {opcode} --cdw10 {lid} --data-nbytes {data_nbytes} --nsid {nsid}"
    )
    assert not err


@xnvme_parametrize(labels=["dev"], opts=["be", "admin"])
def test_pioc(cijoe, device, be_opts, cli_args):
    """Construct and send an I/O command (read)"""

    opcode = "0x02"

    err, state = cijoe.run(f"xnvme info {cli_args}")
    assert not err

    data_nbytes = 0
    for line in state.output().split("\n"):
        if "lba_nbytes" not in line:
            continue
        key, val = line.split(":")
        data_nbytes = int(val.strip())
        break

    assert data_nbytes > 0

    err, _ = cijoe.run(
        f"xnvme pioc {cli_args} --opcode {opcode} --nsid {device['nsid']} --data-nbytes {data_nbytes}"
    )
    assert not err


@xnvme_parametrize(labels=["dev"], opts=["be", "admin"])
def test_dsm(cijoe, device, be_opts, cli_args):
    err, _ = cijoe.run(
        f"xnvme dsm {cli_args} --nsid {device['nsid']} --ad --slba 0 --llb 1"
    )
    assert not err

    err, _ = cijoe.run(
        f"xnvme dsm {cli_args} --nsid {device['nsid']} --idw --slba 0 --llb 1"
    )
    assert not err

    err, _ = cijoe.run(
        f"xnvme dsm {cli_args} --nsid {device['nsid']} --idr --slba 0 --llb 1"
    )
    assert not err

    err, _ = cijoe.run(
        f"xnvme dsm {cli_args} --nsid {device['nsid']} --ad --idw --idr --slba 0 --llb 1"
    )
    assert not err


# For these four tests we request a device with labels 'ctrlr'
@xnvme_parametrize(labels=["ctrlr"], opts=["be", "admin"])
def test_show_regs(cijoe, device, be_opts, cli_args):
    """
    The 'xnvme show-regs' is expected to fail when the kernel config has
    CONFIG_IO_STRICT_DEVMEM=y. When this test is running, we do not know what
    the state of this kernel config-option is, thus we check for it, and adjust
    the test-expectation accordingly.
    """

    if be_opts["be"] == "vfio":
        pytest.skip(reason="[be=vfio] does not support pseudo commands")

    err, _ = cijoe.run(
        f"xnvme show-regs {device['uri']} --be {be_opts['be']} --admin {be_opts['admin']}"
    )

    expect_error = False
    if be_opts["be"] in ["linux"] and be_opts["admin"] in ["nvme"]:
        _, state = cijoe.run(
            "cat /boot/config-$(uname -r) | grep CONFIG_IO_STRICT_DEVMEM"
        )
        expect_error = "CONFIG_IO_STRICT_DEVMEM=y" in state.output()

    if expect_error:
        assert err, "Expected error, however, it did not"
    else:
        assert not err, "Expected it to work, however, it did not"


@xnvme_parametrize(labels=["ctrlr"], opts=["be", "admin"])
def test_subsystem_reset(cijoe, device, be_opts, cli_args):
    if be_opts["be"] == "vfio":
        pytest.skip(reason="[be=vfio] does not support pseudo commands")

    err, _ = cijoe.run(
        f"xnvme subsystem-reset {device['uri']} --be {be_opts['be']} --admin {be_opts['admin']}"
    )

    assert not err


@xnvme_parametrize(labels=["ctrlr"], opts=["be", "admin"])
def test_ctrlr_reset(cijoe, device, be_opts, cli_args):
    if be_opts["be"] == "vfio":
        pytest.skip(reason="[be=vfio] does not support pseudo commands")

    err, _ = cijoe.run(
        f"xnvme ctrlr-reset {device['uri']} --be {be_opts['be']} --admin {be_opts['admin']}"
    )

    assert not err


@xnvme_parametrize(labels=["ctrlr"], opts=["be", "admin"])
def test_namespace_rescan(cijoe, device, be_opts, cli_args):
    if be_opts["be"] == "vfio":
        pytest.skip(reason="[be=vfio] does not support pseudo commands")
    if be_opts["be"] == "spdk":
        pytest.skip(reason="[be=spdk] does not support namespace rescan")

    err, _ = cijoe.run(
        f"xnvme ns-rescan {device['uri']} --be {be_opts['be']} --admin {be_opts['admin']}"
    )

    assert not err
