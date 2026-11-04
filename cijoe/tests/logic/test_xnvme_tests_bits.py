def test_get_set(cijoe):
    err, _ = cijoe.run("xnvme_tests_bits get-set")
    assert not err
