#!/usr/bin/env python3
"""
Generator of the xNVMe command preps, from senfd's data model of the NVMe
specifications

    python toolbox/xnvme_cmd_prep.py /tmp/prep --spec merged.spec.document.json

yace can run the generator as well, with
--generate toolbox/xnvme_cmd_prep.py:CmdPrep. The build does not run the
generator. A contributor runs the generator and commits the files; see
docs/contributing/contributing-cmd-prep.rst.

For each command, the generator writes a record of the fields of the command,
and a function that prepares a submission queue entry from the record. The
records and functions of each command set are in one header,
libxnvme_cmd_prep_<set>.h, and xnvme_cmd_prep.c holds all the functions:

    struct xnvme_cmd_prep_nvm_read_args args = {.nsid = 1, .slba = 0, .nlb = 7};

    err = xnvme_cmd_prep_nvm_read(&ctx->cmd, &args);

The library exports xnvme_cmd_prep_nvm_read_sized(sqe, args, args_nbytes), and
the header defines xnvme_cmd_prep_nvm_read() as a static inline function that
passes sizeof(*args). The function clears the 64 bytes of the entry, then
writes the opcode and each field at the bits that the data model gives. The
function does not write the command identifier, MPTR and DPTR, so they are
zero; the backend sets them when a program passes the command with
xnvme_cmd_pass() or xnvme_cmd_pass_iov(), or an admin command with
xnvme_cmd_pass_admin(). Each header defines a macro, such as
XNVME_CMD_PREP_NVM, that a program tests with #ifdef.

Options

'spec' is the merged data model that senfd writes, of the format
xnvme_spec_headers.FORMAT; the default is the data model in senfd's cache.
'commands' is a comma separated list of command sets, such as "nvm", or of
commands, such as "nvm:read"; the default is the command sets of SETS.
'committed' is the directory of the headers to keep, or one header; the
default is include/ of the repository.

Members

Each field is a member of the record, named by the acronym of the field. Every
record has 'nsid', except the record of a command whose opcode table says that
the Namespace Identifier is not used. Two fields that hold the lower
and the upper part of a value of up to 64 bits, such as NUMDL and NUMDU, are
one member, 'numd'. The 80 bits of the logical block tags stay two members. A
value that the specification splits by bit ranges, such as "KV Key[63:00]" and
"KV Key[127:64]", is one member of bytes. The function returns -EINVAL for a
value wider than its field.

When the data model describes a dword under two conditions on a field, as
CDW13 of Read under "CETYPE is cleared to 0h" and "CETYPE is non-zero", the
function writes the fields of the description whose condition holds, and
returns -EINVAL when a field of the other description is set. A field in both
descriptions is written whatever the condition, when the two conditions cover
every value, as "is cleared to 0h" and "is non-zero" do. The generator stops on
a condition that is no comparison of a field with a value, on a condition on a
field that is no member, and on a field in two descriptions whose conditions
do not cover every value.

The generator stops on a member whose name is no C identifier, is a keyword
of C or C++ or is a macro of the C library or the compiler, such as errno, on
two members of one name, on a single field wider than 64 bits, on the parts of
a value that do not follow each other from bit 0, on a part of a value under a
condition, on a field whose bits are not bits of its dwords, on two fields
that share a bit unless their conditions exclude each other, on a field or a
raw member in the bits of the opcode, of the command identifier or, when the
record has nsid, of CDW1, and on two commands of a set that give one function
name.

A dword whose layout a feature, a directive or an operation defines is a raw
member, such as 'cdw11'. These are the dwords that the command delegates to
the fields that select their use, as CDW11 of Set Features to FID, unless the
command describes the dword itself, as CDW14 of Set Features; and the dwords
that the data model lists for the operations of a command, as CDW11 of
Migration Send. The generator stops on a dword that the command and one of its
operations both describe.

Record ABI

'args_nbytes' of the _sized function is the size of the record that the
caller was compiled with; the static inline function passes it. The function
returns -EINVAL when 'args_nbytes' is zero or not a multiple of 8, since the
size of every record is a multiple of 8. A member beyond the caller's size is
zero. For a caller's record larger than its own, the function returns
-E2BIG unless the bytes beyond its own size are zero, as openat2() does. A
command that gains fields in a later revision of a specification keeps its
function and its callers. The _sized declaration documents 'args' with yace's
tag @recparam in place of @param, so that the bindings pass the size of the
record as well; yace takes the size from the parameter after it, which is
named 'args_nbytes'.

No record has padding that the compiler adds. The generator writes a reserved
member, named 'rsvd' and its byte offset, wherever a C compiler would pad:
before a member that needs alignment, and at the end of the record, up to a
multiple of 8 bytes. A record without fields is one reserved member of 8 bytes.
The function returns -EINVAL for reserved bytes that are not zero.

The generator reads the committed headers and writes the members of each
record in their committed order, with their committed types. A member that is
new in the data model follows them. The new member replaces the reserved bytes
at the end of the record when it fits there; otherwise the record grows by 8
bytes. The generator derives the reserved members again from the layout. The
generator stops when a member or a command is gone from the data model, or
when the type of a member changed, because each breaks a caller compiled
against the committed headers. Such a change needs a major version of the
library and a new soname: delete the committed headers, and the generator
writes every record in the order of the data model.
"""

import re
import sys
from dataclasses import dataclass
from dataclasses import field as default
from pathlib import Path
from typing import Dict, List, Optional, Tuple

sys.path.insert(0, str(Path(__file__).resolve().parent))

import senfd.query  # noqa: E402
from senfd.documents.spec import Bits, Command, SpecDocument, alias  # noqa: E402
from xnvme_spec_headers import generated, load  # noqa: E402

# The command sets with preps: the key of each in the data model, and its
# abbreviation in the function names. The files have the sets in this order,
# and the commands of a set by opcode.
SETS = {
    "admin": "adm",
    "common_io": "io",
    "nvm": "nvm",
    "zoned_namespace": "zns",
    "key_value": "kv",
}

# The functions that pass a command of a set, after which the backend has set the
# command identifier, MPTR and DPTR; by the key of the set, None for any other
PASSES = {
    "admin": "xnvme_cmd_pass_admin()",
    None: "xnvme_cmd_pass() or xnvme_cmd_pass_iov()",
}

# The fields the backend sets from the payload given to xnvme_cmd_pass(),
# xnvme_cmd_pass_iov() or xnvme_cmd_pass_admin()
POINTERS = ("MPTR", "DPTR")

# The dwords of a command by the acronyms that Command.delegated gives them
DWORDS = {f"CDW{nr}": nr for nr in range(16)}

# A reserved member of a record, named by its byte offset
RESERVED = re.compile(r"^rsvd\d+$")

# The name of a member is a C identifier, and no keyword of C or of C++, as a
# C++ program includes the headers too
IDENTIFIER = re.compile(r"^[a-z_][a-z0-9_]*$")
KEYWORDS = frozenset(
    """
    alignas alignof and and_eq asm auto bitand bitor bool break case catch char
    char8_t char16_t char32_t class compl concept const consteval constexpr
    constinit const_cast continue co_await co_return co_yield decltype default
    delete do double dynamic_cast else enum explicit export extern false float
    for friend goto if inline int long mutable namespace new noexcept not not_eq
    nullptr operator or or_eq private protected public register
    reinterpret_cast requires restrict return short signed sizeof static
    static_assert static_cast struct switch template this thread_local throw
    true try typedef typeid typename typeof typeof_unqual union unsigned using
    virtual void volatile wchar_t while xor xor_eq
    """.split()
)

# Names that are macros of the C library, of GCC for a target, or of windows.h,
# which would replace a member of the name
MACROS = frozenset(
    """
    assert errno offsetof stderr stdin stdout
    i386 linux mips powerpc sparc sun unix
    far interface near small
    """.split()
)

# The size of every record is a multiple of RECORD_ALIGN bytes
RECORD_ALIGN = 8

# The integer type of a member, by its width in bits
CTYPES = ((8, "uint8_t"), (16, "uint16_t"), (32, "uint32_t"), (64, "uint64_t"))


@dataclass
class Part:
    """A part of a value: the 'width' bits from bit 'shift' of the value are
    written from bit 'bit' of the submission queue entry"""

    bit: int
    width: int
    shift: int = 0
    label: str = ""  # the name of the field, for the comment on its write


@dataclass
class Member:
    """A member of the record of arguments"""

    name: str
    doc: str
    width: int
    parts: List[Part] = default(default_factory=list)
    nbytes: Optional[int] = None  # the size of a member of bytes
    # The field, the C test, the C test of its negation, and the text
    condition: Optional[Tuple[str, str, str, str]] = None
    value: Optional[str] = None  # the name of the value that the parts are of

    @property
    def title(self) -> str:
        """The name of the value or of the field, for the comment on its check"""

        return self.value or self.parts[0].label

    @property
    def ctype(self) -> str:
        if self.nbytes:
            return "uint8_t"
        return next(ctype for bits, ctype in CTYPES if self.width <= bits)

    @property
    def size(self) -> int:
        """The size of the member in the record, in bytes"""

        if self.nbytes:
            return self.nbytes
        return next(bits for bits, _ in CTYPES if self.width <= bits) // 8

    @property
    def align(self) -> int:
        """The alignment of the member in the record, in bytes"""

        return 1 if self.nbytes else self.size

    @property
    def reserved(self) -> bool:
        return bool(RESERVED.match(self.name))


def reserved_member(offset: int, nbytes: int) -> Member:
    """The reserved member of 'nbytes' bytes at byte 'offset' of a record"""

    return Member(f"rsvd{offset}", "Reserved", 8 * nbytes, nbytes=nbytes)


def layout(members: List[Member]) -> List[Member]:
    """'members' with a reserved member wherever a C compiler would pad the
    record: before a member that needs alignment, and at the end, up to a
    multiple of RECORD_ALIGN bytes. A record of no members is one reserved
    member of RECORD_ALIGN bytes, since C has no empty struct."""

    laid: List[Member] = []
    offset = 0
    for member in members:
        if member.reserved:
            raise ValueError(f"'{member.name}' is the name of a reserved member")
        gap = -offset % member.align
        if gap:
            laid.append(reserved_member(offset, gap))
            offset += gap
        laid.append(member)
        offset += member.size
    gap = -offset % RECORD_ALIGN
    if gap or not laid:
        laid.append(reserved_member(offset, gap or RECORD_ALIGN))

    return laid


def nbytes_of(members: List[Member]) -> int:
    """The size of the record of 'members' that layout() returned"""

    return sum(member.size for member in members)


def bits_of(dword_lower: int, field: Bits) -> int:
    """The first bit of 'field' in the submission queue entry"""

    return 32 * dword_lower + field.lower


def ranges(field: Bits) -> str:
    """The bits of 'field', as the specification writes them"""

    return (
        f"{field.upper:02d}:{field.lower:02d}"
        if field.upper != field.lower
        else str(field.lower)
    )


def condition_of(command: Command, dwords) -> Optional[Tuple[str, str, str, str]]:
    """The field, the C test, the C test of its negation and the text of the
    condition of 'dwords'. Raises ValueError for a condition without a
    comparison in the data model."""

    if not dwords.condition:
        return None
    selected = dwords.selected_when
    if selected is None:
        raise ValueError(
            f"{command.name}: the condition {dwords.condition!r} is no comparison"
        )
    test = f"{selected.comparison} {selected.value}"
    negated = "!=" if selected.comparison == "==" else "=="
    return (
        selected.field.lower(),
        test,
        f"{negated} {selected.value}",
        dwords.condition.strip(),
    )


def covers(first, second) -> bool:
    """Whether a field written under the condition 'first' or under 'second'
    is written whatever the value: either has no condition, or they test one
    field against one value, one with == and the other with !="""

    return first is None or second is None or first[:2] == (second[0], second[2])


def exclusive(first, second) -> bool:
    """Whether no value meets both the conditions 'first' and 'second': they
    test one field, both with == against two values, or one with == and the
    other with != against one value"""

    if first is None or second is None or first[0] != second[0]:
        return False
    if first[1].startswith("==") and second[1].startswith("=="):
        return first[1] != second[1]
    return first[1] == second[2]


def spans(dwords, numbers) -> bool:
    """Whether 'dwords' holds one of the dwords 'numbers'"""

    return any(dwords.lower <= nr <= dwords.upper for nr in numbers)


def has_fields(dwords) -> bool:
    """Whether 'dwords' has a field that is a member of a record"""

    return any(
        f.role == "field" and f.acronym not in POINTERS for f in dwords.fields or []
    )


def operation_dwords(command: Command) -> Dict[int, List[Tuple[str, str]]]:
    """The dwords that the operations of 'command' describe: for each dword, the
    name of each operation and the figure that it cites. Dwords that refine a
    field of the command, as the FDP Events of Get Log Page refine LSP, are no
    raw members; the record has that field."""

    found: Dict[int, List[Tuple[str, str]]] = {}
    for operation in command.operations or []:
        for dwords in operation.sqe or []:
            if dwords.refines:
                continue
            for nr in range(dwords.lower, dwords.upper + 1):
                found.setdefault(nr, []).append(
                    (operation.name, senfd.query.cite(dwords.source))
                )
    return found


def own_dwords(command: Command, specific=()):
    """The dwords of 'command' whose fields are members of the record: those
    not in 'specific' and not described by an operation. Raises ValueError for
    a dword that the command and an operation both describe."""

    operations = operation_dwords(command)
    own = []
    for dwords in sorted(command.sqe, key=lambda d: d.lower):
        if spans(dwords, specific):
            continue
        if spans(dwords, operations):
            if has_fields(dwords):
                raise ValueError(
                    f"{command.name}: CDW{dwords.lower} is described by the command "
                    "and by an operation"
                )
            continue
        own.append(dwords)

    return own


def members_of(command: Command, with_nsid: bool, specific=()) -> List[Member]:
    """The members of the record of 'command', in the order of their dwords,
    with a raw member for each dword in 'specific' and for each dword that an
    operation of the command describes"""

    members: List[Member] = []
    if with_nsid:
        members.append(Member("nsid", "Namespace Identifier, CDW1", 32))
        members[-1].parts.append(Part(32, 32, label="Namespace Identifier"))

    parted: Dict[str, List[Tuple[int, Bits, str]]] = {}
    seen: Dict[str, Member] = {}
    # The bits each field takes: its name, its mask in the SQE, its condition.
    # The function writes the opcode, the backend writes the command identifier
    # at submission, and the record has nsid for CDW1, so a field in those bits
    # stops the generator.
    taken: List[List] = [
        ["the opcode", 0xFF, None],
        ["the command identifier", 0xFFFF << 16, None],
    ]
    if with_nsid:
        taken.append(["nsid", 0xFFFFFFFF << 32, None])

    def overlaps(name, mask, condition):
        for other, bits, other_condition in taken:
            if other != name and mask & bits:
                if not exclusive(condition, other_condition):
                    raise ValueError(
                        f"{command.name}: {name} and {other} take the same bits"
                    )

    for dwords in own_dwords(command, specific):
        condition = condition_of(command, dwords)
        where = (
            f"CDW{dwords.lower}"
            if dwords.lower == dwords.upper
            else f"CDW{dwords.lower}-{dwords.upper}"
        )
        for field in dwords.fields or []:
            if field.role != "field" or field.acronym in POINTERS:
                continue
            nbits = 32 * (dwords.upper - dwords.lower + 1)
            if field.upper is None or field.lower is None or field.upper >= nbits:
                raise ValueError(
                    f"{command.name}: {field.name} has the bits {field.range},"
                    f" which are not bits of {where}"
                )
            bit = bits_of(dwords.lower, field)
            mask = ((1 << field.width) - 1) << bit
            noun = "bits" if field.upper != field.lower else "bit"
            doc = f"{field.name}, {where} {noun} {ranges(field)}"
            if field.width > 64:
                what = field.acronym.lower() if field.acronym else field.name
                raise ValueError(
                    f"{command.name}: {what} is {field.width} bits, wider than a"
                    " member of 64 bits"
                )
            if field.part:
                if condition:
                    raise ValueError(
                        f"{command.name}: {field.name} is a part of"
                        f" '{field.part.name}' in {where} if {condition[3]!r},"
                        " and the generator writes a part only without a condition"
                    )
                overlaps(field.name, mask, None)
                taken.append([field.name, mask, None])
                parted.setdefault(field.part.name, []).append((bit, field, doc))
                continue
            name = (field.acronym or alias(field.name)).lower()
            if name in seen:
                # The same field in each of the descriptions of a dword
                earlier = seen[name]
                if earlier.parts[0].bit == bit and earlier.width == field.width:
                    if not covers(earlier.condition, condition):
                        raise ValueError(
                            f"{command.name}: {name} is in descriptions of {where}"
                            f" if {earlier.condition[3]!r} and if {condition[3]!r},"
                            " which do not cover every value"
                        )
                    earlier.condition = None
                    for entry in taken:
                        if entry[0] == name:
                            entry[2] = None
                    overlaps(name, mask, None)
                    continue
                raise ValueError(f"{command.name}: two fields named {name}")
            overlaps(name, mask, condition)
            taken.append([name, mask, condition])
            member = Member(name, doc, field.width)
            member.parts.append(Part(bit, field.width, label=field.name))
            member.condition = condition
            seen[name] = member
            members.append(member)

    operations = operation_dwords(command)
    for nr in sorted(set(specific) | set(operations)):
        label = (
            "as the feature or operation describes it"
            if nr in specific
            else "operation-specific, as the operation describes it"
        )
        overlaps(f"cdw{nr}", 0xFFFFFFFF << (32 * nr), None)
        member = Member(f"cdw{nr}", f"CDW{nr}, {label}", 32)
        member.parts.append(Part(32 * nr, 32, label=label))
        members.append(member)

    # The values of named parts come before the values of bytes
    for base, found in sorted(
        parted.items(), key=lambda kv: not all(f.acronym for _, f, _ in kv[1])
    ):
        members += members_of_parts(command, base, found)

    named: Dict[str, Member] = {}
    for member in members:
        if not IDENTIFIER.match(member.name) or member.name in KEYWORDS | MACROS:
            raise ValueError(
                f"{command.name}: '{member.name}' is no C identifier, or a keyword"
                " or a macro"
            )
        if member.name in named:
            raise ValueError(f"{command.name}: two members named {member.name}")
        named[member.name] = member
    for member in members:
        if member.condition is None:
            continue
        selector = named.get(member.condition[0])
        if selector is None or selector.nbytes:
            raise ValueError(
                f"{command.name}: the condition {member.condition[3]!r} of"
                f" {member.name} is on no member"
            )

    return members


def members_of_parts(
    command: Command, base: str, found: List[Tuple[int, Bits, str]]
) -> List[Member]:
    """The members of the value 'base' that the fields 'found' are parts of.
    Parts with acronyms, as NUMDL and NUMDU, are one integer member named by
    the acronym of the value, or one member each when the value is wider than
    64 bits. Parts without acronyms, as "KV Key[63:00]" and "KV Key[127:64]",
    are one member of bytes."""

    found = sorted(found, key=lambda p: p[1].part.shift)
    width = 0
    for _, field, _ in found:
        if field.part.shift != width:
            raise ValueError(
                f"{command.name}: the parts of '{base}' do not follow each other"
                " from bit 0"
            )
        width += field.width
    parts = [
        Part(bit, field.width, field.part.shift, label=field.name)
        for bit, field, _ in found
    ]

    if not all(field.acronym for _, field, _ in found):
        if width % 8 or any(p.width % 8 or p.shift % 8 for p in parts):
            raise ValueError(f"{command.name}: '{base}' is not whole bytes")
        doc = f"{base}, the first byte in the lowest bits"
        member = Member(alias(base), doc, width, nbytes=width // 8)
        member.parts += parts
        return [member]

    if len(found) < 2:
        raise ValueError(f"{command.name}: '{base}' has one part only")
    if width > 64:
        members = []
        for (_, field, doc), part in zip(found, parts):
            member = Member(field.acronym.lower(), doc, field.width)
            member.parts.append(Part(part.bit, part.width, label=part.label))
            members.append(member)
        return members

    acronym = found[0][1].part.acronym or ""
    name = acronym.lower() if len(acronym) > 1 else alias(base)
    doc = f"{base}, " + " and ".join(field.acronym for _, field, _ in found)
    return [Member(name, doc, width, parts, value=base)]


def figures_of(command: Command, specific=()) -> List[Tuple[str, str]]:
    """The dwords of the fields of 'command', each with its figure, followed by
    the dwords of its operations, each with the operation and its figure"""

    figures = []
    for dwords in own_dwords(command, specific):
        if not has_fields(dwords):
            continue
        where = (
            f"CDW{dwords.lower}"
            if dwords.lower == dwords.upper
            else f"CDW{dwords.lower}-{dwords.upper}"
        )
        if dwords.condition:
            where += f", if {dwords.condition.strip()}"
        figures.append((where, senfd.query.cite(dwords.source)))
    for nr, described in sorted(operation_dwords(command).items()):
        for operation, figure in described:
            figures.append((f"CDW{nr}, of {operation}", figure))
    return figures


def wrap(text: str, indent: str, width: int = 96) -> List[str]:
    """'text' as the lines of a comment"""

    import textwrap

    return textwrap.wrap(
        text,
        width,
        initial_indent=indent,
        subsequent_indent=indent,
        break_on_hyphens=False,
    ) or [indent.rstrip()]


# A record of the committed header, and a member of one
RECORD = re.compile(r"^struct (?P<name>xnvme_cmd_prep_\w+)_args \{$")
MEMBER = re.compile(
    r"^\t(?P<decl>(?P<ctype>\w+) (?P<name>\w+)(?:\[(?P<nbytes>\d+)\])?;)(?:\s+///<.*)?$"
)


def committed_of(path: Path) -> Dict[str, List[Tuple[str, str]]]:
    """The records of the committed headers in the directory 'path', or of the
    header 'path': for each record, the name and the declaration of each
    member, without the reserved members"""

    records: Dict[str, List[Tuple[str, str]]] = {}
    if path.is_dir():
        for header in sorted(path.glob("libxnvme_cmd_prep_*.h")):
            records.update(committed_of(header))
        return records
    if not path.exists():
        return records
    name = None
    for line in path.read_text(encoding="utf-8").splitlines():
        found = RECORD.match(line)
        if found:
            name = found.group("name")
            records[name] = []
            continue
        if name and line == "};":
            name = None
            continue
        found = MEMBER.match(line)
        if name and found and not RESERVED.match(found.group("name")):
            records[name].append((found.group("name"), found.group("decl")))
    return records


def declaration(member: Member) -> str:
    """The declaration of 'member', as the header has it"""

    array = f"[{member.nbytes}]" if member.nbytes else ""
    return f"{member.ctype} {member.name}{array};"


def delegated_of(command: Command) -> Tuple[int, ...]:
    """The dwords that 'command' delegates to the fields that select their use,
    as Set Features delegates CDW11 to CDW15 to FID, without those that the
    command describes itself, as CDW14 of Set Features"""

    own = {
        nr for d in command.sqe if has_fields(d) for nr in range(d.lower, d.upper + 1)
    }
    return tuple(
        sorted(
            {
                DWORDS[name]
                for delegation in command.delegated
                for name in delegation.fields
                if name in DWORDS
            }
            - own
        )
    )


def uses_nsid(command: Command) -> bool:
    """Whether the record of 'command' has 'nsid': unless its opcode table says
    that the Namespace Identifier is not used"""

    used = command.typed.get("namespace_identifier_used")
    return not (used is not None and used.kind == "flag" and used.flag is False)


def errors_of(members: List[Member]) -> List[str]:
    """What makes the function of the record of 'members' return -EINVAL"""

    errors = [
        "'sqe' or 'args' is NULL",
        f"'args_nbytes' is zero or not a multiple of {RECORD_ALIGN}",
    ]
    if any(m.reserved for m in members):
        errors.append("a reserved byte is not zero")
    if any(not m.nbytes and m.parts and m.width < m.size * 8 for m in members):
        errors.append("a value does not fit its field")
    if any(m.condition for m in members):
        errors.append("a field is not zero while its condition does not hold")
    return errors


class CmdPrep:
    """Writes the records of arguments and the functions that prepare commands"""

    NAME = "xnvme_cmd_prep"

    def __init__(self, output: Path, options: Dict[str, str]):
        self.output = Path(output) / self.NAME
        self.output.mkdir(parents=True, exist_ok=True)
        self.options = dict(options)
        default = Path(__file__).resolve().parents[1] / "include"
        self.committed = committed_of(Path(self.options.get("committed", default)))
        self.breaks: List[str] = []

    def keep(self, name: str, members: List[Member]) -> List[Member]:
        """'members' in the order of the committed record 'name', followed by
        the new members. Appends each change that breaks a caller to
        self.breaks."""

        if name not in self.committed:
            return members
        by_name = {m.name: m for m in members}
        kept = []
        for member, text in self.committed[name]:
            if member not in by_name:
                self.breaks.append(f"{name}(): '{member}' is gone from the data model")
                continue
            if declaration(by_name[member]) != text:
                self.breaks.append(
                    f"{name}(): '{text}' is now '{declaration(by_name[member])}'"
                )
            kept.append(by_name.pop(member))
        return kept + [m for m in members if m.name in by_name]

    def commands(self, spec: SpecDocument) -> List[Tuple[str, str, Command]]:
        """The commands selected: the set, the key of the command, the command"""

        wanted = [w.strip() for w in self.options.get("commands", "").split(",") if w]
        chosen = []
        for set_key in SETS:
            command_set = spec.command_sets.get(set_key)
            if command_set is None:
                continue
            for cmd_key, command in sorted(
                command_set.commands.items(), key=lambda c: int(c[1].opcode)
            ):
                if not wanted or any(
                    w in (set_key, f"{set_key}:{cmd_key}") for w in wanted
                ):
                    chosen.append((set_key, alias(command.name), command))
        return chosen

    def write(self, set_key: str, cmd_key: str, command: Command, decls, defs):
        name = f"xnvme_cmd_prep_{SETS[set_key]}_{cmd_key}"
        sized = f"{name}_sized"
        record = f"{name}_args"
        specific = delegated_of(command)
        members = members_of(command, uses_nsid(command), specific)
        members = layout(self.keep(name, members))
        opcode = int(command.opcode)

        decls += [
            "/**",
            f" * The arguments of {name}(): the fields of {command.name}",
            " *",
        ]
        decls += wrap(f"See {senfd.query.cite(command.source)}.", " * ")
        figures = figures_of(command, specific)
        if figures:
            decls += [" *", " * The fields of each dword are from:", " *"]
            for where, figure in figures:
                decls += wrap(f"- {where}: {figure}", " * ")
        decls += [" *"]
        decls += wrap(
            "A later version of the library adds members only at the end. The"
            " function takes a member that the record of a program lacks as zero,"
            " so zero is the default of every member. The function returns -EINVAL"
            " for a reserved member that is not zero.",
            " * ",
        )
        decls += [" */", f"struct {record} {{"]
        for m in members:
            decls.append(f"\t{declaration(m)} ///< {m.doc}")
        decls += ["};", ""]

        errors = errors_of(members)
        einval = ", ".join(errors[:-1]) + f", or {errors[-1]}"
        decls += [
            "/**",
            f" * Prepare 'sqe' as the command {command.name}, opcode {opcode:02X}h, from 'args'",
            " *",
            " * Clears the 64 bytes of 'sqe', then writes the opcode and each field of 'args'.",
            " * The command identifier, MPTR and DPTR are left zero, for"
            f" {PASSES.get(set_key, PASSES[None])} to set.",
            f" * A program calls {name}(), which passes the size of 'args'.",
            " *",
            " * @param sqe The submission queue entry to prepare",
            f" * @recparam args The fields of the command, see ::{record}",
            " * @param args_nbytes The size of the record that the program was built with",
            " *",
        ]
        decls += wrap(
            "@return On success, 0 is returned. On error, negative `errno` is returned:"
            f" -EINVAL when {einval}; -E2BIG when 'args_nbytes' is larger than the"
            " record and the bytes beyond it are not zero.",
            " * ",
        )
        decls += [
            " *",
            " * @errno",
            " */",
            "int",
            f"{sized}(union xnvme_cmd_sqe *sqe, const struct {record} *args, size_t args_nbytes);",
            "",
            "/**",
            f" * Prepare 'sqe' as the command {command.name} from 'args', see {sized}()",
            " *",
            " * @param sqe The submission queue entry to prepare",
            f" * @param args The fields of the command, see ::{record}",
            " *",
            f" * @return The return value of {sized}()",
            " */",
            "static inline int",
            f"{name}(union xnvme_cmd_sqe *sqe, const struct {record} *args)",
            "{",
            f"\treturn {sized}(sqe, args, sizeof(*args));",
            "}",
            "",
        ]

        offset, reserved = 0, []
        for m in members:
            if m.reserved:
                reserved += [str(offset), str(m.size)]
            offset += m.size
        body = [
            f"XNVME_STATIC_ASSERT(sizeof(struct {record}) == {nbytes_of(members)},",
            '\t\t    "The record has padding")',
            "",
            "int",
            f"{sized}(union xnvme_cmd_sqe *sqe, const struct {record} *args, size_t args_nbytes)",
            "{",
        ]
        if reserved:
            body.append(f"\tstatic const uint16_t rsvd[] = {{{', '.join(reserved)}}};")
        body += [
            f"\tstruct {record} a = {{0}};",
            "\tint err;",
            "",
            (
                f"\terr = prep_args(&a, sizeof(a), args, args_nbytes, rsvd, {len(reserved) // 2});"
                if reserved
                else "\terr = prep_args(&a, sizeof(a), args, args_nbytes, NULL, 0);"
            ),
            "\tif (err || !sqe) {",
            "\t\treturn err ? err : -EINVAL;",
            "\t}",
        ]
        checks = []
        for m in members:
            if m.nbytes or not m.parts:
                continue
            first = m.parts[0].bit
            order = (first // 32, -(first % 32))
            if m.width < dict((c, b) for b, c in CTYPES)[m.ctype]:
                unit = "bit" if m.width == 1 else "bits"
                checks.append(
                    (
                        order,
                        [
                            f"\tif (a.{m.name} >> {m.width}) {{ ///< {m.title}: {m.width} {unit}",
                            "\t\treturn -EINVAL;",
                            "\t}",
                        ],
                    )
                )
            if m.condition:
                field, _, inverse, text = m.condition
                checks.append(
                    (
                        order,
                        [
                            f"\tif (a.{field} {inverse} && a.{m.name}) {{ ///< {m.title}: only if {text}",
                            "\t\treturn -EINVAL;",
                            "\t}",
                        ],
                    )
                )
        if checks:
            body.append("")
        for _, lines in sorted(checks, key=lambda c: c[0]):
            body += lines
        body += [
            "",
            "\tmemset(sqe, 0, sizeof(*sqe));",
            f"\tsqe->cdw[0] = 0x{opcode:02X};",
        ]
        writes = []
        for m in members:
            for p in m.parts:
                if m.nbytes:
                    value = f"prep_le64(&a.{m.name}[{p.shift // 8}], {p.width // 8})"
                else:
                    value = f"a.{m.name}" + (f" >> {p.shift}" if p.shift else "")
                write = (
                    f"prep_bits(sqe, {p.bit // 32}, {p.bit % 32}, {p.width}, {value});"
                    f" ///< {p.label}"
                )
                if m.condition:
                    field, test, _, _ = m.condition
                    lines = [f"\tif (a.{field} {test}) {{", f"\t\t{write}", "\t}"]
                else:
                    lines = [f"\t{write}"]
                writes.append(((p.bit // 32, -(p.bit % 32)), lines))
        if writes:
            body.append("")
        # In the order of the figures: by dword, then from the highest bit
        for _, lines in sorted(writes, key=lambda w: w[0]):
            body += lines
        body += ["", "\treturn 0;", "}", ""]
        defs += body

    def generate(self):
        spec = load(self.options.get("spec"))

        decls: Dict[str, List[str]] = {}
        defs: List[str] = []
        written = set()
        titles = {k: s.title or s.name for k, s in spec.command_sets.items()}
        for set_key, cmd_key, command in self.commands(spec):
            if set_key not in decls:
                decls[set_key] = []
                defs += ["/*", f" * {titles[set_key]}", " */", ""]
            name = f"xnvme_cmd_prep_{SETS[set_key]}_{cmd_key}"
            if name in written:
                raise ValueError(
                    f"{command.name}: two commands of the {titles[set_key]} give"
                    f" {name}()"
                )
            self.write(set_key, cmd_key, command, decls[set_key], defs)
            written.add(name)
        if not self.options.get("commands"):
            for name in sorted(set(self.committed) - written):
                self.breaks.append(f"{name}(): the command is gone from the data model")
        if self.breaks:
            raise ValueError(
                "would break callers of the committed headers; for a major version, delete "
                "them and generate again:\n  " + "\n  ".join(self.breaks)
            )

        headers = []
        for set_key, lines in decls.items():
            name = f"libxnvme_cmd_prep_{SETS[set_key]}.h"
            guard = f"__LIBXNVME_CMD_PREP_{SETS[set_key].upper()}_H"
            head = [
                "/**",
                f" * Prepare the commands of the {titles[set_key]} from records of their fields",
                " *",
                f" * Written from the data model of senfd {spec.meta.version}, by"
                " toolbox/xnvme_cmd_prep.py;",
                " * each field is named and placed as the figure it cites gives it. Do not"
                " edit:",
                " * see the contributor docs on producing the command preps.",
                " *",
                # REUSE-IgnoreStart
                " * SPDX-FileCopyrightText: Samsung Electronics Co., Ltd",
                " *",
                " * SPDX-License-Identifier: BSD-3-Clause",
                # REUSE-IgnoreEnd
                " *",
                f" * @file {name}",
                " */",
                "",
                f"#ifndef {guard}",
                f"#define {guard}",
                "",
                "#ifdef __cplusplus",
                'extern "C" {',
                "#endif",
                "",
                f"/** Defined where the preps of the {titles[set_key]} are declared; a"
                " program tests it with `#ifdef` */",
                f"#define XNVME_CMD_PREP_{SETS[set_key].upper()} 1",
                "",
            ]
            tail = [
                "#ifdef __cplusplus",
                "}",
                "#endif",
                "",
                f"#endif /* {guard} */",
                "",
            ]
            header = self.output / name
            header.write_text("\n".join(head + lines + tail), encoding="utf-8")
            headers.append(header)

        helpers = [
            # REUSE-IgnoreStart
            "// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd",
            "//",
            "// SPDX-License-Identifier: BSD-3-Clause",
            # REUSE-IgnoreEnd
            "",
            "/*",
            f" * Written from the data model of senfd {spec.meta.version}, by"
            " toolbox/xnvme_cmd_prep.py. Do not edit.",
            " */",
            "#include <errno.h>",
            "#include <string.h>",
            "#include <libxnvme.h>",
            "",
            "/**",
            " * Copy the caller's record into 'a', zero beyond the caller's size",
            " *",
            " * @param a The record of the function, zero on entry",
            " * @param a_nbytes The size of 'a'",
            " * @param args The caller's record",
            " * @param args_nbytes The size of the caller's record",
            " * @param rsvd The reserved bytes of 'a', as 'nrsvd' pairs of a byte offset and a",
            " * number of bytes",
            " * @param nrsvd The number of pairs in 'rsvd'",
            " *",
            " * @return 0 on success; -EINVAL when 'args' is NULL, 'args_nbytes' is zero or not a",
            f" * multiple of {RECORD_ALIGN}, or a reserved byte is not zero; -E2BIG when 'args_nbytes'",
            " * is larger than 'a_nbytes' and the bytes beyond 'a_nbytes' are not zero.",
            " */",
            "static int",
            "prep_args(void *a, size_t a_nbytes, const void *args, size_t args_nbytes,",
            "\t  const uint16_t *rsvd, unsigned int nrsvd)",
            "{",
            f"\tif (!args || !args_nbytes || args_nbytes % {RECORD_ALIGN}) {{",
            "\t\treturn -EINVAL;",
            "\t}",
            "\tfor (size_t i = a_nbytes; i < args_nbytes; ++i) {",
            "\t\tif (((const uint8_t *)args)[i]) {",
            "\t\t\treturn -E2BIG;",
            "\t\t}",
            "\t}",
            "\tmemcpy(a, args, args_nbytes < a_nbytes ? args_nbytes : a_nbytes);",
            "",
            "\tfor (unsigned int i = 0; i < nrsvd; ++i) {",
            "\t\tfor (unsigned int b = 0; b < rsvd[2 * i + 1]; ++b) {",
            "\t\t\tif (((const uint8_t *)a)[rsvd[2 * i] + b]) {",
            "\t\t\t\treturn -EINVAL;",
            "\t\t\t}",
            "\t\t}",
            "\t}",
            "",
            "\treturn 0;",
            "}",
            "",
            "/**",
            " * Write the low 'width' bits of 'value' into 'sqe'",
            " *",
            " * A field wider than what is left of dword 'cdw' continues in the dwords after",
            " * it, as SLBA does in CDW10 and CDW11.",
            " *",
            " * @param sqe The submission queue entry",
            " * @param cdw The dword of the lowest bit of the field",
            " * @param bit The lowest bit of the field in dword 'cdw'",
            " * @param width The number of bits of the field",
            " * @param value The value of the field",
            " */",
            "static void",
            "prep_bits(union xnvme_cmd_sqe *sqe, unsigned int cdw, unsigned int bit, unsigned int width,",
            "\t  uint64_t value)",
            "{",
            "\tbit += 32 * cdw;",
            "",
            "\tfor (unsigned int i = 0; i < width;) {",
            "\t\tunsigned int dw = (bit + i) / 32, ofz = (bit + i) % 32;",
            "\t\tunsigned int n = 32 - ofz < width - i ? 32 - ofz : width - i;",
            "\t\tuint32_t mask = n == 32 ? 0xFFFFFFFFu : (1u << n) - 1u;",
            "",
            "\t\tsqe->cdw[dw] = (sqe->cdw[dw] & ~(mask << ofz)) |",
            "\t\t\t       (((uint32_t)(value >> i) & mask) << ofz);",
            "\t\ti += n;",
            "\t}",
            "}",
            "",
        ]
        le64 = [
            "/**",
            " * The 'nbytes' bytes from 'bytes' as an integer, the first in its lowest bits",
            " *",
            " * Gives a value of bytes, as the key of a Key Value command, the byte order of",
            " * the submission queue entry on any host.",
            " */",
            "static uint64_t",
            "prep_le64(const uint8_t *bytes, unsigned int nbytes)",
            "{",
            "\tuint64_t value = 0;",
            "",
            "\tfor (unsigned int i = 0; i < nbytes; ++i) {",
            "\t\tvalue |= (uint64_t)bytes[i] << (8 * i);",
            "\t}",
            "",
            "\treturn value;",
            "}",
            "",
        ]
        source = self.output / "xnvme_cmd_prep.c"
        if any("prep_le64(" in line for line in defs):
            helpers += le64
        source.write_text("\n".join(helpers + defs), encoding="utf-8")

        return generated(headers, [source])


if __name__ == "__main__":
    import argparse

    parser = argparse.ArgumentParser(description="Write the command preps")
    parser.add_argument("output", type=Path)
    parser.add_argument("--spec", default=None)
    parser.add_argument("--commands", default="")
    parser.add_argument("--committed", default=None)
    args = parser.parse_args()
    options = {"commands": args.commands}
    if args.committed:
        options["committed"] = args.committed
    if args.spec:
        options["spec"] = args.spec
    print(CmdPrep(args.output, options).generate())
