import pytest

from ..conftest import cijoe_config_get_device, get_homi_id, xnvme_parametrize


def _skip_without_udmabuf(cijoe, be_opts):
    """upcie registers a udmabuf only with the dmabuf_import module"""

    if be_opts["be"] != "upcie":
        pytest.skip(reason="Written for upcie, whose I/O goes through the dma-buf")

    err, _ = cijoe.run("test -c /dev/udmabuf && test -c /dev/dmabuf_import")
    if err:
        pytest.skip(reason="Requires /dev/udmabuf and the dmabuf_import module")


@xnvme_parametrize(labels=["pcie"], opts=["be"])
def test_dmabuf_udmabuf_rw(cijoe, device, be_opts, cli_args):
    """Writes and reads through a registered udmabuf, and through a second controller"""

    _skip_without_udmabuf(cijoe, be_opts)

    second = cijoe_config_get_device(be_opts["label"] + ["second_ctrlr"])
    alt_uri = f"--alt-uri {second['uri']}" if second else ""
    err, state = cijoe.run(f"xnvme test dmabuf-udmabuf rw {cli_args} {alt_uri}")
    assert not err, state.output()


@xnvme_parametrize(labels=["pcie"], opts=["be"])
def test_dmabuf_udmabuf_reserved(cijoe, device, be_opts, cli_args):
    """A udmabuf registered at addresses reserved with PROT_NONE"""

    _skip_without_udmabuf(cijoe, be_opts)

    err, state = cijoe.run(f"xnvme test dmabuf-udmabuf reserved {cli_args}")
    assert not err, state.output()


@xnvme_parametrize(labels=["pcie"], opts=["be"])
def test_dmabuf_udmabuf_many(cijoe, device, be_opts, cli_args):
    """100 udmabufs registered at once, each read into"""

    _skip_without_udmabuf(cijoe, be_opts)

    err, state = cijoe.run(f"xnvme test dmabuf-udmabuf many {cli_args}")
    assert not err, state.output()


@xnvme_parametrize(labels=["pcie"], opts=["be"])
def test_dmabuf_udmabuf_inside_hip(cijoe, device, be_opts, cli_args):
    """On upcie-hip, -EBUSY for a udmabuf inside one registered through the server

    upcie-hip registers a dma-buf only as a client of a homi server. With a second
    controller, the udmabuf is registered through that one too.
    """

    if be_opts["be"] != "upcie-hip":
        pytest.skip(reason="Written for upcie-hip")
    if not get_homi_id():
        pytest.skip(reason="upcie-hip registers a dma-buf only through a homi server")
    err, _ = cijoe.run("test -c /dev/udmabuf && test -c /dev/dmabuf_import")
    if err:
        pytest.skip(reason="Requires /dev/udmabuf and the dmabuf_import module")

    second = cijoe_config_get_device(be_opts["label"] + ["second_ctrlr"])
    alt_uri = f"--alt-uri {second['uri']}" if second else ""
    err, state = cijoe.run(f"xnvme test dmabuf-udmabuf inside {cli_args} {alt_uri}")
    assert not err, state.output()


@xnvme_parametrize(labels=["dmabuf"], opts=["be"])
def test_dmabuf_udmabuf_inside(cijoe, device, be_opts, cli_args):
    """On io_uring_dmabuf, -EBUSY for a udmabuf named inside a registered one"""

    err, _ = cijoe.run("test -c /dev/udmabuf")
    if err:
        pytest.skip(reason="Requires /dev/udmabuf")

    err, state = cijoe.run(f"xnvme test dmabuf-udmabuf inside {cli_args} --direct 1")
    assert not err, state.output()
