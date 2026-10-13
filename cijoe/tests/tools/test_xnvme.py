import pytest
import yaml

from ..conftest import (
    XnvmeDriver,
    get_osname,
    get_shm_id,
    get_userspace_be,
    is_nvmet,
    xnvme_parametrize,
)


def test_library_info(cijoe):
    err, _ = cijoe.run("xnvme library-info")
    assert not err


def test_enum(cijoe):
    """The enumeration is YAML, a mapping also when it finds nothing"""

    XnvmeDriver.kernel_attach(cijoe)
    err, state = cijoe.run("xnvme enum")
    assert not err
    assert "xnvme_cli_enumeration" in yaml.safe_load(state.output())

    XnvmeDriver.kernel_detach(cijoe)
    err, state = cijoe.run("xnvme enum")
    assert not err
    assert "xnvme_cli_enumeration" in yaml.safe_load(state.output())


@xnvme_parametrize(labels=["fabrics"], opts=["be"])
def test_enum_fabrics(cijoe, device, be_opts, cli_args):
    err, _ = cijoe.run(f"xnvme enum --uri {device['uri']}")
    assert not err


@xnvme_parametrize(labels=["pcie"], opts=["be"])
def test_scan(cijoe, device, be_opts, cli_args):
    """
    Verify that xnvme_scan() discovers devices.

    On Linux/FreeBSD: discovers controllers (dtype: 0x1) and tests both
    kernel-attached and kernel-detached states.
    On Windows/macOS: discovers namespaces (dtype: 0x2) only in attached state.
    """
    from ..conftest import cijoe_config_get_all_devices

    osname = get_osname()
    pcie_devices = cijoe_config_get_all_devices(["pcie"])
    expected_count = len(pcie_devices)

    if osname in ["linux", "freebsd", "debian"]:
        XnvmeDriver.kernel_attach(cijoe)
        err, state = cijoe.run("xnvme scan")
        assert not err

        # Count discovered controllers (dtype: 0x1)
        output = state.output()
        lines = [
            line.strip()
            for line in output.split("\n")
            if line.strip().startswith("- {")
        ]
        scan_count = sum(1 for line in lines if "dtype: 0x1" in line)

        if expected_count > 0:
            assert (
                scan_count == expected_count
            ), f"Expected {expected_count} controllers, found {scan_count}"

        XnvmeDriver.kernel_detach(cijoe)
        err, state = cijoe.run("xnvme scan")
        assert not err

        # Verify controllers are still found when detached
        output = state.output()
        lines = [
            line.strip()
            for line in output.split("\n")
            if line.strip().startswith("- {")
        ]
        scan_count = sum(1 for line in lines if "dtype: 0x1" in line)

        if expected_count > 0:
            assert (
                scan_count == expected_count
            ), f"Expected {expected_count} controllers (detached), found {scan_count}"
    else:
        # Windows/macOS: scan reports namespaces (dtype: 0x2), no detach test
        err, state = cijoe.run("xnvme scan")
        assert not err

        output = state.output()
        lines = [
            line.strip()
            for line in output.split("\n")
            if line.strip().startswith("- {")
        ]
        scan_count = sum(1 for line in lines if "dtype: 0x2" in line)

        if expected_count > 0:
            assert scan_count > 0, "Expected at least one namespace from scan"


def test_scan_iommu_group(cijoe):
    """Each controller xnvme_scan() reports carries the IOMMU group sysfs gives it"""

    if get_osname() != "linux":
        pytest.skip(reason="IOMMU groups are reported on Linux only")

    err, state = cijoe.run("xnvme scan")
    assert not err
    ctrlrs = [
        ident
        for ident in (yaml.safe_load(state.output())["xnvme_scan"] or [])
        if ident["dtype"] == 0x1
    ]
    if not ctrlrs:
        pytest.skip(reason="no NVMe controller on the PCI bus")

    for ident in ctrlrs:
        bdf = ident["uri"]
        err, state = cijoe.run(
            f"basename $(readlink /sys/bus/pci/devices/{bdf}/iommu_group) 2>/dev/null"
            " || true"
        )
        assert not err
        group = (
            state.output().strip().splitlines()[-1] if state.output().strip() else ""
        )
        assert (
            str(ident["iommu_group"]) == group
        ), f"{bdf}: {ident['iommu_group']} != {group}"


def _with_uri_nsid(device, cli_args, nsid):
    """cli_args with the device URI replaced by one naming 'nsid' in its query"""

    return cli_args.replace(device["uri"], f"'{device['uri']}/?nsid={nsid}'", 1)


@xnvme_parametrize(labels=["pcie", "nvm"], opts=["be"])
def test_info_uri_nsid(cijoe, device, be_opts, cli_args):
    """A controller URI names its namespace with '/?nsid=<nsid>'"""

    nsid = int(device["nsid"])
    err, state = cijoe.run(f"xnvme info {_with_uri_nsid(device, cli_args, nsid)}")
    assert not err
    assert f"nsid: {nsid:#x}" in state.output()


@xnvme_parametrize(labels=["pcie", "nvm"], opts=["be"])
def test_info_uri_nsid_inactive(cijoe, device, be_opts, cli_args):
    """A namespace named in the URI that is not active is refused, not replaced"""

    # The configured controllers each have a single namespace
    nsid = int(device["nsid"]) + 1
    err, state = cijoe.run(f"xnvme info {_with_uri_nsid(device, cli_args, nsid)}")
    assert err, f"opened an inactive namespace: {state.output()}"


def test_uri_query_malformed(cijoe):
    """A query naming anything but a namespace is refused"""

    for query in ["?foo=1", "/?nsid=", "/?nsid=0", "/?nsid=1&x=2"]:
        err, _ = cijoe.run(f"xnvme info '1GB{query}' --be ramdisk")
        assert err, f"accepted '1GB{query}'"


@xnvme_parametrize(labels=["dev"], opts=["be", "admin", "sync", "async"])
def test_info(cijoe, device, be_opts, cli_args):
    err, state = cijoe.run(f"xnvme info {cli_args}")
    assert not err

    output = state.output()

    # Parse resolved mixin ids from xnvme_be section
    resolved = {}
    for line in output.split("\n"):
        line = line.strip()
        for key in ["admin", "sync", "async"]:
            if line.startswith(f"{key}: {{id: '"):
                resolved[key] = line.split("'")[1]
        if line.startswith("attr: {name: '"):
            resolved["be"] = line.split("'")[1]

    # Verify each user-specified opt matches the resolved value
    for key in ["be", "admin", "sync", "async"]:
        if key in be_opts:
            assert key in resolved, f"'{key}' not found in xnvme info output"
            assert resolved[key] == be_opts[key], (
                f"Mismatch for '{key}': requested '{be_opts[key]}', "
                f"resolved '{resolved[key]}'"
            )


@xnvme_parametrize(labels=["dev"], opts=["be", "admin"])
def test_idfy(cijoe, device, be_opts, cli_args):
    if be_opts["mem"] == "upcie-cuda":
        pytest.skip(reason="[mem=upcie-cuda] This test does not support CUDA memory")
    err, _ = cijoe.run(
        f"xnvme idfy {cli_args} --cns 0x0 --cntid 0x0 --setid 0x0 --uuid 0x0"
    )
    assert not err


@xnvme_parametrize(labels=["dev"], opts=["be", "admin"])
def test_idfy_ns(cijoe, device, be_opts, cli_args):
    if be_opts["mem"] == "upcie-cuda":
        pytest.skip(reason="[mem=upcie-cuda] This test does not support CUDA memory")
    err, _ = cijoe.run(f"xnvme idfy-ns {cli_args} --nsid {device['nsid']}")
    assert not err


@xnvme_parametrize(labels=["dev"], opts=["be", "admin"])
def test_idfy_ctrlr(cijoe, device, be_opts, cli_args):
    if be_opts["mem"] == "upcie-cuda":
        pytest.skip(reason="[mem=upcie-cuda] This test does not support CUDA memory")
    err, _ = cijoe.run(f"xnvme idfy-ctrlr {cli_args}")
    assert not err


@xnvme_parametrize(labels=["dev", "idfy_cs"], opts=["be", "admin"])
def test_idfy_cs(cijoe, device, be_opts, cli_args):
    if be_opts["admin"] == "block":
        pytest.skip(reason="[admin=block] does not implement idfy-cs")
    if be_opts["admin"] == "ramdisk":
        pytest.skip(reason="[be=ramdisk] does not implement idfy-cs")
    if is_nvmet(cijoe, device):
        pytest.skip(reason="[nvmet] does not implement idfy-cs")

    err, _ = cijoe.run(f"xnvme idfy-cs {cli_args}")
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
    err, state = cijoe.run(f"xnvme idfy-ns {cli_args} --nsid {device['nsid']}")
    assert not err

    lbafl, lbafu = 0, 0
    for line in state.output().split("\n"):
        if "format_lsb:" in line:
            lbafl = int(line.split(":")[1].strip())
        elif "format_msb:" in line:
            lbafu = int(line.split(":")[1].strip())

    err, _ = cijoe.run(f"xnvme format {cli_args} --lbafl {lbafl} --lbafu {lbafu}")
    assert not err


@xnvme_parametrize(labels=["nvm", "sanitize"], opts=["be", "admin"])
def test_sanitize(cijoe, device, be_opts, cli_args):
    if be_opts["admin"] == "block":
        pytest.skip(reason="[admin=block] does not implement sanitize")
    if be_opts["admin"] == "ramdisk":
        pytest.skip(reason="[be=ramdisk] does not implement sanitize")

    err, _ = cijoe.run(f"xnvme sanitize --sanact 0x2 {cli_args}")

    assert not err

    # Get log sanitize status log page
    err, _ = cijoe.run(f"xnvme log-sanitize {cli_args} --nsid {device['nsid']}")

    assert not err


@xnvme_parametrize(labels=["dev"], opts=["be", "admin"])
def test_log_erri(cijoe, device, be_opts, cli_args):
    if be_opts["mem"] == "upcie-cuda":
        pytest.skip(reason="[mem=upcie-cuda] This test does not support CUDA memory")
    if be_opts["admin"] == "block":
        pytest.skip(reason="[admin=block] does not implement error-log")
    if be_opts["admin"] == "ramdisk":
        pytest.skip(reason="[be=ramdisk] does not implement error-log")

    err, _ = cijoe.run(f"xnvme log-erri {cli_args}")
    assert not err


@xnvme_parametrize(labels=["dev"], opts=["be", "admin"])
def test_log_health_controller(cijoe, device, be_opts, cli_args):
    if be_opts["mem"] == "upcie-cuda":
        pytest.skip(reason="[mem=upcie-cuda] This test does not support CUDA memory")
    if be_opts["admin"] == "block":
        pytest.skip(reason="[admin=block] does not implement health-log")
    if be_opts["admin"] == "ramdisk":
        pytest.skip(reason="[be=ramdisk] does not implement health-log")

    err, _ = cijoe.run(f"xnvme log-health {cli_args} --nsid 0xFFFFFFFF")
    assert not err


@xnvme_parametrize(labels=["dev", "log_health_ns"], opts=["be", "admin"])
def test_log_health_namespace(cijoe, device, be_opts, cli_args):
    if be_opts["admin"] == "block":
        pytest.skip(reason="[admin=block] does not implement health-log")
    if be_opts["admin"] == "ramdisk":
        pytest.skip(reason="[be=ramdisk] does not implement health-log")

    err, _ = cijoe.run(f"xnvme log-health {cli_args} --nsid {device['nsid']}")
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
        f"xnvme log {cli_args} --lid {lid} --lsp {lsp} --lpo-nbytes {lpo_nbytes} "
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
        err, _ = cijoe.run(f"xnvme feature-get {cli_args} --fid {fid}")
        assert not err

        # Get fid while setting select-bit
        for sbit in ["0x0", "0x1", "0x2", "0x3"]:
            err, _ = cijoe.run(f"xnvme feature-get {cli_args} --fid {fid} --sel {sbit}")
            assert not err


@xnvme_parametrize(labels=["dev"], opts=["be", "admin"])
def test_feature_set(cijoe, device, be_opts, cli_args):
    if be_opts["admin"] == "ramdisk":
        pytest.skip(reason="[be=ramdisk] does not implement feature-set")
    if is_nvmet(cijoe, device):
        pytest.skip(reason="[nvmet] does not implement feature 0x4")

    err, _ = cijoe.run(f"xnvme feature-set {cli_args} --fid 0x4 --feat 0x1")

    if be_opts["admin"] == "block":
        assert err
    else:
        assert not err


@xnvme_parametrize(labels=["dev"], opts=["be", "admin"])
def test_padc(cijoe, device, be_opts, cli_args):
    """Construct and send an admin command (Get Log Page: Error Information)"""

    if be_opts["admin"] == "block":
        pytest.skip(reason="[admin=block] does not support admin-passthru")
    if be_opts["admin"] == "ramdisk":
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

    # Number of dwords to read (NUMDL), zero-based, in the upper half of cdw10
    numdl = data_nbytes // 4 - 1
    cdw10 = f"{(numdl << 16) | int(lid, 16):#x}"

    err, _ = cijoe.run(
        f"xnvme padc {cli_args} --opcode {opcode} --cdw10 {cdw10} --data-nbytes {data_nbytes} --nsid {nsid}"
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
    if be_opts["mem"] == "upcie-cuda":
        pytest.skip(reason="[mem=upcie-cuda] This test does not support CUDA memory")
    if be_opts["admin"] == "block":
        pytest.skip(reason="[admin=block] does not implement dsm")
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

    if be_opts["admin"] == "libvfn":
        pytest.skip(reason="[be=libvfn] does not support pseudo commands")

    err, _ = cijoe.run(f"xnvme show-regs {cli_args}")

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
        f"xnvme subsystem-reset {device['uri']} --be {be_opts['be']} --admin {be_opts['admin']}"
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
        f"xnvme ctrlr-reset {device['uri']} --be {be_opts['be']} --admin {be_opts['admin']}"
    )

    assert not err


@xnvme_parametrize(labels=["ctrlr"], opts=["be", "admin"])
def test_namespace_rescan(cijoe, device, be_opts, cli_args):
    if be_opts["admin"] == "libvfn":
        pytest.skip(reason="[be=libvfn] does not support pseudo commands")
    if be_opts["admin"] == "spdk":
        pytest.skip(reason="[admin=spdk] does not support namespace rescan")

    err, _ = cijoe.run(f"xnvme ns-rescan {cli_args}")

    assert not err


@xnvme_parametrize(labels=["pcie"], opts=["be"])
def test_homi_id_and_shm_id_must_agree(cijoe, device, be_opts, cli_args):
    """
    Giving both spellings different values is refused; giving both the same is not.

    They name one identifier, so two values means whoever passed them meant one
    runtime and named two. Letting either win would reach a runtime the caller
    did not ask for, which is worse than saying so. The same value twice says
    one thing twice, and is allowed.

    The refusal happens while the options are being built, before any device is
    opened, so this needs no server.
    """

    uri = device["uri"]
    base = (
        f"xnvme info {uri} --dev-nsid {device['nsid']}"
        f" --be {be_opts['be']} --admin {be_opts['be']} --sync {be_opts['be']}"
    )

    err, state = cijoe.run(f"{base} --homi-id 41 --shm_id 42")
    assert err, "two identifiers were accepted; one of them was silently ignored"
    assert (
        "disagree" in state.output()
    ), f"refused, but not for the reason given: {state.output()!r}"

    # What is asserted is that the pair got past the option check, not that the
    # command succeeded: opening the device can fail for reasons of its own,
    # and reading those as "the pair was refused" is how this test lied once.
    _, state = cijoe.run(f"{base} --homi-id 42 --shm_id 42")
    assert (
        "disagree" not in state.output()
    ), f"the same identifier given twice was refused: {state.output()!r}"


def test_frontend_overview(cijoe):
    """Bare xnvme prints the groups rather than every subcommand"""

    err, state = cijoe.run("xnvme")
    assert not err
    assert "Start here:" in state.output()
    assert "xnvme driver" in state.output()
    assert "idfy-ctrlr" not in state.output()


def test_frontend_group(cijoe):
    err, state = cijoe.run("xnvme cmd")
    assert not err
    assert "cmd-admin" in state.output()


def test_frontend_tests_listed_and_run(cijoe):
    """The installed test programs are found on the exec path, and run through xnvme"""

    err, state = cijoe.run("xnvme test")
    assert not err
    assert "xnvme test nvm" in state.output()

    err, _ = cijoe.run("xnvme test nvm enum")
    assert not err


def test_frontend_perf_program(cijoe):
    err, state = cijoe.run("xnvme")
    assert not err
    assert "xnvme perf" in state.output()

    err, _ = cijoe.run("xnvme perf --help")
    assert not err


def test_frontend_env(cijoe):
    """'xnvme env' is one YAML document, the facts of the system then the library"""

    err, state = cijoe.run("xnvme env")
    assert not err
    doc = yaml.safe_load(state.output())
    assert {"system", "ver", "xnvme_libconf", "xnvme_platform"} <= set(doc)

    if get_osname() == "linux":
        system = doc["system"]
        assert system["iommu"]["groups"] >= 0
        assert {"available", "builtin", "loaded"} <= set(system["vfio-pci"])
        assert "default_kib" in system["hugepages"]
        if "nvme_driver" in system:
            assert system["nvme_driver"]["poll_queues"] >= 0
            assert ("fix" in system["nvme_driver"]) == (
                system["nvme_driver"]["poll_queues"] == 0
            )


def test_frontend_driver(cijoe):
    """Bare 'xnvme driver' is 'xnvme list' and then 'xnvme env', as one YAML document"""

    err, state = cijoe.run("xnvme driver")
    assert not err
    doc = yaml.safe_load(state.output())
    assert {"system", "ver", "xnvme_platform"} <= set(doc)
    if get_osname() in ["linux", "freebsd"]:
        assert "devices" in doc


def test_frontend_driver_refuses_backend_of_no_controller(cijoe):
    """A backend that reaches no controller function, or none at all, binds nothing"""

    if get_osname() not in ["linux", "freebsd"]:
        pytest.skip(reason="'xnvme driver --be' is not implemented on this platform")

    err, state = cijoe.run("xnvme driver --be nosuch --all")
    assert err


def frontend_ctrlr(cijoe, bdf):
    """The 'xnvme list' entry of the controller function at 'bdf'"""

    err, state = cijoe.run("xnvme list")
    assert not err

    return (yaml.safe_load(state.output())["devices"] or {}).get(bdf)


def frontend_pcie_bdf(cijoe, linux_only=False):
    from ..conftest import cijoe_config_get_all_devices

    if get_osname() not in (["linux"] if linux_only else ["linux", "freebsd"]):
        pytest.skip(reason="'xnvme driver --be' is not implemented on this platform")
    for device in cijoe_config_get_all_devices(["pcie"]):
        if device.get("driver_attachment", "kernel") != "none":
            return device["uri"]
    pytest.skip(reason="no PCIe device in the configuration")


def test_frontend_driver_be(cijoe):
    """A controller moves to the driver of a userspace backend, and back to the kernel's"""

    bdf = frontend_pcie_bdf(cijoe)
    be = get_userspace_be()
    XnvmeDriver.kernel_attach(cijoe)
    assert frontend_ctrlr(cijoe, bdf)["driver"] == "nvme"

    err, state = cijoe.run(f"xnvme driver --be {be} {bdf} --force")
    assert not err
    XnvmeDriver.IS_KERNEL_ATTACHED = None
    ctrlr = frontend_ctrlr(cijoe, bdf)
    assert ctrlr["driver"] in ["vfio-pci", "uio_pci_generic", "nic_uio"]
    assert yaml.safe_load(state.output())["devices"] == {
        bdf: {"driver": ctrlr["driver"], "changed": True}
    }

    err, state = cijoe.run(f"xnvme driver --be {be} {bdf}")
    assert not err
    assert yaml.safe_load(state.output())["devices"] == {
        bdf: {"driver": ctrlr["driver"], "changed": False}
    }

    err, _ = cijoe.run(f"xnvme driver --be posix {bdf}")
    assert not err
    assert frontend_ctrlr(cijoe, bdf)["driver"] == "nvme"
    XnvmeDriver.IS_KERNEL_ATTACHED = True


def test_frontend_driver_be_all(cijoe):
    """--all moves every controller, and reports each of them"""

    frontend_pcie_bdf(cijoe)
    be = get_userspace_be()
    XnvmeDriver.kernel_attach(cijoe)

    err, state = cijoe.run("xnvme list")
    assert not err
    bdfs = set(yaml.safe_load(state.output())["devices"] or {})

    err, state = cijoe.run(f"xnvme driver --be {be} --all --force")
    assert not err
    XnvmeDriver.IS_KERNEL_ATTACHED = None
    assert set(yaml.safe_load(state.output())["devices"]) == bdfs

    err, state = cijoe.run("xnvme driver --be posix --all")
    assert not err
    assert set(yaml.safe_load(state.output())["devices"]) == bdfs
    for bdf in bdfs:
        assert frontend_ctrlr(cijoe, bdf)["driver"] == "nvme"
    XnvmeDriver.IS_KERNEL_ATTACHED = True


def test_frontend_driver_refuses_open_namespace(cijoe):
    """A namespace held open by a process keeps its controller on the kernel driver"""

    bdf = frontend_pcie_bdf(cijoe, linux_only=True)
    XnvmeDriver.kernel_attach(cijoe)

    err, state = cijoe.run(
        f"ls /sys/bus/pci/devices/{bdf}/nvme/*/ | grep -m1 -E '^nvme[0-9]+n[0-9]+$'"
    )
    if err or not state.output().strip():
        pytest.skip(reason="the controller has no namespace block device")
    ns = state.output().strip().splitlines()[-1]

    err, _ = cijoe.run(
        f"sleep 60 < /dev/{ns} > /dev/null 2>&1 & echo $! > /tmp/xnvme-hold.pid"
    )
    assert not err
    try:
        err, state = cijoe.run(f"xnvme driver --be upcie {bdf}")
        assert err
        assert "open by sleep (pid" in state.output()
        assert frontend_ctrlr(cijoe, bdf)["driver"] == "nvme"
    finally:
        cijoe.run("kill $(cat /tmp/xnvme-hold.pid); rm -f /tmp/xnvme-hold.pid")


IDFY_CTRLR = "--opcode=0x06 --cdw10=1 --data-len=4096 --read --raw-binary"


def test_frontend_cmd_admin_matches_nvme_cli(cijoe):
    """The same command line gives the same answer from nvme-cli and from xnvme, on the
    kernel path and, after an attach, on the userspace one"""

    bdf = frontend_pcie_bdf(cijoe)
    XnvmeDriver.kernel_attach(cijoe)
    err, _ = cijoe.run("command -v nvme")
    if err:
        pytest.skip(reason="nvme-cli is not installed")

    err, state = cijoe.run(f"ls /sys/bus/pci/devices/{bdf}/nvme/")
    assert not err
    ctrl = state.output().split()[-1]
    err, state = cijoe.run(
        f"ls /sys/bus/pci/devices/{bdf}/nvme/{ctrl}/ | grep -m1 -E '^nvme[0-9]+n[0-9]+$'"
    )
    if err:
        pytest.skip(reason="the controller has no namespace block device")
    ns = state.output().split()[-1]

    err, state = cijoe.run(
        f"nvme admin-passthru /dev/{ctrl} {IDFY_CTRLR} 2>/dev/null | sha256sum"
    )
    assert not err
    expected = state.output().split()[0]
    err, state = cijoe.run(
        f"xnvme cmd-admin /dev/{ns} {IDFY_CTRLR} 2>/dev/null | sha256sum"
    )
    assert not err
    assert state.output().split()[0] == expected

    err, _ = cijoe.run(f"xnvme driver --be upcie {bdf} --force")
    assert not err
    XnvmeDriver.IS_KERNEL_ATTACHED = None
    try:
        err, state = cijoe.run(
            f"xnvme cmd-admin {bdf} {IDFY_CTRLR} --be upcie 2>/dev/null | sha256sum"
        )
        assert not err
        assert state.output().split()[0] == expected
    finally:
        err, _ = cijoe.run(f"xnvme driver --be posix {bdf}")
    assert not err
    XnvmeDriver.IS_KERNEL_ATTACHED = True
