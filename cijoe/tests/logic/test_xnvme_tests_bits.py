def test_get_set(cijoe):
    err, _ = cijoe.run("xnvme test bits get-set")
    assert not err
