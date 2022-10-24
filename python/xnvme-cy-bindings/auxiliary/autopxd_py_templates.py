PREAMBLE = """
import cython
from cython.operator cimport dereference
from libc.string cimport memcpy
from libc.stdlib cimport calloc, free
from libc.stdint cimport uintptr_t
from cpython cimport memoryview

from libc.stdint cimport uint16_t, uint32_t, int8_t, uint64_t, int64_t, uint8_t
cimport libxnvme

cdef class xnvme_base:
    # cdef void *pointer
    cdef bint auto_free
    cdef dict ref_counting

    def __init__(self, __void_p=None, **kwargs):
        self.auto_free = False
        self.ref_counting = {}

        if __void_p:
            self._self_cast_void_p(__void_p)
        elif kwargs:
            self._self_alloc()
            self.auto_free = True
            for k,v in kwargs.items():
                self.__setattr__(k,v)

    cdef _safe_str(self, char* string):
        if <void *> string == NULL:
            return None
        return string

    def to_dict(self):
        return {x: self.__getattr__(x) for x in self.fields}

    def __del__(self):
        if self.pointer and self.auto_free:
            self._self_dealloc()

    # def void_pointer(self):
    #     return <uintptr_t> self.pointer

cdef class xnvme_void_p(xnvme_base):
    cdef void *pointer

    def __init__(self, pointer=None):
        if pointer:
            self._set_void_pointer(pointer)

    def _set_void_pointer(self, uintptr_t void_p):
        self.pointer = <void *> void_p

    def __getattr__(self, attr_name):
        if attr_name == 'void_pointer':
            return <uintptr_t> self.pointer
        if attr_name == 'pointer':
            return <uintptr_t> self.pointer
        return super().__getattr__(attr_name)

from libc cimport stdio

cdef class FILE(xnvme_base):
    cdef stdio.FILE *pointer

    def __init__(self):
        pass

    def fopen(self, filename, opentype):
        self.pointer = stdio.fopen(filename, opentype)

    def fdopen(self, fd, opentype):
        self.pointer = stdio.fdopen(fd, opentype)

    def fclose(self):
        return stdio.fclose(self.pointer)

    def tmpfile(self):
        self.pointer = stdio.tmpfile()

    def fflush(self):
        return stdio.fflush(self.pointer)

    def getvalue(self, seek_pos=0):
        offset = stdio.ftell(self.pointer) # Save to reset later
        stdio.fseek(self.pointer, seek_pos, stdio.SEEK_SET) # Beginning of file-stream

        ret_string = b''
        cdef char* data = <char*> calloc(1, 4096)
        while True:
            ret = stdio.fread(<void*> data, 1, 4096, self.pointer)
            if ret == stdio.EOF or ret == 0:
                break
            ret_string += data[:ret]
        free(data)

        stdio.fseek(self.pointer, offset, stdio.SEEK_SET) # Reset file-stream position
        return ret_string


class XNVMeException(Exception):
    pass

class XNVMeNullPointerException(XNVMeException):
    pass

class StructGetterSetter:
    def __init__(self, obj, prefix):
        self.obj = obj
        self.prefix = prefix

    def __getattr__(self, name):
        if hasattr(self.obj, self.prefix+name):
            return getattr(self.obj, self.prefix+name)
        else:
            return StructGetterSetter(self.obj, self.prefix+name+'__')

    def __setattr__(self, name, value):
        if name in ['obj', 'prefix']:
            super().__setattr__(name, value)
        else:
            return setattr(self.obj, self.prefix+name, value)

# NOTE: This is the only function returning a struct directly instead of a pointer. Handled manually.
def xnvme_cmd_ctx_from_dev(xnvme_dev dev):
    cdef libxnvme.xnvme_cmd_ctx ctx = libxnvme.xnvme_cmd_ctx_from_dev(dev.pointer)
    cdef libxnvme.xnvme_cmd_ctx* ctx_p = <libxnvme.xnvme_cmd_ctx *> calloc(1, sizeof(libxnvme.xnvme_cmd_ctx))
    memcpy(<void*> ctx_p, <void*> &ctx, sizeof(libxnvme.xnvme_cmd_ctx))
    cdef xnvme_cmd_ctx ret = xnvme_cmd_ctx()
    ret.pointer = ctx_p
    return ret

# NOTE: Callback functions require some hacking, that is not worth automating
# Handler for typedef: ctypedef int (*libxnvme.xnvme_enumerate_cb "xnvme_enumerate_cb")(libxnvme.xnvme_dev* dev, void* cb_args)
cdef int xnvme_enumerate_python_callback_handler(libxnvme.xnvme_dev* dev, void* cb_args):
    (py_func, py_cb_args) = <object> cb_args
    cdef xnvme_dev py_dev = xnvme_dev()
    py_dev.pointer = dev
    return py_func(py_dev, py_cb_args)

def xnvme_enumerate(sys_uri, xnvme_opts opts, object cb_func, object cb_args):
    # Given we force the context to be a Python object, we are free to wrap it in a Python-tuple and tag our python
    # callback-function along.
    cb_args_tuple = (cb_func, cb_args)
    cdef void* cb_args_context = <void*>cb_args_tuple

    # sys_uri has a special meaning on NULL, so we translate None->NULL and otherwise pass a string along
    cdef char* _sys_uri
    if sys_uri is None:
        _sys_uri = NULL
    else:
        _sys_uri = <char*> sys_uri

    return libxnvme.xnvme_enumerate(_sys_uri, opts.pointer, xnvme_enumerate_python_callback_handler, cb_args_context)


# NOTE: Callback functions require some hacking, that is not worth automating
# Handler for typedef: ctypedef void (*libxnvme.xnvme_queue_cb "xnvme_queue_cb")(libxnvme.xnvme_cmd_ctx* ctx, void* opaque)
cdef void xnvme_queue_cb_python_callback_handler(libxnvme.xnvme_cmd_ctx* dev, void* cb_args):
    (py_func, py_cb_args) = <object> cb_args
    cdef xnvme_cmd_ctx py_ctx = xnvme_cmd_ctx()
    py_ctx.pointer = dev
    py_func(py_ctx, py_cb_args)

def xnvme_cmd_ctx_set_cb(xnvme_cmd_ctx ctx, object cb, object cb_arg):
    # Given we force the context to be a Python object, we are free to wrap it in a Python-tuple and tag our python
    # callback-function along.
    cb_args_tuple = (cb, cb_arg)
    cdef void* cb_args_context = <void*>cb_args_tuple

    libxnvme.xnvme_cmd_ctx_set_cb(ctx.pointer, xnvme_queue_cb_python_callback_handler, cb_args_context)

def xnvme_queue_set_cb(xnvme_queue queue, object cb, object cb_arg):
    # Given we force the context to be a Python object, we are free to wrap it in a Python-tuple and tag our python
    # callback-function along.
    cb_args_tuple = (cb, cb_arg)
    cdef void* cb_args_context = <void*>cb_args_tuple

    cdef int ret
    ret = libxnvme.xnvme_queue_set_cb(queue.pointer, xnvme_queue_cb_python_callback_handler, cb_args_context)
    return ret

"""

BLOCK_OPAQUE_TEMPLATE = """
cdef class {block_name}(xnvme_base):
    cdef {lib_prefix}.{block_name} *pointer

    def __init__(self, __void_p=None):
        if __void_p:
            self._self_cast_void_p(__void_p)

    def _self_cast_void_p(self, void_p):
        self.pointer = <{lib_prefix}.{block_name} *> (<uintptr_t> void_p.pointer)

    def __getattr__(self, attr_name):
        if attr_name == 'void_pointer':
            return <uintptr_t> self.pointer
"""

SETTER_TEMPLATE = """
        if attr_name == '{member_name}':
            self.pointer.{member_name} = value
            return"""

SETTER_TEMPLATE_STRUCT_POINTER = """
        if attr_name == '{member_name}':
            assert isinstance(value, {member_type})
            self.pointer.{member_name} = <{lib_prefix}.{__member_type}> value.void_pointer
            return"""

GETTER_TEMPLATE = """
        if attr_name == '{member_name}':
            return self.pointer.{member_name}"""

GETTER_TEMPLATE_SAFE_STR = """
        if attr_name == '{member_name}':
            return self._safe_str(self.pointer.{member_name})"""

GETTER_TEMPLATE_STRUCT_POINTER = """
        if attr_name == '{member_name}':
            return {member_type}(__void_p=xnvme_void_p(<uintptr_t>self.pointer.{member_name}))"""

GETTER_TEMPLATE_STRUCT_POINTER_ADDRESSOF = """
        if attr_name == '{member_name}':
            return {member_type}(__void_p=xnvme_void_p(<uintptr_t>&self.pointer.{member_name}))"""

GETTER_TEMPLATE_ARRAY_NUMPY = """
        if attr_name == '{member_name}':
            return np.ctypeslib.as_array(
                ctypes.cast(<uintptr_t>self.pointer.{member_name},
                ctypes.POINTER(ctypes.c_uint8)),shape=({member_length},))"""

GETTER_TEMPLATE_ARRAY_XNVME = """
        if attr_name == '{member_name}':
            return StructIndexer(self, '{member_name}__', {member_length})
            return {member_type}(__void_p=xnvme_void_p(<uintptr_t>self.pointer.{member_name}))"""

STRUCT_GETTER_TEMPLATE = """
        if attr_name == '{member_name}':
            return StructGetterSetter(self, '{member_name}__')"""

BLOCK_TEMPLATE = """
cdef class {block_name}(xnvme_base):
    cdef {lib_prefix}.{block_name} *pointer
    fields = [{fields}]

    def _self_cast_void_p(self, void_p):
        self.pointer = <{lib_prefix}.{block_name} *> (<uintptr_t> void_p.pointer)

    def _self_alloc(self):
        self.pointer = <{lib_prefix}.{block_name} *> calloc(1, sizeof({lib_prefix}.{block_name}))

    def _self_dealloc(self):
        free(self.pointer)

    def __setattr__(self, attr_name, value):
        if <void *> self.pointer == NULL:
            raise AttributeError('Internal pointer is not initialized. Use _self_alloc() or supply some attribute to the constructor when instantiating this object.')

        if not isinstance(value, (int, float)):
            self.ref_counting[attr_name] = value
{setters}
        raise AttributeError(f'{{self}} has no attribute {{attr_name}}')

    def __getattr__(self, attr_name):
        if <void *> self.pointer == NULL:
            raise AttributeError('Internal pointer is not initialized. Use _self_alloc() or supply some attribute to the constructor when instantiating this object.')
        if attr_name == 'sizeof':
            return sizeof({lib_prefix}.{block_name})
        if attr_name == 'void_pointer':
            return <uintptr_t> self.pointer
{getters}
        raise AttributeError(f'{{self}} has no attribute {{attr_name}}')
"""

FUNC_VOID_TEMPLATE = """
def {func_name}({py_func_args}):
    {lib_prefix}.{func_name}({c_func_args})
"""

FUNC_TEMPLATE = """
def {func_name}({py_func_args}):
    cdef {ret_type_def} ret{init_def}
    {assign_def} = {lib_prefix}.{func_name}({c_func_args}){verification}
    return ret
"""
