import pytest

from ..conftest import get_homi_id, xnvme_parametrize

# The memory interfaces that register host memory of the caller's
REGISTERS_HOST_MEMORY = (
    "hugepage",
    "io_uring_dmabuf",
    "libvfn",
    "posix",
    "spdk",
    "windows",
)


def has_iommu_group(cijoe, device):
    # The driver does not tell, as vfio-pci in no-IOMMU mode is bound like
    # vfio-pci with one
    uri = device["uri"]
    err, _ = cijoe.run(f"test -e /sys/bus/pci/devices/{uri}/iommu_group")
    return not err


def skip_unless_registers_host_memory(cijoe, device, be_opts):
    if be_opts["mem"] not in REGISTERS_HOST_MEMORY:
        pytest.skip(reason="Backend does not register host memory")

    # Unless an IOMMU addresses it by virtual address, SPDK registers a PCIe
    # device only memory in hugetlb pages, which the tests do not allocate
    if be_opts["mem"] == "spdk" and "pcie" in device["labels"]:
        if not has_iommu_group(cijoe, device):
            pytest.skip(reason="SPDK without an IOMMU registers hugetlb memory only")
        err, _ = cijoe.run('test "$XNVME_SPDK_IOVA_MODE" = pa')
        if not err:
            pytest.skip(reason="SPDK with iova_mode=pa registers hugetlb memory only")


@xnvme_parametrize(labels=["dev"], opts=["be"])
def test_mem_map_unmap(cijoe, device, be_opts, cli_args):
    skip_unless_registers_host_memory(cijoe, device, be_opts)

    err, _ = cijoe.run(f"xnvme_tests_map mem_map_unmap {cli_args} --count 31")
    assert not err


@xnvme_parametrize(labels=["dev"], opts=["be"])
def test_register_counted(cijoe, device, be_opts, cli_args):
    skip_unless_registers_host_memory(cijoe, device, be_opts)

    err, _ = cijoe.run(f"xnvme_tests_map register_counted {cli_args}")
    assert not err


@xnvme_parametrize(labels=["pcie"], opts=["be"])
def test_mem_map_iova_pa(cijoe, device, be_opts, cli_args):
    """SPDK behind an IOMMU in iova_mode=pa refuses memory not in hugetlb pages"""

    if be_opts["mem"] != "spdk":
        pytest.skip(reason="Only SPDK has an iova_mode")
    if not has_iommu_group(cijoe, device):
        pytest.skip(reason="Without an IOMMU, SPDK refuses this in either mode")
    # A served client takes the iova_mode of the server
    if get_homi_id():
        pytest.skip(reason="The server's iova_mode applies, not the client's")

    err, state = cijoe.run(
        f"XNVME_SPDK_IOVA_MODE=pa xnvme_tests_map mem_map_unmap {cli_args} --count 1"
    )
    assert err
    assert "errno: 95" in state.output()  # ENOTSUP on Linux, where SPDK runs


@xnvme_parametrize(labels=["pcie"], opts=["be"])
def test_mptr_registration(cijoe, device, be_opts, cli_args):
    """
    Metadata memory the backend has no record of is refused, not submitted.

    The subcommand writes with a registered mbuf first and an unregistered one
    second, reporting itself inconclusive where the namespace has no
    out-of-band space, since such a namespace refuses either write.
    """

    # uPCIe translates through the registry only where the device consumes
    # physical addresses. Behind an IOMMU the address comes from the mapping
    # instead, so there is nothing for an unregistered buffer to fail to
    # resolve to and the check does not apply. Other backends are asked
    # regardless of how they get there.
    if be_opts["be"].startswith("upcie"):
        uri = device["uri"]
        err, state = cijoe.run(
            f"sh -c 'basename $(readlink -f /sys/bus/pci/devices/{uri}/driver)'"
        )
        if err or "uio_pci_generic" not in state.output():
            pytest.skip(
                reason="uPCIe needs uio_pci_generic; with vfio the IOMMU translates"
            )

    err, state = cijoe.run(f"xnvme_tests_map mptr_registration {cli_args}")
    if "INCONCLUSIVE" in state.output():
        pytest.skip(
            reason="namespace has no out-of-band space; mptr is refused regardless"
        )
    assert not err
