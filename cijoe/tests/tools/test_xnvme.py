import pytest
import yaml

from ..conftest import (
    XnvmeDriver,
    get_osname,
    get_shm_id,
    get_userspace_be,
    xnvme_parametrize,
)


def test_env_library(cijoe):
    err, _ = cijoe.run("xnvme env library")
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
def test_cmd_admin(cijoe, device, be_opts, cli_args):
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
        f"xnvme cmd-admin {cli_args} --opcode {opcode} --cdw10 {cdw10} --data-len {data_nbytes} --namespace-id {nsid} --read"
    )
    assert not err


@xnvme_parametrize(labels=["dev"], opts=["be", "admin"])
def test_cmd_io(cijoe, device, be_opts, cli_args):
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
        f"xnvme cmd-io {cli_args} --opcode {opcode} --namespace-id {device['nsid']} --data-len {data_nbytes} --read"
    )
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
