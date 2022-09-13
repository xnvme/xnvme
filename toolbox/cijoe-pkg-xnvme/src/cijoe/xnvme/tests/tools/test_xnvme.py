import pytest

from ..conftest import XnvmeDriver, xnvme_parametrize


def test_library_info(cijoe):

    err, _ = cijoe.run("xnvme library-info")
    assert not err


def test_enum(cijoe):

    XnvmeDriver.kernel_attach(cijoe)
    err, _ = cijoe.run("xnvme enum")
    assert not err

    XnvmeDriver.kernel_detach(cijoe)
    err, _ = cijoe.run("xnvme enum")
    assert not err


@xnvme_parametrize(labels=["fabrics"], opts=["be"])
def test_enum_fabrics(cijoe, device, be_opts, cli_args):

    err, _ = cijoe.run(f"xnvme enum --uri {device['uri']}")
    assert not err


@xnvme_parametrize(labels=["dev"], opts=["be", "admin"])
def test_info(cijoe, device, be_opts, cli_args):

    err, _ = cijoe.run(f"xnvme info {cli_args}")
    assert not err


@xnvme_parametrize(labels=["dev"], opts=["be", "admin"])
def test_idfy(cijoe, device, be_opts, cli_args):

    err, _ = cijoe.run(
        f"xnvme idfy {cli_args} --cns 0x0 --cntid 0x0 --setid 0x0 --uuid 0x0"
    )
    assert not err


@xnvme_parametrize(labels=["dev"], opts=["be", "admin"])
def test_idfy_ns(cijoe, device, be_opts, cli_args):

    err, _ = cijoe.run(f"xnvme idfy-ns {cli_args} --nsid {device['nsid']}")
    assert not err


@xnvme_parametrize(labels=["dev"], opts=["be", "admin"])
def test_idfy_ctrlr(cijoe, device, be_opts, cli_args):

    err, _ = cijoe.run(f"xnvme idfy-ctrlr {cli_args}")
    assert not err


@xnvme_parametrize(labels=["dev"], opts=["be", "admin"])
def test_idfy_cs(cijoe, device, be_opts, cli_args):

    if be_opts["be"] == "linux" and be_opts["admin"] in ["block"]:
        pytest.skip(reason="[admin=block] does not implement idfy-cs")

    err, _ = cijoe.run(f"xnvme idfy-cs {cli_args}")
    assert not err


@xnvme_parametrize(labels=["nvm"], opts=["be", "admin"])
def test_format(cijoe, device, be_opts, cli_args):

    if be_opts["be"] == "linux" and be_opts["admin"] in ["block"]:
        pytest.skip(reason="[admin=block] does not implement format")

    err, _ = cijoe.run(f"xnvme format {cli_args}")
    assert not err


@xnvme_parametrize(labels=["nvm"], opts=["be", "admin"])
def test_sanitize(cijoe, device, be_opts, cli_args):

    if be_opts["be"] == "linux" and be_opts["admin"] in ["block"]:
        pytest.skip(reason="[admin=block] does not implement sanitize")

    pytest.skip(reason="TODO: always fails. Investigate.")

    err, _ = cijoe.run(f"xnvme sanitize {cli_args}")
    assert not err


@xnvme_parametrize(labels=["dev"], opts=["be", "admin"])
def test_log_erri(cijoe, device, be_opts, cli_args):

    if be_opts["be"] == "linux" and be_opts["admin"] in ["block"]:
        pytest.skip(reason="[admin=block] does not implement health-log")

    err, _ = cijoe.run(f"xnvme log-erri {cli_args} --nsid {device['nsid']}")
    assert not err


@xnvme_parametrize(labels=["dev"], opts=["be", "admin"])
def test_log_health(cijoe, device, be_opts, cli_args):

    if be_opts["be"] == "linux" and be_opts["admin"] in ["block"]:
        pytest.skip(reason="[admin=block] does not implement health-log")

    # Check the controller
    err, _ = cijoe.run(f"xnvme log-health {cli_args} --nsid 0xFFFFFFFF")

    # Check the namespace
    err, _ = cijoe.run(f"xnvme log-health {cli_args} --nsid {device['nsid']}")
    assert not err


@xnvme_parametrize(labels=["dev"], opts=["be", "admin"])
def test_log(cijoe, device, be_opts, cli_args):

    if be_opts["be"] == "linux" and be_opts["admin"] in ["block"]:
        pytest.skip(reason="[admin=block] does not implement get_log")

    lid, lsp, lpo_nbytes, rae, nbytes = "0x1", "0x0", 0, 0, 4096

    err, _ = cijoe.run(
        f"xnvme log {cli_args} --lid {lid} --lsp {lsp} --lpo-nbytes {lpo_nbytes} "
        f"--rae {rae} --data-nbytes {nbytes}"
    )
    assert not err


@xnvme_parametrize(labels=["dev"], opts=["be", "admin"])
def test_feature_get(cijoe, device, be_opts, cli_args):

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

    err, _ = cijoe.run(f"xnvme feature-set {cli_args} --fid 0x4 --feat 0x1 --save")

    if be_opts["admin"] == "block":
        assert err
    else:
        assert not err


@xnvme_parametrize(labels=["dev"], opts=["be", "admin"])
def test_padc(cijoe, device, be_opts, cli_args):
    """Construct and send an admin command (identify-controller)"""

    if be_opts["be"] == "linux" and be_opts["admin"] in ["block"]:
        pytest.skip(reason="[admin=block] does not support admin-passthru")

    opcode = "0x02"
    cns = "0x1"
    cmd_path = "/tmp/cmd-out.nvmec"

    err, state = cijoe.run(f"xnvme info {cli_args}")
    assert not err

    data_nbytes = 0
    for line in state.output():
        if "lba_nbytes" not in line:
            continue
        key, val = line.split(":")
        data_nbytes = int(val.strip())
        assert data_nbytes > 0
        break

    err, _ = cijoe.run(f"rm {cmd_path}")

    err, _ = cijoe.run(
        f"nvmec create --opcode {opcode} --cdw10 {cns} --cmd-output {cmd_path}"
    )
    assert not err

    err, _ = cijoe.run(f"nvmec show --cmd-input {cmd_path}")
    assert not err

    err, _ = cijoe.run(
        f"xnvme padc {cli_args} --cmd-input {cmd_path} --data-nbytes {data_nbytes}"
    )
    assert not err


@xnvme_parametrize(labels=["dev"], opts=["be", "admin"])
def test_pioc(cijoe, device, be_opts, cli_args):
    """Construct and send an I/O command (read)"""

    opcode = "0x02"
    cmd_path = "/tmp/cmd-out.nvmec"

    err, state = cijoe.run(f"xnvme info {cli_args}")
    assert not err

    data_nbytes = 0
    for line in state.output():
        if "lba_nbytes" not in line:
            continue
        key, val = line.split(":")
        data_nbytes = int(val.strip())
        assert data_nbytes > 0
        break

    err, _ = cijoe.run(f"rm {cmd_path}")

    err, _ = cijoe.run(
        f"nvmec create"
        f" --opcode {opcode}"
        f" --nsid {device['nsid']}"
        f" --cmd-output {cmd_path}"
    )
    assert not err

    err, _ = cijoe.run(f"nvmec show --cmd-input {cmd_path}")
    assert not err

    err, _ = cijoe.run(
        f"xnvme pioc {cli_args} --cmd-input {cmd_path} --data-nbytes {data_nbytes}"
    )
    assert not err
