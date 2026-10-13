from ..conftest import HomiServer, get_homi_id, xnvme_parametrize


@xnvme_parametrize(labels=["nvm"], opts=[])
def test_delayed_identification(cijoe, device, be_opts, cli_args):
    """Opens the controller itself, so a server must not hold it"""

    if get_homi_id():
        HomiServer.stop(cijoe)

    err, state = cijoe.run(f"xnvme test delay-identification open {device['uri']}")

    assert "XNVME_GEO_UNKNOWN" in state.output()
    assert "XNVME_GEO_CONVENTIONAL" in state.output()
