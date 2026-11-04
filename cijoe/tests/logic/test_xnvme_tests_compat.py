def test_deprecated(cijoe):
    err, _ = cijoe.run("xnvme_tests_compat deprecated")
    assert not err


def test_null_args(cijoe):
    err, _ = cijoe.run("xnvme_tests_compat null-args")
    assert not err
