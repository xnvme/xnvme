from ..conftest import xnvme_parametrize


@xnvme_parametrize(labels=["nvm"], opts=[])
def test_delayed_identification(cijoe, device, be_opts, cli_args):
    err, state = cijoe.run(f"xnvme_tests_delay_identification open {device['uri']}")

    assert "XNVME_GEO_UNKNOWN" in state.output()
    assert "XNVME_GEO_CONVENTIONAL" in state.output()
