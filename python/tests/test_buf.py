import xnvme.cython_bindings as xnvme


class TestBufferAlloc:
    def test_buf_alloc_free(self, dev):
        count = 5
        for i in range(count):
            buf_nbytes = 1 << i
            buf = xnvme.xnvme_buf_alloc(dev, buf_nbytes)
            assert isinstance(buf, xnvme.xnvme_void_p)
            xnvme.xnvme_buf_free(dev, buf)

    def test_virt_buf_alloc_free(self, dev):
        count = 5
        for i in range(count):
            buf_nbytes = 1 << i
            buf = xnvme.xnvme_buf_virt_alloc(0x1000, buf_nbytes)
            assert isinstance(buf, xnvme.xnvme_void_p)
            xnvme.xnvme_buf_virt_free(buf)
