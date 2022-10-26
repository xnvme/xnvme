import ctypes

import conftest
import numpy as np
import pytest
import xnvme.cython_bindings as xnvme

KEY_SIZE_BYTES = 128 // 8  # 128-bit keys in KV spec


class KVException(Exception):
    def __init__(self, err, status_code, status_code_type, *args, **kwargs):
        self.err = err
        self.status_code = status_code
        self.status_code_type = status_code_type
        super().__init__(*args, **kwargs)


def kvs_store(dev, buf_key, key_length, buf_value, value_length, opts=0):
    ctx = xnvme.xnvme_cmd_ctx_from_dev(dev)
    err = xnvme.xnvme_kvs_store(
        ctx,
        conftest.DEVICE_NSID,
        xnvme.xnvme_void_p(buf_key.void_pointer),
        key_length,
        buf_value,
        value_length,
        opts,
    )
    if err or xnvme.xnvme_cmd_ctx_cpl_status(ctx):
        raise KVException(
            err,
            ctx.cpl.status.sc,
            ctx.cpl.status.sct,
            xnvme.xnvmec_perr(b"xnvme_kvs_store()", err),
        )
    return ctx


def kvs_retrieve(dev, buf_key, key_length, buf_value, value_length, opts=0):
    ctx = xnvme.xnvme_cmd_ctx_from_dev(dev)
    err = xnvme.xnvme_kvs_retrieve(
        ctx,
        conftest.DEVICE_NSID,
        xnvme.xnvme_void_p(buf_key.void_pointer),
        key_length,
        buf_value,
        value_length,
        opts,
    )
    if err or xnvme.xnvme_cmd_ctx_cpl_status(ctx):
        raise KVException(
            err,
            ctx.cpl.status.sc,
            ctx.cpl.status.sct,
            xnvme.xnvmec_perr(b"xnvme_kvs_retrieve()", err),
        )
    return ctx


def kvs_exist(dev, buf_key, key_length):
    ctx = xnvme.xnvme_cmd_ctx_from_dev(dev)
    err = xnvme.xnvme_kvs_exist(
        ctx,
        conftest.DEVICE_NSID,
        xnvme.xnvme_void_p(buf_key.void_pointer),
        key_length,
    )
    if err or xnvme.xnvme_cmd_ctx_cpl_status(ctx):
        raise KVException(
            err,
            ctx.cpl.status.sc,
            ctx.cpl.status.sct,
            xnvme.xnvmec_perr(b"xnvme_kvs_exist()", err),
        )
    return ctx


def kvs_delete(dev, buf_key, key_length):
    ctx = xnvme.xnvme_cmd_ctx_from_dev(dev)
    err = xnvme.xnvme_kvs_delete(
        ctx,
        conftest.DEVICE_NSID,
        xnvme.xnvme_void_p(buf_key.void_pointer),
        key_length,
    )
    if err or xnvme.xnvme_cmd_ctx_cpl_status(ctx):
        raise KVException(
            err,
            ctx.cpl.status.sc,
            ctx.cpl.status.sct,
            xnvme.xnvmec_perr(b"xnvme_kvs_delete()", err),
        )
    return ctx


def kvs_list(dev, buf_key, key_length, buf_value, value_length, opts=0):
    ctx = xnvme.xnvme_cmd_ctx_from_dev(dev)
    err = xnvme.xnvme_kvs_list(
        ctx,
        conftest.DEVICE_NSID,
        xnvme.xnvme_void_p(buf_key.void_pointer),
        key_length,
        buf_value,
        value_length,
    )
    if err or xnvme.xnvme_cmd_ctx_cpl_status(ctx):
        raise KVException(
            err,
            ctx.cpl.status.sc,
            ctx.cpl.status.sct,
            xnvme.xnvmec_perr(b"xnvme_kvs_list()", err),
        )
    return ctx


class TestKeyValue:
    def test_single_key(self, dev_kv, autofreed_buffer):
        buf, buf_memview = autofreed_buffer(dev_kv, 4096)
        buf_key, buf_key_memview = autofreed_buffer(dev_kv, KEY_SIZE_BYTES)

        key = b"marco"
        value = b"polo"

        buf_memview[: len(value)] = memoryview(value)
        buf_key_memview[: len(key)] = memoryview(key)

        kvs_store(dev_kv, buf_key, len(key), buf, len(value))

        buf_memview[:] = 0  # Zero to be certain we are reading correctly

        kvs_retrieve(dev_kv, buf_key, len(key), buf, len(value))

        assert np.all(
            buf_memview[: len(value)] == memoryview(value)
        ), "Retreiving returned different value"

    def test_delete_exist(self, dev_kv, autofreed_buffer):
        buf, buf_memview = autofreed_buffer(dev_kv, 4096)
        buf_key, buf_key_memview = autofreed_buffer(dev_kv, KEY_SIZE_BYTES)

        key = b"deleteme"
        value = b"x" * 128

        buf_memview[: len(value)] = memoryview(value)
        buf_key_memview[: len(key)] = memoryview(key)

        # Store a key we can delete
        kvs_store(dev_kv, buf_key, len(key), buf, len(value))

        # Before we delete, we verify the key is there.
        kvs_exist(dev_kv, buf_key, len(key))

        # Delete the key
        kvs_delete(dev_kv, buf_key, len(key))

        # Reading should now fail
        with pytest.raises(KVException) as assertion:
            kvs_exist(dev_kv, buf_key, len(key))
        assert assertion.value.status_code_type == xnvme.XNVME_STATUS_CODE_TYPE_CMDSPEC
        assert assertion.value.status_code == xnvme.XNVME_SPEC_KV_SC_KEY_NOT_EXISTS

    def test_compress(self, dev_kv, autofreed_buffer):
        """
        Bit 10 if set to '1' specifies that the controller shall not compress the KV value. Bit 10 if cleared to '0'
        specifies that the controller shall compress the KV value if compression is supported.
        """
        buf, buf_memview = autofreed_buffer(dev_kv, 4096)
        buf_key, buf_key_memview = autofreed_buffer(dev_kv, KEY_SIZE_BYTES)

        key = b"compressme"
        value = b"x" * 128

        buf_memview[: len(value)] = memoryview(value)
        buf_key_memview[: len(key)] = memoryview(key)

        kvs_store(
            dev_kv,
            buf_key,
            len(key),
            buf,
            len(value),
            opts=xnvme.XNVME_KVS_STORE_OPT_COMPRESS,
        )

    def test_dont_store_if_key_exists(self, dev_kv, autofreed_buffer):
        """
        Bit 9 if set to '1' specifies that the controller shall not store the KV value if the KV key exists. Bit 9 if
        cleared to '0' specifies that the controller shall store the KV value if other Store Options are met.
        """
        buf, buf_memview = autofreed_buffer(dev_kv, 4096)
        buf_key, buf_key_memview = autofreed_buffer(dev_kv, KEY_SIZE_BYTES)

        key = b"Ipreexist"
        value = b"x" * 128

        buf_memview[: len(value)] = memoryview(value)
        buf_key_memview[: len(key)] = memoryview(key)

        kvs_store(dev_kv, buf_key, len(key), buf, len(value))  # Make the key preexist

        # Fail to replace existing key
        with pytest.raises(KVException) as assertion:
            kvs_store(
                dev_kv,
                buf_key,
                len(key),
                buf,
                len(value),
                opts=xnvme.XNVME_KVS_STORE_OPT_DONT_STORE_IF_KEY_EXISTS,
            )
        assert assertion.value.status_code_type == xnvme.XNVME_STATUS_CODE_TYPE_CMDSPEC
        assert assertion.value.status_code == xnvme.XNVME_SPEC_KV_SC_KEY_EXISTS

        # Deleting the key first should now make it work
        kvs_delete(dev_kv, buf_key, len(key))
        kvs_store(
            dev_kv,
            buf_key,
            len(key),
            buf,
            len(value),
            opts=xnvme.XNVME_KVS_STORE_OPT_DONT_STORE_IF_KEY_EXISTS,
        )

    def test_dont_store_if_key_doesnt_exist(self, dev_kv, autofreed_buffer):
        """
        Bit 8 if set to '1' specifies that the controller shall not store the KV value if the KV key does not exists.
        Bit 8 if cleared to '0' specifies that the controller shall store the KV value if other Store Options are met.
        """
        buf, buf_memview = autofreed_buffer(dev_kv, 4096)
        buf_key, buf_key_memview = autofreed_buffer(dev_kv, KEY_SIZE_BYTES)

        key = b"Ipreexist"
        value = b"x" * 128

        buf_memview[: len(value)] = memoryview(value)
        buf_key_memview[: len(key)] = memoryview(key)

        kvs_store(dev_kv, buf_key, len(key), buf, len(value))  # Make the key preexist

        # Only allowed to replace existing key
        kvs_store(
            dev_kv,
            buf_key,
            len(key),
            buf,
            len(value),
            opts=xnvme.XNVME_KVS_STORE_OPT_DONT_STORE_IF_KEY_NOT_EXISTS,
        )

        # Deleting the key first should now make it fail
        kvs_delete(dev_kv, buf_key, len(key))
        with pytest.raises(KVException) as assertion:
            kvs_store(
                dev_kv,
                buf_key,
                len(key),
                buf,
                len(value),
                opts=xnvme.XNVME_KVS_STORE_OPT_DONT_STORE_IF_KEY_NOT_EXISTS,
            )
        assert assertion.value.status_code_type == xnvme.XNVME_STATUS_CODE_TYPE_CMDSPEC
        assert assertion.value.status_code == xnvme.XNVME_SPEC_KV_SC_KEY_NOT_EXISTS

    def test_multiple_keys(self, dev_kv, autofreed_buffer):
        buf, buf_memview = autofreed_buffer(dev_kv, 4096)
        buf_key, buf_key_memview = autofreed_buffer(dev_kv, KEY_SIZE_BYTES)

        def kv_for_n(n):
            key = f"marco_{n}".encode()
            value = f"polo_{n}".encode()
            return key, value

        # Store multiple values
        for n in range(128):
            key, value = kv_for_n(n)
            buf_memview[: len(value)] = memoryview(value)
            buf_key_memview[: len(key)] = memoryview(key)

            kvs_store(dev_kv, buf_key, len(key), buf, len(value))

        # Retreive all values
        for n in range(128):
            buf_memview[:] = 0  # Zero to be certain we are reading correctly

            key, value = kv_for_n(n)
            buf_key_memview[: len(key)] = memoryview(key)

            kvs_retrieve(dev_kv, buf_key, len(key), buf, len(value))

            assert np.all(
                buf_memview[: len(value)] == memoryview(value)
            ), "Retreiving returned different value"

    def test_list(self, dev_kv, autofreed_buffer):
        buffer_size = 4096
        buf, buf_memview = autofreed_buffer(dev_kv, buffer_size)
        buf_key, buf_key_memview = autofreed_buffer(dev_kv, KEY_SIZE_BYTES)

        def kv_for_n(n):
            key = f"list_{n}".encode()
            value = f"polo_{n}".encode()
            return key, value

        # Store multiple values
        for n in range(128):
            key, value = kv_for_n(n)
            buf_memview[: len(value)] = memoryview(value)
            buf_key_memview[: len(key)] = memoryview(key)

            kvs_store(dev_kv, buf_key, len(key), buf, len(value))

        def unpack_list_data(list_data):
            num_keys = int(
                list_data[:4].view(np.uint32)
            )  # First 32 bits define the number of keys
            key_data = list_data[4:]  # The initial key data
            read_keys = []
            for _ in range(num_keys):
                # Extract key length from data
                key_length = int(key_data[:2].view(np.uint16))
                assert key_length <= 16, "Invalid key length"

                # Extract key name from data
                key = bytes(key_data[2 : 2 + key_length])
                read_keys.append(key)

                # Offset the buffer for next iteration
                padding = (
                    2 + key_length
                ) % 4  # "[...] end the data structure on a 4 byte boundary"
                key_data = key_data[
                    2 + key_length + padding :
                ]  # The remaining key data
            return read_keys

        # Finding all keys that can fit in the buffer
        buf_memview[:] = 0  # Zero to be certain we are reading correctly
        buf_key_memview[:] = 0  # Clear key to get what the device considers "first"

        kvs_list(dev_kv, buf_key, len(key), buf, buffer_size)
        read_keys = unpack_list_data(buf_memview)
        assert (
            len(read_keys) >= 128
        )  # We just created 128, so there should be at least that

        # Fetch to small buffer
        buf_memview[:] = 0  # Zero to be certain we are reading correctly
        buf_key_memview[:] = 0  # Clear key to get what the device considers "first"

        kvs_list(dev_kv, buf_key, len(key), buf, 128)
        read_keys = unpack_list_data(buf_memview)
        assert len(read_keys) < 128  # We should have significantly less than 128 now

        # Find the specific range of keys we just created
        buf_memview[:] = 0  # Zero to be certain we are reading correctly
        buf_key_memview[:] = 0  # Clear key to fill a new one
        key = b"list_0"
        buf_key_memview[: len(key)] = memoryview(key)

        kvs_list(dev_kv, buf_key, len(key), buf, buffer_size)
        read_keys = unpack_list_data(buf_memview)
        assert len(read_keys) >= 128  # Assert we found the amount of keys we created
        assert all(x == f"list_{i}".encode() for i, x in enumerate(read_keys[:128]))

        # Continue from a specific key
        buf_memview[:] = 0  # Zero to be certain we are reading correctly
        buf_key_memview[:] = 0  # Clear key to fill a new one
        key = b"list_64"
        buf_key_memview[: len(key)] = memoryview(key)

        kvs_list(dev_kv, buf_key, len(key), buf, buffer_size)
        read_keys = unpack_list_data(buf_memview)
        assert len(read_keys) >= 64  # Assert we found the amount of keys we created
        assert all(x == f"list_{i+64}".encode() for i, x in enumerate(read_keys[:64]))

    @pytest.mark.parametrize(
        "expected_sc, buffer_size",
        [
            (0, 4096),
            (xnvme.XNVME_SPEC_KV_SC_INVALID_VAL_SIZE, 4096 + 1),  # QEMU limitation
        ],
    )
    def test_large_value(self, dev_kv, autofreed_buffer, buffer_size, expected_sc):
        buf, buf_memview = autofreed_buffer(dev_kv, buffer_size)
        buf_key, buf_key_memview = autofreed_buffer(dev_kv, KEY_SIZE_BYTES)

        key = b"marco_large"
        buf_key_memview[: len(key)] = memoryview(key)
        buf_memview[:] = np.arange(buffer_size)

        if expected_sc != 0:
            with pytest.raises(KVException) as assertion:
                kvs_store(dev_kv, buf_key, len(key), buf, buffer_size)
            assert assertion.value.status_code == expected_sc
        else:
            kvs_store(dev_kv, buf_key, len(key), buf, buffer_size)

    def test_large_key(self, dev_kv, autofreed_buffer):
        buffer_size = 128
        buf, buf_memview = autofreed_buffer(dev_kv, buffer_size)
        buf_key, buf_key_memview = autofreed_buffer(
            dev_kv, 32
        )  # On purpose larger than KEY_SIZE_BYTES

        key = b"marco_toooooo_large"
        buf_key_memview[: len(key)] = memoryview(key)
        buf_memview[:] = np.arange(buffer_size)

        kvs_store(dev_kv, buf_key, 16, buf, buffer_size)
        with pytest.raises(KVException) as assertion:
            kvs_store(dev_kv, buf_key, len(key), buf, buffer_size)
        assert assertion.value.status_code_type == xnvme.XNVME_STATUS_CODE_TYPE_CMDSPEC
        assert assertion.value.status_code == xnvme.XNVME_SPEC_KV_SC_INVALID_KEY_SIZE

        kvs_retrieve(dev_kv, buf_key, 16, buf, buffer_size)
        with pytest.raises(KVException) as assertion:
            kvs_retrieve(dev_kv, buf_key, len(key), buf, buffer_size)
        assert assertion.value.status_code_type == xnvme.XNVME_STATUS_CODE_TYPE_GENERIC
        assert assertion.value.status_code == xnvme.XNVME_STATUS_CODE_INVALID_FIELD

        kvs_exist(dev_kv, buf_key, 16)
        with pytest.raises(KVException) as assertion:
            kvs_exist(dev_kv, buf_key, len(key))
        assert assertion.value.status_code_type == xnvme.XNVME_STATUS_CODE_TYPE_GENERIC
        assert assertion.value.status_code == xnvme.XNVME_STATUS_CODE_INVALID_FIELD

        kvs_delete(dev_kv, buf_key, 16)
        with pytest.raises(KVException) as assertion:
            kvs_delete(dev_kv, buf_key, len(key))
        assert assertion.value.status_code_type == xnvme.XNVME_STATUS_CODE_TYPE_GENERIC
        assert assertion.value.status_code == xnvme.XNVME_STATUS_CODE_INVALID_FIELD

        kvs_list(dev_kv, buf_key, 16, buf, buffer_size)
        with pytest.raises(KVException) as assertion:
            kvs_list(dev_kv, buf_key, len(key), buf, buffer_size)
        assert assertion.value.status_code_type == xnvme.XNVME_STATUS_CODE_TYPE_GENERIC
        assert assertion.value.status_code == xnvme.XNVME_STATUS_CODE_INVALID_FIELD
