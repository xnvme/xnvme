import os
from ctypes import byref, pointer

import numpy as np
import pytest
import xnvme.ctypes_bindings.api as xnvme
from conftest import xnvme_parametrize
from utils import buf_and_view, dev_from_params, xnvme_cmd_ctx_cpl_status

KEY_SIZE_BYTES = 128 // 8  # 128-bit keys in KV spec


class KVException(Exception):
    def __init__(self, err, status_code, status_code_type, *args, **kwargs):
        self.err = err
        self.status_code = status_code
        self.status_code_type = status_code_type
        super().__init__(*args, **kwargs)


def kvs_store(dev, kbuf, kbuf_nbytes, vbuf, vbuf_nbytes, opts=0):
    ctx = xnvme.xnvme_cmd_ctx_from_dev(dev)
    err = xnvme.xnvme_kvs_store(
        byref(ctx),
        xnvme.xnvme_dev_get_nsid(dev),
        kbuf,
        kbuf_nbytes,
        vbuf,
        vbuf_nbytes,
        opts,
    )
    if err or xnvme_cmd_ctx_cpl_status(pointer(ctx)):
        raise KVException(
            abs(err),
            ctx.cpl.status.sc,
            ctx.cpl.status.sct,
            os.strerror(abs(err)),
        )
    return ctx


def kvs_retrieve(dev, kbuf, kbuf_nbytes, vbuf, vbuf_nbytes, opts=0):
    ctx = xnvme.xnvme_cmd_ctx_from_dev(dev)
    err = xnvme.xnvme_kvs_retrieve(
        byref(ctx),
        xnvme.xnvme_dev_get_nsid(dev),
        kbuf,
        kbuf_nbytes,
        vbuf,
        vbuf_nbytes,
        opts,
    )
    if err or xnvme_cmd_ctx_cpl_status(pointer(ctx)):
        raise KVException(
            abs(err),
            ctx.cpl.status.sc,
            ctx.cpl.status.sct,
            os.strerror(abs(err)),
        )
    return ctx


def kvs_exist(dev, kbuf, kbuf_nbytes):
    ctx = xnvme.xnvme_cmd_ctx_from_dev(dev)
    err = xnvme.xnvme_kvs_exist(
        byref(ctx),
        xnvme.xnvme_dev_get_nsid(dev),
        kbuf,
        kbuf_nbytes,
    )
    if err or xnvme_cmd_ctx_cpl_status(pointer(ctx)):
        raise KVException(
            abs(err),
            ctx.cpl.status.sc,
            ctx.cpl.status.sct,
            os.strerror(abs(err)),
        )
    return ctx


def kvs_delete(dev, kbuf, kbuf_nbytes):
    ctx = xnvme.xnvme_cmd_ctx_from_dev(dev)
    err = xnvme.xnvme_kvs_delete(
        byref(ctx),
        xnvme.xnvme_dev_get_nsid(dev),
        kbuf,
        kbuf_nbytes,
    )
    if err or xnvme_cmd_ctx_cpl_status(pointer(ctx)):
        raise KVException(
            abs(err),
            ctx.cpl.status.sc,
            ctx.cpl.status.sct,
            os.strerror(abs(err)),
        )
    return ctx


def kvs_list(dev, kbuf, kbuf_nbytes, vbuf, vbuf_nbytes, opts=0):
    ctx = xnvme.xnvme_cmd_ctx_from_dev(dev)
    err = xnvme.xnvme_kvs_list(
        byref(ctx),
        xnvme.xnvme_dev_get_nsid(dev),
        kbuf,
        kbuf_nbytes,
        vbuf,
        vbuf_nbytes,
    )
    if err or xnvme_cmd_ctx_cpl_status(pointer(ctx)):
        raise KVException(
            abs(err),
            ctx.cpl.status.sc,
            ctx.cpl.status.sct,
            os.strerror(abs(err)),
        )
    return ctx


class TestKeyValue:
    @xnvme_parametrize(labels=["kvs"], opts=["be"])
    def test_single_key(self, cijoe, device, be_opts, cli_args):
        dev = dev_from_params(device, be_opts)
        assert dev

        vbuf, vbuf_view = buf_and_view(dev, 4096)
        kbuf, kbuf_view = buf_and_view(dev, KEY_SIZE_BYTES)

        key = b"marco"
        value = b"polo"

        vbuf_view[: len(value)] = memoryview(value)
        kbuf_view[: len(key)] = memoryview(key)

        kvs_store(dev, kbuf, len(key), vbuf, len(value))

        vbuf_view[:] = 0  # Zero to be certain we are reading correctly

        kvs_retrieve(dev, kbuf, len(key), vbuf, len(value))

        assert np.all(
            vbuf_view[: len(value)] == memoryview(value)
        ), "Retreiving returned different value"

        xnvme.xnvme_dev_close(dev)

    @xnvme_parametrize(labels=["kvs"], opts=["be"])
    def test_delete_exist(self, cijoe, device, be_opts, cli_args):
        dev = dev_from_params(device, be_opts)
        assert dev

        vbuf, vbuf_view = buf_and_view(dev, 4096)
        kbuf, kbuf_view = buf_and_view(dev, KEY_SIZE_BYTES)

        key = b"deleteme"
        value = b"x" * 128

        vbuf_view[: len(value)] = memoryview(value)
        kbuf_view[: len(key)] = memoryview(key)

        # Store a key we can delete
        kvs_store(dev, kbuf, len(key), vbuf, len(value))

        # Before we delete, we verify the key is there.
        kvs_exist(dev, kbuf, len(key))

        # Delete the key
        kvs_delete(dev, kbuf, len(key))

        # Reading should now fail
        with pytest.raises(KVException) as assertion:
            kvs_exist(dev, kbuf, len(key))
        assert assertion.value.status_code_type == xnvme.XNVME_STATUS_CODE_TYPE_CMDSPEC
        assert assertion.value.status_code == xnvme.XNVME_SPEC_KV_SC_KEY_NOT_EXISTS

        xnvme.xnvme_dev_close(dev)

    @xnvme_parametrize(labels=["kvs"], opts=["be"])
    def test_compress(self, cijoe, device, be_opts, cli_args):
        """
        Bit 10 if set to '1' specifies that the controller shall not compress the KV value. Bit 10 if cleared to '0'
        specifies that the controller shall compress the KV value if compression is supported.
        """
        dev = dev_from_params(device, be_opts)
        assert dev

        vbuf, vbuf_view = buf_and_view(dev, 4096)
        kbuf, kbuf_view = buf_and_view(dev, KEY_SIZE_BYTES)

        key = b"compressme"
        value = b"x" * 128

        vbuf_view[: len(value)] = memoryview(value)
        kbuf_view[: len(key)] = memoryview(key)

        kvs_store(
            dev,
            kbuf,
            len(key),
            vbuf,
            len(value),
            opts=xnvme.XNVME_KVS_STORE_OPT_COMPRESS,
        )

        xnvme.xnvme_buf_free(dev, vbuf)
        xnvme.xnvme_buf_free(dev, kbuf)
        xnvme.xnvme_dev_close(dev)

    @xnvme_parametrize(labels=["kvs"], opts=["be"])
    def test_dont_store_if_key_exists(self, cijoe, device, be_opts, cli_args):
        """
        Bit 9 if set to '1' specifies that the controller shall not store the KV value if the KV key exists. Bit 9 if
        cleared to '0' specifies that the controller shall store the KV value if other Store Options are met.
        """
        dev = dev_from_params(device, be_opts)
        assert dev

        vbuf, vbuf_view = buf_and_view(dev, 4096)
        kbuf, kbuf_view = buf_and_view(dev, KEY_SIZE_BYTES)

        key = b"Ipreexist"
        value = b"x" * 128

        vbuf_view[: len(value)] = memoryview(value)
        kbuf_view[: len(key)] = memoryview(key)

        kvs_store(dev, kbuf, len(key), vbuf, len(value))  # Make the key preexist

        # Fail to replace existing key
        with pytest.raises(KVException) as assertion:
            kvs_store(
                dev,
                kbuf,
                len(key),
                vbuf,
                len(value),
                opts=xnvme.XNVME_KVS_STORE_OPT_DONT_STORE_IF_KEY_EXISTS,
            )
        assert assertion.value.status_code_type == xnvme.XNVME_STATUS_CODE_TYPE_CMDSPEC
        assert assertion.value.status_code == xnvme.XNVME_SPEC_KV_SC_KEY_EXISTS

        # Deleting the key first should now make it work
        kvs_delete(dev, kbuf, len(key))
        kvs_store(
            dev,
            kbuf,
            len(key),
            vbuf,
            len(value),
            opts=xnvme.XNVME_KVS_STORE_OPT_DONT_STORE_IF_KEY_EXISTS,
        )

        xnvme.xnvme_buf_free(dev, vbuf)
        xnvme.xnvme_buf_free(dev, kbuf)
        xnvme.xnvme_dev_close(dev)

    @xnvme_parametrize(labels=["kvs"], opts=["be"])
    def test_dont_store_if_key_doesnt_exist(self, cijoe, device, be_opts, cli_args):
        """
        Bit 8 if set to '1' specifies that the controller shall not store the KV value if the KV key does not exists.
        Bit 8 if cleared to '0' specifies that the controller shall store the KV value if other Store Options are met.
        """
        dev = dev_from_params(device, be_opts)
        assert dev

        vbuf, vbuf_view = buf_and_view(dev, 4096)
        kbuf, kbuf_view = buf_and_view(dev, KEY_SIZE_BYTES)

        key = b"Ipreexist"
        value = b"x" * 128

        vbuf_view[: len(value)] = memoryview(value)
        kbuf_view[: len(key)] = memoryview(key)

        kvs_store(dev, kbuf, len(key), vbuf, len(value))  # Make the key preexist

        # Only allowed to replace existing key
        kvs_store(
            dev,
            kbuf,
            len(key),
            vbuf,
            len(value),
            opts=xnvme.XNVME_KVS_STORE_OPT_DONT_STORE_IF_KEY_NOT_EXISTS,
        )

        # Deleting the key first should now make it fail
        kvs_delete(dev, kbuf, len(key))
        with pytest.raises(KVException) as assertion:
            kvs_store(
                dev,
                kbuf,
                len(key),
                vbuf,
                len(value),
                opts=xnvme.XNVME_KVS_STORE_OPT_DONT_STORE_IF_KEY_NOT_EXISTS,
            )
        assert assertion.value.status_code_type == xnvme.XNVME_STATUS_CODE_TYPE_CMDSPEC
        assert assertion.value.status_code == xnvme.XNVME_SPEC_KV_SC_KEY_NOT_EXISTS

        xnvme.xnvme_buf_free(dev, vbuf)
        xnvme.xnvme_buf_free(dev, kbuf)
        xnvme.xnvme_dev_close(dev)

    @xnvme_parametrize(labels=["kvs"], opts=["be"])
    def test_multiple_keys(self, cijoe, device, be_opts, cli_args):
        dev = dev_from_params(device, be_opts)
        assert dev

        vbuf, vbuf_view = buf_and_view(dev, 4096)
        kbuf, kbuf_view = buf_and_view(dev, KEY_SIZE_BYTES)

        def kv_for_n(n):
            key = f"marco_{n}".encode()
            value = f"polo_{n}".encode()
            return key, value

        # Store multiple values
        for n in range(128):
            key, value = kv_for_n(n)
            vbuf_view[: len(value)] = memoryview(value)
            kbuf_view[: len(key)] = memoryview(key)

            kvs_store(dev, kbuf, len(key), vbuf, len(value))

        # Retreive all values
        for n in range(128):
            vbuf_view[:] = 0  # Zero to be certain we are reading correctly

            key, value = kv_for_n(n)
            kbuf_view[: len(key)] = memoryview(key)

            kvs_retrieve(dev, kbuf, len(key), vbuf, len(value))

            assert np.all(
                vbuf_view[: len(value)] == memoryview(value)
            ), "Retreiving returned different value"

        xnvme.xnvme_buf_free(dev, vbuf)
        xnvme.xnvme_buf_free(dev, kbuf)
        xnvme.xnvme_dev_close(dev)

    @xnvme_parametrize(labels=["kvs"], opts=["be"])
    def test_list(self, cijoe, device, be_opts, cli_args):
        dev = dev_from_params(device, be_opts)
        assert dev

        buffer_size = 4096
        vbuf, vbuf_view = buf_and_view(dev, buffer_size)
        kbuf, kbuf_view = buf_and_view(dev, KEY_SIZE_BYTES)

        def kv_for_n(n):
            key = f"list_{n}".encode()
            value = f"polo_{n}".encode()
            return key, value

        # Store multiple values
        for n in range(128):
            key, value = kv_for_n(n)
            vbuf_view[: len(value)] = memoryview(value)
            kbuf_view[: len(key)] = memoryview(key)

            kvs_store(dev, kbuf, len(key), vbuf, len(value))

        def unpack_list_data(list_data):
            num_keys = int(
                list_data[:4].view(np.uint32)
            )  # First 32 bits define the number of keys
            key_data = list_data[4:]  # The initial key data
            read_keys = []
            for _ in range(num_keys):
                # Extract key length from data
                kbuf_nbytes = int(key_data[:2].view(np.uint16))
                assert kbuf_nbytes <= 16, "Invalid key length"

                # Extract key name from data
                key = bytes(key_data[2 : 2 + kbuf_nbytes])
                read_keys.append(key)

                # Offset the buffer for next iteration
                padding = (
                    2 + kbuf_nbytes
                ) % 4  # "[...] end the data structure on a 4 byte boundary"
                key_data = key_data[
                    2 + kbuf_nbytes + padding :
                ]  # The remaining key data
            return read_keys

        # Finding all keys that can fit in the buffer
        vbuf_view[:] = 0  # Zero to be certain we are reading correctly
        kbuf_view[:] = 0  # Clear key to get what the device considers "first"

        kvs_list(dev, kbuf, len(key), vbuf, buffer_size)
        read_keys = unpack_list_data(vbuf_view)
        assert (
            len(read_keys) >= 128
        )  # We just created 128, so there should be at least that

        # Fetch to small buffer
        vbuf_view[:] = 0  # Zero to be certain we are reading correctly
        kbuf_view[:] = 0  # Clear key to get what the device considers "first"

        kvs_list(dev, kbuf, len(key), vbuf, 128)
        read_keys = unpack_list_data(vbuf_view)
        assert len(read_keys) < 128  # We should have significantly less than 128 now

        # Find the specific range of keys we just created
        vbuf_view[:] = 0  # Zero to be certain we are reading correctly
        kbuf_view[:] = 0  # Clear key to fill a new one
        key = b"list_0"
        kbuf_view[: len(key)] = memoryview(key)

        kvs_list(dev, kbuf, len(key), vbuf, buffer_size)
        read_keys = unpack_list_data(vbuf_view)
        assert len(read_keys) >= 128  # Assert we found the amount of keys we created
        assert all(x == f"list_{i}".encode() for i, x in enumerate(read_keys[:128]))

        # Continue from a specific key
        vbuf_view[:] = 0  # Zero to be certain we are reading correctly
        kbuf_view[:] = 0  # Clear key to fill a new one
        key = b"list_64"
        kbuf_view[: len(key)] = memoryview(key)

        kvs_list(dev, kbuf, len(key), vbuf, buffer_size)
        read_keys = unpack_list_data(vbuf_view)
        assert len(read_keys) >= 64  # Assert we found the amount of keys we created
        assert all(x == f"list_{i+64}".encode() for i, x in enumerate(read_keys[:64]))

        xnvme.xnvme_buf_free(dev, vbuf)
        xnvme.xnvme_buf_free(dev, kbuf)
        xnvme.xnvme_dev_close(dev)

    @pytest.mark.parametrize(
        "expected_sc, buffer_size",
        [
            (0, 4096),
            (xnvme.XNVME_SPEC_KV_SC_INVALID_VAL_SIZE, 4096 + 1),  # QEMU limitation
        ],
    )
    @xnvme_parametrize(labels=["kvs"], opts=["be"])
    def test_large_value(
        self, cijoe, device, be_opts, cli_args, buffer_size, expected_sc
    ):
        dev = dev_from_params(device, be_opts)
        assert dev

        vbuf, vbuf_view = buf_and_view(dev, buffer_size)
        kbuf, kbuf_view = buf_and_view(dev, KEY_SIZE_BYTES)

        key = b"marco_large"
        kbuf_view[: len(key)] = memoryview(key)
        vbuf_view[:] = np.arange(buffer_size)

        if expected_sc != 0:
            with pytest.raises(KVException) as assertion:
                kvs_store(dev, kbuf, len(key), vbuf, buffer_size)
            assert assertion.value.status_code == expected_sc
        else:
            kvs_store(dev, kbuf, len(key), vbuf, buffer_size)

        xnvme.xnvme_buf_free(dev, vbuf)
        xnvme.xnvme_buf_free(dev, kbuf)
        xnvme.xnvme_dev_close(dev)

    @xnvme_parametrize(labels=["kvs"], opts=["be"])
    def test_large_key(self, cijoe, device, be_opts, cli_args):
        dev = dev_from_params(device, be_opts)
        assert dev

        buffer_size = 128
        vbuf, vbuf_view = buf_and_view(dev, buffer_size)
        kbuf, kbuf_view = buf_and_view(dev, 32)  # On purpose larger than KEY_SIZE_BYTES

        key = b"marco_toooooo_large"
        kbuf_view[: len(key)] = memoryview(key)
        vbuf_view[:] = np.arange(buffer_size)

        kvs_store(dev, kbuf, 16, vbuf, buffer_size)
        with pytest.raises(KVException) as assertion:
            kvs_store(dev, kbuf, len(key), vbuf, buffer_size)
        assert assertion.value.status_code_type == xnvme.XNVME_STATUS_CODE_TYPE_CMDSPEC
        assert assertion.value.status_code == xnvme.XNVME_SPEC_KV_SC_INVALID_KEY_SIZE

        kvs_retrieve(dev, kbuf, 16, vbuf, buffer_size)
        with pytest.raises(KVException) as assertion:
            kvs_retrieve(dev, kbuf, len(key), vbuf, buffer_size)
        assert assertion.value.status_code_type == xnvme.XNVME_STATUS_CODE_TYPE_GENERIC
        assert assertion.value.status_code == xnvme.XNVME_STATUS_CODE_INVALID_FIELD

        kvs_exist(dev, kbuf, 16)
        with pytest.raises(KVException) as assertion:
            kvs_exist(dev, kbuf, len(key))
        assert assertion.value.status_code_type == xnvme.XNVME_STATUS_CODE_TYPE_GENERIC
        assert assertion.value.status_code == xnvme.XNVME_STATUS_CODE_INVALID_FIELD

        kvs_delete(dev, kbuf, 16)
        with pytest.raises(KVException) as assertion:
            kvs_delete(dev, kbuf, len(key))
        assert assertion.value.status_code_type == xnvme.XNVME_STATUS_CODE_TYPE_GENERIC
        assert assertion.value.status_code == xnvme.XNVME_STATUS_CODE_INVALID_FIELD

        kvs_list(dev, kbuf, 16, vbuf, buffer_size)
        with pytest.raises(KVException) as assertion:
            kvs_list(dev, kbuf, len(key), vbuf, buffer_size)
        assert assertion.value.status_code_type == xnvme.XNVME_STATUS_CODE_TYPE_GENERIC
        assert assertion.value.status_code == xnvme.XNVME_STATUS_CODE_INVALID_FIELD

        xnvme.xnvme_buf_free(dev, vbuf)
        xnvme.xnvme_buf_free(dev, kbuf)
        xnvme.xnvme_dev_close(dev)
