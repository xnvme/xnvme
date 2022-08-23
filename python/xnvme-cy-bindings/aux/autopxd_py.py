import re

from .autopxd_py_templates import (
    BLOCK_OPAQUE_TEMPLATE,
    BLOCK_TEMPLATE,
    GETTER_TEMPLATE,
    GETTER_TEMPLATE_SAFE_STR,
    GETTER_TEMPLATE_STRUCT_POINTER,
    GETTER_TEMPLATE_STRUCT_POINTER_ADDRESSOF,
    PREAMBLE,
    SETTER_TEMPLATE,
    SETTER_TEMPLATE_STRUCT_POINTER,
)


def get_definitions(pxd_f):
    data = pxd_f.read()

    member_regex = r"(?::\n\s+([_a-zA-Z0-9\* \[\]]+(?: \"([_a-zA-Z0-9]+)\")?\n)+|\n)"
    for struct in re.finditer(
        r'(cdef struct ([_a-zA-Z0-9]+)(?: "([_a-zA-Z0-9]+)")?' + member_regex + ")",
        data,
    ):
        _, __struct_name, struct_name, _, _ = struct.groups()
        struct = struct.groups()[0]
        members = []
        for member in re.finditer(
            r"(\s+([_a-zA-Z0-9]+\*?) ([_a-zA-Z0-9\[\]]+)( \".*\")?\n)", struct
        ):
            _, _type, member_name, _ = member.groups()
            if _type != "struct" and member_name != struct_name:
                # Hack in case of empty struct declarations
                members.append((_type, member_name))
        yield ("struct", struct_name, __struct_name, members)

    for union in re.finditer(
        r'(cdef union ([_a-zA-Z0-9]+)(?: "([_a-zA-Z0-9]+)")?' + member_regex + ")",
        data,
    ):
        _, __union_name, union_name, _, _ = union.groups()
        union = union.groups()[0]
        members = []
        for member in re.finditer(
            r"(\s+([_a-zA-Z0-9]+\*?) ([_a-zA-Z0-9\[\]]+)( \".*\")?\n)", union
        ):
            _, _type, member_name, _ = member.groups()
            members.append((_type, member_name))
        yield ("union", union_name, __union_name, members)

    for func in re.finditer(
        r'([_a-zA-Z0-9\*]+) ([_a-zA-Z0-9]+)(?: "([_a-zA-Z0-9]+)")?(\(.*\))', data
    ):
        _type, __func_name, func_name, arg_body = func.groups()

        args = []
        for arg in re.finditer(
            r"([_a-zA-Z0-9\*]+) ([_a-zA-Z0-9]+)(?:, )?", arg_body.strip("()")
        ):
            arg_type, arg_name = arg.groups()
            args.append((arg_type, arg_name))

        yield ("func", _type, func_name, __func_name, args)

    for enum in re.finditer(
        r'(cpdef enum ([_a-zA-Z0-9]+)(?: "([_a-zA-Z0-9]+)")?):\n', data
    ):
        _, __enum_name, enum_name = enum.groups()
        members = []
        yield ("enum", enum_name, __enum_name, members)

    for typedef in re.finditer(
        r'(ctypedef ([_a-zA-Z0-9\*]+) \(([_a-zA-Z0-9\*]+)\)(?: "([_a-zA-Z0-9]+)")?.*)\n',
        data,
    ):
        _, _ret_type, typedef_name, __typedef_name = typedef.groups()
        yield ("typedef", __typedef_name or "", typedef_name.replace("*", ""), None)


def gen_code(
    pyx_f, definitions, preamble=True, ignore_definitions=set(), lib_prefix=""
):
    enum_defs = [args[1] for t, *args in definitions if t == "enum"]
    if preamble:
        pyx_f.write(
            f"""
# Enums:
# from libxnvme import {', '.join(enum_defs)}
from libxnvme cimport {', '.join(enum_defs)}
"""
        )
        pyx_f.write(PREAMBLE)

    for _type, *args in definitions:
        if args[-2] in ignore_definitions:
            continue

        if _type == "struct" or _type == "union":
            _, block_name, members = args

            ignore_list = [
                "xnvme_spec_vs_register",  # Requires investigation of unions (_self_cast_void_p failing)
                # '_xnvme_spec_vs_register_bits_s',
                "xnvmec_sub",
                "xnvmec",
            ]
            if (
                block_name in ignore_list
                or block_name.startswith("_xnvmec_")
                or block_name.startswith("_xnvme_")
            ):  # Ignores and auto-gen'ed anon struct
                continue

            # if "[" not in n and # TODO: Arrays are not supported yet (PyMemoryView_FromMemory(char *mem, Py_ssize_t size, int flags))
            # (t.startswith("xnvme_")) and t.endswith("*")
            filtered_members = [
                (t, n)
                for t, n in members
                if not (  # TODO: We can't get/set these structs yet (wrap/unwrap from xnvme python class)
                    "[" in t
                    or "[" in n
                    or t
                    == "xnvme_queue_cb"  # TODO: Cannot assign to a callback function pointer atm.
                    or t
                    == "void*"  # TODO: Cannot assign to void pointer atm. (wrap/unwrap from xnvme_void_p python class)
                    # or t = "xnvme_geo_type"
                    or t == "xnvme_spec_vs_register"  # TODO: Support enum types
                    # or "xnvme_be_attr" in t  # TODO: Support union types
                )  # TODO: xnvme_be_attr_list has unsupported empty array length
            ]
            fields = ", ".join(f'"{n}"' for t, n in filtered_members)

            opaque_struct = filtered_members == []

            if opaque_struct:
                pyx_f.write(
                    BLOCK_OPAQUE_TEMPLATE.format(
                        block_name=block_name, lib_prefix=lib_prefix
                    )
                )
                continue

            def struct_to_class_name(t):
                return t.replace("*", "")

            def pick_setter(t, n):
                if t.startswith("_xnvmec_"):
                    return (
                        " " * 8
                        + f'# setters for "_xnvmec_" auto-gen structs not implemented {t}: {n}'
                    )
                elif t.startswith("_xnvme_"):
                    return (
                        " " * 8
                        + f'# setters for "_xnvme_" auto-gen structs not implemented {t}: {n}'
                    )
                elif t.startswith("xnvme_"):
                    if t.endswith("*"):
                        return SETTER_TEMPLATE_STRUCT_POINTER.format(
                            lib_prefix=lib_prefix,
                            member_name=n,
                            member_type=struct_to_class_name(t),
                            __member_type=t,
                        )
                    else:
                        return f"        # TODO: Direct assignment not implemented {t}: {n}"
                else:
                    return SETTER_TEMPLATE.format(member_name=n)

            setters = "\n".join(pick_setter(t, n) for t, n in filtered_members)

            def pick_getter(t, n):
                if t == "char*":
                    return GETTER_TEMPLATE_SAFE_STR.format(member_name=n)
                elif t.startswith("_xnvmec_"):
                    return (
                        " " * 8
                        + f'# getters for "_xnvmec_" auto-gen structs not implemented {t}: {n}'
                    )
                elif t.startswith("_xnvme_"):
                    return (
                        " " * 8
                        + f'# getters for "_xnvme_" auto-gen structs not implemented {t}: {n}'
                    )
                elif t.startswith("xnvme_"):
                    if t.endswith("*"):
                        return GETTER_TEMPLATE_STRUCT_POINTER.format(
                            member_name=n, member_type=struct_to_class_name(t)
                        )
                    else:
                        if t == "xnvme_geo_type":
                            return f"        # TODO: Detect and support enum types {t}: {n}"
                        return GETTER_TEMPLATE_STRUCT_POINTER_ADDRESSOF.format(
                            member_type=t, member_name=n
                        )
                else:
                    return GETTER_TEMPLATE.format(member_name=n)

            getters = "\n".join(pick_getter(t, n) for t, n in filtered_members)

            pyx_f.write(
                BLOCK_TEMPLATE.format(
                    block_name=block_name,
                    lib_prefix=lib_prefix,
                    fields=fields,
                    getters=getters,
                    setters=setters,
                )
            )
        elif _type == "func":
            ret_type, _, func_name, func_args = args

            ignore_list = [
                "xnvme_be_attr_list_bundled",  # Requires investigation of pointer-pointer (xnvme_be_attr_list**)
                "xnvme_queue_init",  # Requires investigation of pointer-pointer (xnvme_queue **)
                "xnvme_buf_phys_alloc",  # Requires investigation of phys pointer (uint64_t* phys)
                "xnvme_buf_phys_realloc",  # Requires investigation of phys pointer (uint64_t* phys)
                "xnvme_buf_phys_free",  # Requires investigation of phys pointer (uint64_t* phys)
                "xnvme_buf_vtophys",  # Requires investigation of phys pointer (uint64_t* phys)
                "xnvme_cmd_ctx_from_dev",  # Manually handled
                "xnvme_enumerate",  # Manually handled
                "xnvme_cmd_ctx_set_cb",  # Manually handled
                "xnvme_queue_set_cb",  # Manually handled
                "xnvme_lba_fpr",  # uint64_t* supported yet
                "xnvme_lba_pr",  # uint64_t* supported yet
                "xnvme_lba_fprn",  # uint64_t* supported yet
                "xnvme_lba_prn",  # uint64_t* supported yet
                "xnvme_enumeration_alloc",  # Cannot assign type 'xnvme_enumeration *' to 'xnvme_enumeration **'
                "xnvmec_get_opt_attr",
                "xnvmec",
                "xnvmec_sub",
                "xnvmec_subfunc",
                "xnvmec_timer_start",
                "xnvmec_timer_stop",
                "xnvmec_timer_bw_pr",
                "xnvmec_cli_to_opts",
                "_xnvmec_cmd_from_file",
                "xnvmec_cmd_from_file",
                "xnvme_spec_feat_fpr",  # Takes struct (not pointer) as argument
                "xnvme_spec_feat_pr",  # Takes struct (not pointer) as argument
                "xnvme_spec_drecv_sar_pr",  # Takes struct (not pointer) as argument
                "xnvmec_cmd_to_file",  # Somehow causes linking issues
            ]
            if func_name in ignore_list:
                continue

            _py_func_args = []
            for t, n in func_args:
                if t in ["void*", "uint64_t*"]:
                    statement = f"xnvme_void_p {n}"
                elif t in ["FILE*"]:
                    statement = f"FILE {n}"
                elif t.startswith("xnvme_") or t.startswith("xnvmec_"):
                    statement = f"{t.replace('*','')} {n}"
                else:
                    statement = f"{t} {n}"
                _py_func_args.append(statement)
            py_func_args = ", ".join(_py_func_args)
            c_func_args = ", ".join(
                # n + (".pointer" if (t[-1] == "*" and t != "char*") else "")
                ("<uint64_t *>" if t == "uint64_t*" else "")
                + n
                + (
                    ".pointer"
                    if (
                        (t.startswith("xnvme_") or t.startswith("xnvmec_"))
                        or t in ["void*", "uint64_t*", "FILE*"]
                    )
                    and t not in enum_defs
                    else ""
                )
                for t, n in func_args
            )
            if ret_type == "void":
                func_template = f"""
def {func_name}({py_func_args}):
    {lib_prefix}.{func_name}({c_func_args})
"""
            else:
                post_process = ""
                if ret_type == "void*":
                    ret_type_def = "xnvme_void_p"
                    assign_def = "ret.pointer"
                    post_process = (
                        f"\n    if <void*> ret.pointer == NULL:"
                        f'\n        raise XNVMeNullPointerException("{func_name} returned a null-pointer")'
                    )
                    init_def = " = xnvme_void_p()"
                elif (
                    ret_type.startswith("xnvme_") or ret_type.startswith("xnvmec_")
                ) and not ret_type.endswith("*"):
                    # Supposed to return a struct directly, not a pointer
                    assign_def = f"cdef {lib_prefix}.{ret_type} _ret"
                    ret_type_def = ret_type
                    post_process = (
                        f"\n    ret.pointer = <{lib_prefix}.{ret_type} *> calloc(1, sizeof({lib_prefix}.{ret_type}))"
                        f"\n    ret.auto_free = True"
                        f"\n    memcpy(<void*> ret.pointer, <void*> &_ret, sizeof({lib_prefix}.{ret_type}))"
                    )
                    init_def = f" = {ret_type_def}()"
                elif ret_type.startswith("xnvme_") or ret_type.startswith("xnvmec_"):
                    ret_type_def = f"{ret_type.replace('*','')}"
                    assign_def = "ret.pointer"
                    post_process = (
                        f"\n    if <void*> ret.pointer == NULL:"
                        f'\n        raise XNVMeNullPointerException("{func_name} returned a null-pointer")'
                    )
                    init_def = f" = {ret_type_def}()"
                else:
                    ret_type_def = ret_type
                    assign_def = "ret"
                    post_process = ""
                    init_def = ""
                func_template = f"""
def {func_name}({py_func_args}):
    cdef {ret_type_def} ret{init_def}
    {assign_def} = {lib_prefix}.{func_name}({c_func_args}){post_process}
    return ret
"""
            pyx_f.write(func_template)
