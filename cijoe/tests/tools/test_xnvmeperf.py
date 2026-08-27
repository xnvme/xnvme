from ..conftest import get_cplane_id, xnvme_parametrize


@xnvme_parametrize(labels=["nvm"], opts=["be"])
def test_run_read(cijoe, device, be_opts, cli_args):
    cplane_id = get_cplane_id()
    flag = "--cplane-id" if be_opts["be"].startswith("upcie") else "--shm_id"
    cplane_arg = f"{flag} {cplane_id}" if cplane_id else ""

    err, _ = cijoe.run(
        f"xnvmeperf run --iopattern read --qdepth 32 --iosize 4096 --runtime 3"
        f" --cpumask 0x1 --be {be_opts['be']} {cplane_arg} {device['uri']}"
    )
    assert not err


@xnvme_parametrize(labels=["nvm"], opts=["be"])
def test_run_write(cijoe, device, be_opts, cli_args):
    cplane_id = get_cplane_id()
    flag = "--cplane-id" if be_opts["be"].startswith("upcie") else "--shm_id"
    cplane_arg = f"{flag} {cplane_id}" if cplane_id else ""

    err, _ = cijoe.run(
        f"xnvmeperf run --iopattern write --qdepth 32 --iosize 4096 --runtime 3"
        f" --cpumask 0x1 --be {be_opts['be']} {cplane_arg} {device['uri']}"
    )
    assert not err


@xnvme_parametrize(labels=["nvm"], opts=["be"])
def test_run_randread(cijoe, device, be_opts, cli_args):
    cplane_id = get_cplane_id()
    flag = "--cplane-id" if be_opts["be"].startswith("upcie") else "--shm_id"
    cplane_arg = f"{flag} {cplane_id}" if cplane_id else ""

    err, _ = cijoe.run(
        f"xnvmeperf run --iopattern randread --qdepth 32 --iosize 4096 --runtime 3"
        f" --cpumask 0x1 --be {be_opts['be']} {cplane_arg} {device['uri']}"
    )
    assert not err


@xnvme_parametrize(labels=["nvm"], opts=["be"])
def test_run_randwrite(cijoe, device, be_opts, cli_args):
    cplane_id = get_cplane_id()
    flag = "--cplane-id" if be_opts["be"].startswith("upcie") else "--shm_id"
    cplane_arg = f"{flag} {cplane_id}" if cplane_id else ""

    err, _ = cijoe.run(
        f"xnvmeperf run --iopattern randwrite --qdepth 32 --iosize 4096 --runtime 3"
        f" --cpumask 0x1 --be {be_opts['be']} {cplane_arg} {device['uri']}"
    )
    assert not err


@xnvme_parametrize(labels=["nvm"], opts=["be"])
def test_verify(cijoe, device, be_opts, cli_args):
    cplane_id = get_cplane_id()
    flag = "--cplane-id" if be_opts["be"].startswith("upcie") else "--shm_id"
    cplane_arg = f"{flag} {cplane_id}" if cplane_id else ""

    err, _ = cijoe.run(
        f"xnvmeperf verify --iosize 4096 --count 64"
        f" --be {be_opts['be']} {cplane_arg} {device['uri']}"
    )
    assert not err
