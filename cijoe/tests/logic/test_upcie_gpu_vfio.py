import pytest

from ..conftest import xnvme_parametrize


@xnvme_parametrize(labels=["pcie"], opts=["be"])
def test_gpu_buffers_reach_the_controller_under_vfio(cijoe, device, be_opts, cli_args):
    """
    A controller behind an IOMMU does I/O into GPU memory.

    Written against the kernel this is heading toward rather than the one
    installed: IOMMU_IOAS_MAP_FILE does not yet accept a dma-buf exported by a
    GPU runtime, so the backend maps the heap, is refused, and says which call
    refused it. That is a skip here rather than a failure, so a kernel carrying
    the support turns this green without an edit, and one without it does not
    report a defect that is not ours.
    """

    if be_opts["be"] not in ("upcie-cuda", "upcie-hip"):
        pytest.skip(reason="Only the GPU backends put data buffers in device memory")

    uri = device["uri"]
    err, state = cijoe.run(
        f"sh -c 'basename $(readlink -f /sys/bus/pci/devices/{uri}/driver)'"
    )
    if err or "vfio-pci" not in state.output():
        pytest.skip(
            reason="Requires vfio-pci; under uio the controller uses phys addrs"
        )

    err, state = cijoe.run(
        f"xnvmeperf verify {uri} --be {be_opts['be']} --iosize 4096 --count 32"
    )
    if err and "Operation not supported" in state.output():
        pytest.skip(
            reason="This kernel does not map GPU memory into an IOAS; see the design note"
        )

    assert not err, "GPU I/O under vfio failed for a reason other than the kernel's"
    assert "0 mismatches" in state.output()
