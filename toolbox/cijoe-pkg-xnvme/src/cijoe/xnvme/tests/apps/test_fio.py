from pathlib import Path

from cijoe.fio.wrapper import fio_fancy

from ..conftest import xnvme_parametrize


@xnvme_parametrize(labels=["dev"], opts=["be", "admin", "sync", "async"])
def test_fio_engine(cijoe, device, be_opts, cli_args):
    """
    The construction of the fio-invocation is done in 'cijoe.fio.wrapper.fio_fancy'
    """

    fio_output_fpath = Path("/tmp/fio-output.txt")

    # size = "64M"
    size = "1G"

    err, _ = fio_fancy(
        cijoe, fio_output_fpath, "verify", "xnvme", device, be_opts, {}, {"size": size}
    )
    assert not err
