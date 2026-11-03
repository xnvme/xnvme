import pytest

from ..conftest import xnvme_parametrize


@xnvme_parametrize(labels=["dev"], opts=["be"])
def test_mem_map_unmap(cijoe, device, be_opts, cli_args):
    if be_opts["mem"] not in ("libvfn", "posix", "spdk", "windows"):
        pytest.skip(reason="Backend does not support memory-mapping")

    # Without an IOMMU, SPDK maps only memory in hugetlb pages, which the test
    # does not allocate
    if be_opts["mem"] == "spdk":
        uri = device["uri"]
        err, state = cijoe.run(
            f"sh -c 'basename $(readlink -f /sys/bus/pci/devices/{uri}/driver)'"
        )
        if err or "uio_pci_generic" in state.output():
            pytest.skip(reason="SPDK without an IOMMU maps hugetlb memory only")

    err, _ = cijoe.run(f"xnvme_tests_map mem_map_unmap {cli_args} --count 31")
    assert not err


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
