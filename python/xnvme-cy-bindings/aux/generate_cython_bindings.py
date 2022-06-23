from io import StringIO

from . import autopxd_py


def generate_pyx(pxd_contents):
    pyx_contents = StringIO()
    pyx_contents.write(
        """
ctypedef int off_t

ctypedef bint bool
"""
    )

    # Gen CPython interface from .pxd file.
    pxd_contents.seek(0)
    definitions = list(autopxd_py.get_definitions(pxd_contents))

    def filter_empty(definitions):
        _defs = {}
        for _type, *args in definitions:
            if _type == "func":
                ret_type, name, __name, members = args
                _defs[__name] = (_type, ret_type, name, __name, members)
                continue

            name, __name, members = args
            if __name not in _defs:
                _defs[__name] = (_type, name, __name, members)
            else:
                _, _, _, _members = _defs[__name]
                if len(_members) == 0:
                    # Prefer definitions with any members
                    _defs[__name] = (_type, name, __name, members)
        return _defs

    definitions = filter_empty(definitions)

    autopxd_py.gen_code(
        pyx_contents,
        list(definitions.values()),
        preamble=True,
        lib_prefix="libxnvme",
    )

    pyx_contents.seek(0)
    return pyx_contents
