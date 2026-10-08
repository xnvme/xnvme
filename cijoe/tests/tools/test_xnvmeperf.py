from ..conftest import get_shm_id, xnvme_parametrize


@xnvme_parametrize(labels=["nvm"], opts=["be"])
def test_run_read(cijoe, device, be_opts, cli_args):
    shm_id = get_shm_id()
    shm_arg = f"--homi-id {shm_id}" if shm_id else ""

    err, _ = cijoe.run(
        f"xnvmeperf run --iopattern read --qdepth 32 --iosize 4096 --runtime 3"
        f" --cpumask 0x1 --be {be_opts['be']} {shm_arg} {device['uri']}"
    )
    assert not err


@xnvme_parametrize(labels=["nvm"], opts=["be"])
def test_run_write(cijoe, device, be_opts, cli_args):
    shm_id = get_shm_id()
    shm_arg = f"--homi-id {shm_id}" if shm_id else ""

    err, _ = cijoe.run(
        f"xnvmeperf run --iopattern write --qdepth 32 --iosize 4096 --runtime 3"
        f" --cpumask 0x1 --be {be_opts['be']} {shm_arg} {device['uri']}"
    )
    assert not err


@xnvme_parametrize(labels=["nvm"], opts=["be"])
def test_run_randread(cijoe, device, be_opts, cli_args):
    shm_id = get_shm_id()
    shm_arg = f"--homi-id {shm_id}" if shm_id else ""

    err, _ = cijoe.run(
        f"xnvmeperf run --iopattern randread --qdepth 32 --iosize 4096 --runtime 3"
        f" --cpumask 0x1 --be {be_opts['be']} {shm_arg} {device['uri']}"
    )
    assert not err


@xnvme_parametrize(labels=["nvm"], opts=["be"])
def test_run_randwrite(cijoe, device, be_opts, cli_args):
    shm_id = get_shm_id()
    shm_arg = f"--homi-id {shm_id}" if shm_id else ""

    err, _ = cijoe.run(
        f"xnvmeperf run --iopattern randwrite --qdepth 32 --iosize 4096 --runtime 3"
        f" --cpumask 0x1 --be {be_opts['be']} {shm_arg} {device['uri']}"
    )
    assert not err


@xnvme_parametrize(labels=["nvm"], opts=["be"])
def test_verify(cijoe, device, be_opts, cli_args):
    shm_id = get_shm_id()
    shm_arg = f"--homi-id {shm_id}" if shm_id else ""

    err, _ = cijoe.run(
        f"xnvmeperf verify --iosize 4096 --count 64"
        f" --be {be_opts['be']} {shm_arg} {device['uri']}"
    )
    assert not err


@xnvme_parametrize(labels=["pcie", "nvm"], opts=["be"])
def test_run_uri_nsid(cijoe, device, be_opts, cli_args):
    """A device URI names its namespace with '/?nsid=<nsid>'"""

    shm_id = get_shm_id()
    shm_arg = f"--homi-id {shm_id}" if shm_id else ""
    err, _ = cijoe.run(
        f"xnvmeperf run --iopattern read --qdepth 32 --iosize 4096 --runtime 3"
        f" --cpumask 0x1 --be {be_opts['be']} {shm_arg}"
        f" '{device['uri']}/?nsid={device['nsid']}'"
    )
    assert not err


@xnvme_parametrize(labels=["pcie", "nvm"], opts=["be"])
def test_run_uri_nsid_inactive(cijoe, device, be_opts, cli_args):
    """A namespace named in the URI that is not active is refused"""

    shm_id = get_shm_id()
    shm_arg = f"--homi-id {shm_id}" if shm_id else ""
    # The configured controllers each have a single namespace
    nsid = int(device["nsid"]) + 1
    err, state = cijoe.run(
        f"xnvmeperf run --iopattern read --qdepth 32 --iosize 4096 --runtime 3"
        f" --cpumask 0x1 --be {be_opts['be']} {shm_arg}"
        f" '{device['uri']}/?nsid={nsid}'"
    )
    assert err, f"ran against an inactive namespace: {state.output()}"
