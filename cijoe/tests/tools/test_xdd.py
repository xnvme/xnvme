import pytest

from ..conftest import XnvmeDriver, xnvme_parametrize


def test_xdd(cijoe):
    err, _ = cijoe.run("dd if=/dev/random of=test.0 count=2 bs=1M")
    assert not err

    commands = [
        (
            "xdd sync --data-input test.0 --data-output test.1 --data-nbytes 2097152",
            "cmp test.0 test.1 && rm test.1",
        ),
        (
            "xdd sync --data-input test.0 --data-output test.1 --data-nbytes 1048576 --offset 1048576",
            "cmp --ignore-initial=1048576:0 test.0 test.1 && rm test.1",
        ),
        (
            "xdd sync --data-input test.0 --data-output test.1 --data-nbytes 1048577 --offset 1048575",
            "cmp --ignore-initial=1048575:0 test.0 test.1 && rm test.1",
        ),
        (
            "xdd sync --data-input test.0 --data-output test.1 --data-nbytes 1048575 --offset 1048577",
            "cmp --ignore-initial=1048577:0 test.0 test.1 && rm test.1",
        ),
        (
            "xdd sync --data-input test.0 --data-output test.1 --data-nbytes 1048576 --offset 1048576 --iosize 8192",
            "cmp --ignore-initial=1048576:0 test.0 test.1 && rm test.1",
        ),
        (
            "xdd async --data-input test.0 --data-output test.1 --data-nbytes 2097152",
            "cmp test.0 test.1 && rm test.1",
        ),
        (
            "xdd async --data-input test.0 --data-output test.1 --data-nbytes 1048576 --offset 1048576",
            "cmp --ignore-initial=1048576:0 test.0 test.1 && rm test.1",
        ),
        (
            "xdd async --data-input test.0 --data-output test.1 --data-nbytes 1048577 --offset 1048575",
            "cmp --ignore-initial=1048575:0 test.0 test.1 && rm test.1",
        ),
        (
            "xdd async --data-input test.0 --data-output test.1 --data-nbytes 1048575 --offset 1048577",
            "cmp --ignore-initial=1048577:0 test.0 test.1 && rm test.1",
        ),
        (
            "xdd async --data-input test.0 --data-output test.1 --data-nbytes 1048576 --offset 1048576 --iosize 8192",
            "cmp --ignore-initial=1048576:0 test.0 test.1 && rm test.1",
        ),
        (
            "xdd async --data-input test.0 --data-output test.1 --data-nbytes 1048576 --offset 1048576 --qdepth 32",
            "cmp --ignore-initial=1048576:0 test.0 test.1 && rm test.1",
        ),
        (
            "xdd async --data-input test.0 --data-output test.1 --data-nbytes 1048576 --offset 1048576 --qdepth 32 --iosize 8192",
            "cmp --ignore-initial=1048576:0 test.0 test.1 && rm test.1",
        ),
    ]

    for xdd_command, cmp_command in commands:
        err, _ = cijoe.run(xdd_command)
        assert not err

        err, _ = cijoe.run(cmp_command)
        assert not err
