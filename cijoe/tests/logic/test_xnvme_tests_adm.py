import pytest

from ..conftest import XnvmeDriver, get_osname, get_shm_id, is_nvmet, xnvme_parametrize


@xnvme_parametrize(labels=["dev"], opts=["be", "admin"])
def test_idfy(cijoe, device, be_opts, cli_args):
    if be_opts["mem"] == "upcie-cuda":
        pytest.skip(reason="[mem=upcie-cuda] This test does not support CUDA memory")
    err, _ = cijoe.run(
        f"xnvme test adm idfy {cli_args} --cns 0x0 --cntid 0x0 --setid 0x0 --uuid 0x0"
    )
    assert not err


@xnvme_parametrize(labels=["dev"], opts=["be", "admin"])
def test_idfy_ns(cijoe, device, be_opts, cli_args):
    if be_opts["mem"] == "upcie-cuda":
        pytest.skip(reason="[mem=upcie-cuda] This test does not support CUDA memory")
    err, _ = cijoe.run(f"xnvme test adm idfy-ns {cli_args} --nsid {device['nsid']}")
    assert not err


@xnvme_parametrize(labels=["dev"], opts=["be", "admin"])
def test_idfy_ctrlr(cijoe, device, be_opts, cli_args):
    if be_opts["mem"] == "upcie-cuda":
        pytest.skip(reason="[mem=upcie-cuda] This test does not support CUDA memory")
    err, _ = cijoe.run(f"xnvme test adm idfy-ctrlr {cli_args}")
    assert not err


@xnvme_parametrize(labels=["dev", "idfy_cs"], opts=["be", "admin"])
def test_idfy_cs(cijoe, device, be_opts, cli_args):
    if be_opts["admin"] == "block":
        pytest.skip(reason="[admin=block] does not implement idfy-cs")
    if be_opts["admin"] == "ramdisk":
        pytest.skip(reason="[be=ramdisk] does not implement idfy-cs")
    if is_nvmet(cijoe, device):
        pytest.skip(reason="[nvmet] does not implement idfy-cs")

    err, _ = cijoe.run(f"xnvme test adm idfy-cs {cli_args}")
    assert not err


@xnvme_parametrize(labels=["nvm"], opts=["be", "admin"])
def test_format(cijoe, device, be_opts, cli_args):
    if be_opts["admin"] == "block":
        pytest.skip(reason="[admin=block] does not implement format")
    elif be_opts["admin"] == "spdk" and "fabrics" in device["labels"]:
        pytest.skip(reason="spdk fabrics does not support format")

    if be_opts["admin"] == "ramdisk":
        pytest.skip(reason="[be=ramdisk] does not implement format")

    # Reformat with the current LBA format to avoid changing the block size.
    err, state = cijoe.run(f"xnvme test adm idfy-ns {cli_args} --nsid {device['nsid']}")
    assert not err

    lbafl, lbafu = 0, 0
    for line in state.output().split("\n"):
        if "format_lsb:" in line:
            lbafl = int(line.split(":")[1].strip())
        elif "format_msb:" in line:
            lbafu = int(line.split(":")[1].strip())

    err, _ = cijoe.run(
        f"xnvme test adm format {cli_args} --lbafl {lbafl} --lbafu {lbafu}"
    )
    assert not err


@xnvme_parametrize(labels=["nvm", "sanitize"], opts=["be", "admin"])
def test_sanitize(cijoe, device, be_opts, cli_args):
    if be_opts["admin"] == "block":
        pytest.skip(reason="[admin=block] does not implement sanitize")
    if be_opts["admin"] == "ramdisk":
        pytest.skip(reason="[be=ramdisk] does not implement sanitize")

    err, _ = cijoe.run(f"xnvme test adm sanitize --sanact 0x2 {cli_args}")

    assert not err

    # Get log sanitize status log page
    err, _ = cijoe.run(
        f"xnvme test adm log-sanitize {cli_args} --nsid {device['nsid']}"
    )

    assert not err


@xnvme_parametrize(labels=["dev"], opts=["be", "admin"])
def test_log_erri(cijoe, device, be_opts, cli_args):
    if be_opts["mem"] == "upcie-cuda":
        pytest.skip(reason="[mem=upcie-cuda] This test does not support CUDA memory")
    if be_opts["admin"] == "block":
        pytest.skip(reason="[admin=block] does not implement error-log")
    if be_opts["admin"] == "ramdisk":
        pytest.skip(reason="[be=ramdisk] does not implement error-log")

    err, _ = cijoe.run(f"xnvme test adm log-erri {cli_args}")
    assert not err


@xnvme_parametrize(labels=["dev"], opts=["be", "admin"])
def test_log_health_controller(cijoe, device, be_opts, cli_args):
    if be_opts["mem"] == "upcie-cuda":
        pytest.skip(reason="[mem=upcie-cuda] This test does not support CUDA memory")
    if be_opts["admin"] == "block":
        pytest.skip(reason="[admin=block] does not implement health-log")
    if be_opts["admin"] == "ramdisk":
        pytest.skip(reason="[be=ramdisk] does not implement health-log")

    err, _ = cijoe.run(f"xnvme test adm log-health {cli_args} --nsid 0xFFFFFFFF")
    assert not err


@xnvme_parametrize(labels=["dev", "log_health_ns"], opts=["be", "admin"])
def test_log_health_namespace(cijoe, device, be_opts, cli_args):
    if be_opts["admin"] == "block":
        pytest.skip(reason="[admin=block] does not implement health-log")
    if be_opts["admin"] == "ramdisk":
        pytest.skip(reason="[be=ramdisk] does not implement health-log")

    err, _ = cijoe.run(f"xnvme test adm log-health {cli_args} --nsid {device['nsid']}")
    assert not err


@xnvme_parametrize(labels=["dev"], opts=["be", "admin"])
def test_log(cijoe, device, be_opts, cli_args):
    if be_opts["mem"] == "upcie-cuda":
        pytest.skip(reason="[mem=upcie-cuda] This test does not support CUDA memory")
    if be_opts["admin"] == "block":
        pytest.skip(reason="[admin=block] does not implement get-log")
    if be_opts["admin"] == "ramdisk":
        pytest.skip(reason="[be=ramdisk] does not implement get-log")

    lid, lsp, lpo_nbytes, rae, nbytes, nsid = "0x1", "0x0", 0, 0, 4096, "0xFFFFFFFF"

    err, _ = cijoe.run(
        f"xnvme test adm log {cli_args} --lid {lid} --lsp {lsp} --lpo-nbytes {lpo_nbytes} "
        f"--rae {rae} --data-nbytes {nbytes} --nsid {nsid}"
    )
    assert not err


@xnvme_parametrize(labels=["dev"], opts=["be", "admin"])
def test_feature_get(cijoe, device, be_opts, cli_args):
    if be_opts["admin"] == "block":
        pytest.skip(reason="[admin=block] does not implement feature-get")
    if be_opts["admin"] == "ramdisk":
        pytest.skip(reason="[be=ramdisk] does not implement feature-get")

    for fid, descr in [("0x4", "Temperature threshold"), ("0x5", "Error recovery")]:
        # Get fid without setting select-bit
        err, _ = cijoe.run(f"xnvme test adm feature-get {cli_args} --fid {fid}")
        assert not err

        # Get fid while setting select-bit
        for sbit in ["0x0", "0x1", "0x2", "0x3"]:
            err, _ = cijoe.run(
                f"xnvme test adm feature-get {cli_args} --fid {fid} --sel {sbit}"
            )
            assert not err


@xnvme_parametrize(labels=["dev"], opts=["be", "admin"])
def test_feature_set(cijoe, device, be_opts, cli_args):
    if be_opts["admin"] == "ramdisk":
        pytest.skip(reason="[be=ramdisk] does not implement feature-set")
    if is_nvmet(cijoe, device):
        pytest.skip(reason="[nvmet] does not implement feature 0x4")

    err, _ = cijoe.run(f"xnvme test adm feature-set {cli_args} --fid 0x4 --feat 0x1")

    if be_opts["admin"] == "block":
        assert err
    else:
        assert not err


@xnvme_parametrize(labels=["dev"], opts=["be", "admin"])
def test_dsm(cijoe, device, be_opts, cli_args):
    if be_opts["mem"] == "upcie-cuda":
        pytest.skip(reason="[mem=upcie-cuda] This test does not support CUDA memory")
    if be_opts["admin"] == "block":
        pytest.skip(reason="[admin=block] does not implement dsm")
    err, _ = cijoe.run(
        f"xnvme test adm dsm {cli_args} --nsid {device['nsid']} --ad --slba 0 --llb 1"
    )
    assert not err

    err, _ = cijoe.run(
        f"xnvme test adm dsm {cli_args} --nsid {device['nsid']} --idw --slba 0 --llb 1"
    )
    assert not err

    err, _ = cijoe.run(
        f"xnvme test adm dsm {cli_args} --nsid {device['nsid']} --idr --slba 0 --llb 1"
    )
    assert not err

    err, _ = cijoe.run(
        f"xnvme test adm dsm {cli_args} --nsid {device['nsid']} --ad --idw --idr --slba 0 --llb 1"
    )
    assert not err


# For these four tests we request a device with labels 'ctrlr'
@xnvme_parametrize(labels=["ctrlr"], opts=["be", "admin"])
def test_show_regs(cijoe, device, be_opts, cli_args):
    """
    The 'xnvme test adm show-regs' is expected to fail when the kernel config has
    CONFIG_IO_STRICT_DEVMEM=y. When this test is running, we do not know what
    the state of this kernel config-option is, thus we check for it, and adjust
    the test-expectation accordingly.
    """

    if be_opts["admin"] == "libvfn":
        pytest.skip(reason="[be=libvfn] does not support pseudo commands")

    err, _ = cijoe.run(f"xnvme test adm show-regs {cli_args}")

    expect_error = False
    if get_osname() == "linux" and be_opts["admin"] == "nvme":
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
    if be_opts["admin"] == "libvfn":
        pytest.skip(reason="[be=libvfn] does not support pseudo commands")
    if be_opts["admin"] == "upcie":
        pytest.skip(reason="[be=upcie] does not support subsystem-reset")
    if get_shm_id():
        pytest.skip(reason="reset tears down the queues held by the mproc primary")

    err, state = cijoe.run(
        f"xnvme test adm subsystem-reset {device['uri']} --be {be_opts['be']} --admin {be_opts['admin']}"
    )

    # Skip when subsystem-reset isn't available (upstream QEMU CAP.NSSRS=0;
    # kernel rejects ioctl on the read-only fd). Match on message text since
    # the underlying errno differs between Linux and FreeBSD.
    out = state.output()
    if err and (
        "'Operation not supported'" in out or "'Operation not permitted'" in out
    ):
        pytest.skip(
            reason="controller subsystem reset unavailable (CAP.NSSRS=0 or kernel ioctl-rw guard)"
        )

    assert not err

    # Full driver unbind/rebind cycle to recover from subsystem reset
    XnvmeDriver.kernel_detach(cijoe)
    XnvmeDriver.kernel_attach(cijoe)


@xnvme_parametrize(labels=["ctrlr"], opts=["be", "admin"])
def test_ctrlr_reset(cijoe, device, be_opts, cli_args):
    if be_opts["admin"] == "libvfn":
        pytest.skip(reason="[be=libvfn] does not support pseudo commands")
    if be_opts["admin"] == "upcie":
        pytest.skip(reason="[be=upcie] does not support controller-reset")
    if get_shm_id():
        pytest.skip(reason="reset tears down the queues held by the mproc primary")

    err, _ = cijoe.run(
        f"xnvme test adm ctrlr-reset {device['uri']} --be {be_opts['be']} --admin {be_opts['admin']}"
    )

    assert not err


@xnvme_parametrize(labels=["ctrlr"], opts=["be", "admin"])
def test_namespace_rescan(cijoe, device, be_opts, cli_args):
    if be_opts["admin"] == "libvfn":
        pytest.skip(reason="[be=libvfn] does not support pseudo commands")
    if be_opts["admin"] == "spdk":
        pytest.skip(reason="[admin=spdk] does not support namespace rescan")

    err, _ = cijoe.run(f"xnvme test adm ns-rescan {cli_args}")

    assert not err
