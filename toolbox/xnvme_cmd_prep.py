#!/usr/bin/env python3
"""
The command preps of xNVMe, from senfd's data model of the NVMe specifications

This file is a generator for yace. yace runs the generator and parses the
headers that the generator writes, as yace parses any header:

    yace --generate toolbox/xnvme_cmd_prep.py:CmdPrep \\
         --generator-option xnvme_cmd_prep.spec=merged.spec.document.json \\
         --output /tmp/prep --strict --emit capi

The build does not run the generator. A contributor runs the generator on a
machine with the specification documents and commits the files that the
generator writes, as with the spec headers.

For each command, the generator writes a record of the fields of the command,
and a function that prepares a submission queue entry from the record. The
records and functions of each command set are in one header,
libxnvme_cmd_prep_<set>.h, and one source file holds all the functions:

    struct xnvme_cmd_prep_nvm_read_args args = {
        .args_nbytes = sizeof(args), .nsid = 1, .slba = 0, .nlb = 7};

    err = xnvme_cmd_prep_nvm_read(&ctx->cmd, &args);

The function clears the 64 bytes of the entry, then writes the opcode and each
field at the bits the data model gives it. The command identifier, MPTR and
DPTR are left zero, for xnvme_cmd_pass() and the backend to set.

Each field is a member of the record, named by the acronym of the field. Every
command has 'nsid', except an admin command whose opcode table says that the
Namespace Identifier is not used. When the specification splits a value into
two fields named "<name> Lower" and "<name> Upper", and the two fit in 64
bits, the record has one member for the value, such as 'numd' for NUMDL and
NUMDU. The 80 bits of the logical block tags do not fit, so the tags stay two
members. When the specification splits a value by bit ranges, such as "KV
Key[63:00]" and "KV Key[127:64]", the record has one member of bytes. The
function refuses a value wider than its field with -EINVAL, rather than
cutting the value.

The data model can describe a dword once for each of two conditions on a
field. CDW13 of Read is described for "if CETYPE is cleared to 0h" and for
"if CETYPE is non-zero". The function writes the fields of the description
whose condition holds, and refuses with -EINVAL a field of the other
description that is set. Any other condition stops the generator.

The first member of every record, 'args_nbytes', is the size of the record
that the caller was compiled with, and the caller sets it. The function
refuses a record whose size is zero with -EINVAL. A field that the caller's
record does not have is zero. The function refuses a caller's record that is
larger than its own with -E2BIG, unless the bytes that the function does not
know are zero, as openat2() does with the size it is given. A command that
gains fields in a later revision of a specification thereby keeps its function
and its callers.

Each header defines a macro, XNVME_CMD_PREP_<SET>, such as XNVME_CMD_PREP_NVM,
so that a program that also builds against a library without the preps can
test for them with #ifdef.

A command can leave a dword to its operations, as Migration Send leaves CDW11
to the operation that SEL selects. The data model lists the dwords of each
operation, and the record has one raw member for each such dword, such as
'cdw11', as it has for the dwords of SPECIFIC. A dword that the command and
one of its operations both describe stops the generator. FOREIGN lists the
dwords that the data model gives a command from the figures of something
else, and the generator leaves those out. The dwords of a command that come
from different sections of one document also stop the generator, so that such
a dword is looked up and listed rather than written.

No record has padding that the compiler adds. The generator lays the members
out as a C compiler does and writes a reserved member, named 'rsvd' and its
byte offset, wherever the compiler would pad: before a member that needs
alignment, and at the end of the record, up to a multiple of 8 bytes. The
function refuses reserved bytes that are not zero with -EINVAL. The size of a
record therefore says exactly which members the record has, and a field that
a newer caller sets in bytes that this library holds reserved is refused
rather than dropped.

This compatibility holds while the records only grow at their end, so the
generator keeps what the committed headers have. The members of each record
stay in their committed order, with their committed types, and a member that
is new in the data model is added after them. The new member takes the place
of the reserved bytes at the end of the record when it fits there, and the
size of the record stays the same. When the reserved bytes are used up, the
record grows by 8 bytes and ends in a new reserved member. In both cases the
members that were there keep their offsets. The reserved members are not
kept: the generator derives them again from the layout. A member or a command
that is gone from the data model, or a member whose type changed, stops the
generator, because each would break a caller compiled against the committed
headers. Such a change waits for a major version of the library and a new
soname: delete the committed headers, and the generator writes every record
in the order of the data model.

The generator has three options. 'spec' is the merged specification document
that senfd writes; the default is the one in senfd's cache. 'commands' is a
comma separated list of command sets, such as "nvm", or of commands, such as
"nvm:read"; the default is the command sets of SETS. 'committed' is the
directory of the headers to keep, or one header; the default is include/ of
the repository.
"""

import re
import sys
from dataclasses import dataclass
from dataclasses import field as default
from pathlib import Path
from typing import Dict, List, Optional, Tuple

sys.path.insert(0, str(Path(__file__).resolve().parent))

import senfd.query  # noqa: E402
from senfd.documents.spec import Bits, Command, SpecDocument  # noqa: E402
from xnvme_spec_headers import alias, generated, senfd_version  # noqa: E402

# The command sets that get preps: the key of each in the data model, and the
# abbreviation used in the function names. The files list the sets in this
# order, and the commands of a set by opcode.
SETS = {
    "admin": "adm",
    "i_o": "io",
    "nvm": "nvm",
    "zoned_namespace": "zns",
    "key_value": "kv",
}

# The title of the section of each command set in the files written
TITLES = {
    "admin": "Admin Command Set",
    "i_o": "I/O Commands",
    "nvm": "NVM Command Set",
    "zoned_namespace": "Zoned Namespace Command Set",
    "key_value": "Key Value Command Set",
}

# The fields the backend sets from the payload given to xnvme_cmd_pass()
POINTERS = ("MPTR", "DPTR")

# The command set specifications that senfd cites by a short name, such as
# "NVM 1.3" for "NVM Command Set 1.3"
CITED = re.compile(r"^(NVM|Zoned Namespace|Key Value) (?=\d)")

# The dwords whose layout a feature or a directive operation defines. The
# record carries each as a raw member, such as 'cdw11'. The data model gives
# Set Features and Get Features a CDW11 of the Host Metadata feature (Base 2.4,
# Figures 530 and 531), and the raw member replaces it.
SPECIFIC = {
    ("admin", "set_features"): (11, 12, 13, 15),
    ("admin", "get_features"): (11,),
    ("admin", "directive_send"): (12, 13, 14, 15),
    ("admin", "directive_receive"): (12, 13, 14, 15),
}

# The dwords that the data model gives a command from the figures of something
# else. They are left out of the record. The data model gives the Controller
# Data Queue command the CDW11, CDW12 and CDW13 of the Controller Data Queue
# feature of Set Features (Base 2.4, Figures 508 to 510). The command leaves
# CDW11 and CDW12 to its operations and has no CDW13 (Base 2.4, 5.2.5).
FOREIGN = {
    ("admin", "controller_data_queue"): (11, 12, 13),
}

# The first and the last dword that a command or its operations describe
COMMAND_DWORDS = (10, 15)

# A reserved member of a record, named by its byte offset
RESERVED = re.compile(r"^rsvd\d+$")

# The size that every record is a multiple of, in bytes
RECORD_ALIGN = 8

# The integer type of a member, by its width in bits
CTYPES = ((8, "uint8_t"), (16, "uint16_t"), (32, "uint32_t"), (64, "uint64_t"))

# The conditions on a field the generator follows, and the C test of each
CONDITIONS = (
    (re.compile(r"^(?P<field>[A-Z][A-Z0-9]*) is cleared to 0h$"), "== 0"),
    (re.compile(r"^(?P<field>[A-Z][A-Z0-9]*) is non-zero$"), "!= 0"),
)

# A field named as a part of a value by its bit range, such as "KV Key[63:00]"
RANGED = re.compile(r"^(?P<base>.+?)\s*\[(?P<upper>\d+):(?P<lower>\d+)\]$")

# A field named as the lower or upper half of a value
HALF = re.compile(r"^(?P<base>.+?)\s+(?P<half>Lower|Upper)$")


@dataclass
class Part:
    """A part of a value: 'width' bits, from bit 'shift' of the value, are
    written from bit 'bit' of the submission queue entry"""

    bit: int
    width: int
    shift: int = 0
    label: str = ""  # the name of the field, used in the comment on its write


@dataclass
class Member:
    """A member of the record of arguments"""

    name: str
    doc: str
    width: int
    parts: List[Part] = default(default_factory=list)
    nbytes: Optional[int] = None  # the size of a member of bytes
    condition: Optional[Tuple[str, str, str]] = None  # the field, the C test, the text

    @property
    def title(self) -> str:
        """The name of the field, used in the comment on its check"""

        return re.sub(r" Lower$", "", self.parts[0].label) if self.parts else self.name

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

    return Member(f"rsvd{offset}", "Reserved; must be zero", 8 * nbytes, nbytes=nbytes)


def layout(members: List[Member]) -> List[Member]:
    """'members' with a reserved member wherever a C compiler would pad the
    record: before a member that needs alignment, and at the end, up to a
    multiple of RECORD_ALIGN bytes. The offsets of 'members' are those the
    compiler gives them, so a record that gains a member at its end keeps the
    offsets of the members it had."""

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
    if gap:
        laid.append(reserved_member(offset, gap))

    return laid


def nbytes_of(members: List[Member]) -> int:
    """The size of the record of 'members', which layout() has laid out"""

    return sum(member.size for member in members)


def label_of(field: Bits) -> str:
    """The name of 'field', such as "Force Unit Access"; the member itself is
    named by the acronym"""

    return field.name


def cite(source) -> str:
    """The figure a field is from, as senfd cites it, with the name of the
    command set specification written in full"""

    return CITED.sub(r"\1 Command Set ", senfd.query.cite(source))


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


def condition_of(
    command: Command, text: Optional[str]
) -> Optional[Tuple[str, str, str]]:
    """The field, the C test and the text of a condition the generator follows"""

    if not text:
        return None
    for pattern, test in CONDITIONS:
        found = pattern.match(text.strip())
        if found:
            return found.group("field").lower(), test, text.strip()
    raise ValueError(f"{command.name}: the condition {text!r} is not one of CONDITIONS")


def spans(dwords, numbers) -> bool:
    """Whether 'dwords' holds one of the dwords 'numbers'"""

    return any(dwords.lower <= nr <= dwords.upper for nr in numbers)


def has_fields(dwords) -> bool:
    """Whether 'dwords' has a field that a record would carry"""

    return any(
        f.role == "field" and f.acronym not in POINTERS for f in dwords.fields or []
    )


def operation_dwords(command: Command) -> Dict[int, List[Tuple[str, str]]]:
    """The dwords that the operations of 'command' describe: for each dword, the
    name of each operation describing it and the figure it cites"""

    found: Dict[int, List[Tuple[str, str]]] = {}
    for operation in command.operations or []:
        for dwords in operation.sqe or []:
            for nr in range(dwords.lower, dwords.upper + 1):
                if COMMAND_DWORDS[0] <= nr <= COMMAND_DWORDS[1]:
                    found.setdefault(nr, []).append(
                        (operation.name, cite(dwords.source))
                    )
    return found


def own_dwords(command: Command, specific=(), foreign=()):
    """The dwords of 'command' whose fields are members of the record: those not
    'specific', not 'foreign' and not left to an operation. Stops on a dword of
    'foreign' that the data model does not have, on a dword that the command
    and an operation both describe, and on dwords from different sections of
    one document."""

    for nr in foreign:
        if not any(spans(dwords, (nr,)) for dwords in command.sqe):
            raise ValueError(f"{command.name}: FOREIGN lists CDW{nr}, which is gone")

    operations = operation_dwords(command)
    own = []
    for dwords in sorted(command.sqe, key=lambda d: d.lower):
        if spans(dwords, foreign) or spans(dwords, specific):
            continue
        if spans(dwords, operations):
            if has_fields(dwords):
                raise ValueError(
                    f"{command.name}: CDW{dwords.lower} is described by the command "
                    "and by an operation"
                )
            continue
        own.append(dwords)

    sections: Dict[str, Tuple[str, int]] = {}
    for dwords in own:
        if not has_fields(dwords) or dwords.lower < COMMAND_DWORDS[0]:
            continue
        document = dwords.source.document
        section = ".".join((dwords.source.section or "").split(".")[:3])
        first = sections.setdefault(document, (section, dwords.lower))
        shared = min(len(first[0].split(".")), len(section.split(".")))
        if first[0].split(".")[:shared] != section.split(".")[:shared]:
            raise ValueError(
                f"{command.name}: CDW{first[1]} is from section {first[0]} and "
                f"CDW{dwords.lower} from section {section} of one document; list "
                "the dwords that are not of the command in FOREIGN"
            )

    return own


def members_of(
    command: Command, with_nsid: bool, specific=(), foreign=()
) -> List[Member]:
    """The members of the record of 'command', in the order of their dwords, with
    a raw member for each of the 'specific' dwords and for each dword that an
    operation of the command describes, and without the 'foreign' dwords"""

    members: List[Member] = []
    if with_nsid:
        members.append(Member("nsid", "Namespace Identifier, CDW1", 32))
        members[-1].parts.append(Part(32, 32, label="Namespace Identifier"))

    halves: Dict[str, Dict[str, Tuple[int, Bits, str]]] = {}
    pieces: Dict[str, List[Tuple[int, Bits, str, int]]] = {}
    seen: Dict[str, Member] = {}

    for dwords in own_dwords(command, specific, foreign):
        condition = condition_of(command, dwords.condition)
        where = (
            f"CDW{dwords.lower}"
            if dwords.lower == dwords.upper
            else f"CDW{dwords.lower}-{dwords.upper}"
        )
        for field in dwords.fields or []:
            if field.role != "field" or field.acronym in POINTERS:
                continue
            bit = bits_of(dwords.lower, field)
            noun = "bits" if field.upper != field.lower else "bit"
            doc = f"{field.name}, {where} {noun} {ranges(field)}"
            ranged = RANGED.match(field.name or "")
            half = HALF.match(field.name or "")
            if ranged and not field.acronym:
                pieces.setdefault(ranged.group("base"), []).append(
                    (int(ranged.group("lower")), field, doc, bit)
                )
                continue
            if half and field.acronym:
                halves.setdefault(half.group("base"), {})[half.group("half")] = (
                    bit,
                    field,
                    doc,
                )
                continue
            name = (field.acronym or alias(field.name)).lower()
            if name in seen:
                # The same field in each of the descriptions of a dword
                if seen[name].parts[0].bit == bit and seen[name].width == field.width:
                    seen[name].condition = None
                    continue
                raise ValueError(f"{command.name}: two fields named {name}")
            member = Member(name, doc, field.width)
            member.parts.append(Part(bit, field.width, label=label_of(field)))
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
        member = Member(f"cdw{nr}", f"CDW{nr}, {label}", 32)
        member.parts.append(Part(32 * nr, 32, label=label))
        members.append(member)

    for base, pair in halves.items():
        if set(pair) != {"Lower", "Upper"}:
            raise ValueError(f"{command.name}: '{base}' has one half only")
        lbit, lower, _ = pair["Lower"]
        ubit, upper, _ = pair["Upper"]
        width = lower.width + upper.width
        if width > 64:
            for bit, field, doc in (pair["Lower"], pair["Upper"]):
                member = Member(field.acronym.lower(), doc, field.width)
                member.parts.append(Part(bit, field.width, label=label_of(field)))
                members.append(member)
            continue
        stem = lower.acronym[:-1] if lower.acronym[:-1] == upper.acronym[:-1] else ""
        name = stem.lower() if len(stem) > 1 else alias(base)
        doc = f"{base}, {lower.acronym} and {upper.acronym}"
        member = Member(name, doc, width)
        member.parts += [
            Part(lbit, lower.width, label=label_of(lower)),
            Part(ubit, upper.width, lower.width, label=label_of(upper)),
        ]
        members.append(member)

    for base, found in pieces.items():
        found.sort(key=lambda p: p[0])
        width = sum(f.width for _, f, _, _ in found)
        if width % 8 or any(f.width % 8 or lo % 8 for lo, f, _, _ in found):
            raise ValueError(f"{command.name}: '{base}' is not whole bytes")
        doc = f"{base}, the first byte in the lowest bits"
        member = Member(alias(base), doc, width, nbytes=width // 8)
        member.parts += [
            Part(bit, f.width, lo, label=f.name) for lo, f, _, bit in found
        ]
        members.append(member)

    return members


def figures_of(command: Command, specific=(), foreign=()) -> List[Tuple[str, str]]:
    """The dwords of the fields of 'command', each with the figure citing them,
    followed by the dwords of its operations, each with the operation and the
    figure"""

    figures = []
    for dwords in own_dwords(command, specific, foreign):
        if not has_fields(dwords):
            continue
        where = (
            f"CDW{dwords.lower}"
            if dwords.lower == dwords.upper
            else f"CDW{dwords.lower}-{dwords.upper}"
        )
        if dwords.condition:
            where += f", if {dwords.condition.strip()}"
        figures.append((where, cite(dwords.source)))
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
    header 'path': for each record, the name and the declaration of each member,
    without the reserved members, which layout() derives"""

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
    """The declaration of 'member' in its record, as the header writes it"""

    array = f"[{member.nbytes}]" if member.nbytes else ""
    return f"{member.ctype} {member.name}{array};"


def size_member() -> Member:
    """The first member of every record: its size"""

    return Member(
        "args_nbytes", "The size of the record, sizeof(args); must be set", 32
    )


class CmdPrep:
    """Writes the records of arguments and the functions preparing commands"""

    NAME = "xnvme_cmd_prep"

    def __init__(self, output: Path, options: Dict[str, str]):
        self.output = Path(output) / self.NAME
        self.output.mkdir(parents=True, exist_ok=True)
        self.options = dict(options)
        default = Path(__file__).resolve().parents[1] / "include"
        self.committed = committed_of(Path(self.options.get("committed", default)))
        self.breaks: List[str] = []

    def keep(self, name: str, members: List[Member]) -> List[Member]:
        """'members' in the order of the committed record of 'name', followed by
        the new members; a change that would break a caller is added to
        self.breaks"""

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
        """The commands asked for: the set, the key of the command, the command"""

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
        record = f"{name}_args"
        used = command.columns.get("namespace_identifier_used", "Yes")
        specific = SPECIFIC.get((set_key, cmd_key), ())
        foreign = FOREIGN.get((set_key, cmd_key), ())
        members = members_of(
            command,
            with_nsid=not used.startswith("No"),
            specific=specific,
            foreign=foreign,
        )
        members = layout(self.keep(name, [size_member(), *members]))
        opcode = int(command.opcode)

        decls += [
            "/**",
            f" * The arguments of {name}(): the fields of {command.name}",
            " *",
        ]
        decls += wrap(f"See {cite(command.source)}.", " * ")
        figures = figures_of(command, specific, foreign)
        if figures:
            decls += [" *", " * The fields of each dword are from:", " *"]
            for where, figure in figures:
                decls += wrap(f"- {where}: {figure}", " * ")
        decls += [" */", f"struct {record} {{"]
        for m in members:
            decls.append(f"\t{declaration(m)} ///< {m.doc}")
        decls += ["};", ""]

        decls += [
            "/**",
            f" * Prepare 'sqe' as the command {command.name}, opcode {opcode:02X}h, from 'args'",
            " *",
            " * Clears the 64 bytes of 'sqe', then writes the opcode and each field of 'args'.",
            " * The command identifier, MPTR and DPTR are left zero, for xnvme_cmd_pass() to set.",
            " *",
            " * @param sqe The submission queue entry to prepare",
            f" * @param args The fields of the command, see ::{record}",
            " *",
            " * @return On success, 0 is returned. On error, negative `errno` is returned:",
            " * -EINVAL when 'sqe' or 'args' is NULL, 'args->args_nbytes' is zero, a value does",
            " * not fit its field, or a reserved byte is not zero, and -E2BIG when",
            " * 'args->args_nbytes' is larger than the record and the bytes beyond it are not",
            " * zero.",
            " *",
            " * @errno",
            " */",
            "int",
            f"{name}(union xnvme_spec_sqe *sqe, const struct {record} *args);",
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
            f"{name}(union xnvme_spec_sqe *sqe, const struct {record} *args)",
            "{",
        ]
        if reserved:
            body.append(f"\tstatic const uint16_t rsvd[] = {{{', '.join(reserved)}}};")
        body += [
            f"\tstruct {record} a = {{0}};",
            "\tint err;",
            "",
            (
                f"\terr = prep_args(&a, sizeof(a), args, rsvd, {len(reserved) // 2});"
                if reserved
                else "\terr = prep_args(&a, sizeof(a), args, NULL, 0);"
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
                field, test, text = m.condition
                inverse = "!= 0" if test == "== 0" else "== 0"
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
                    field, test, _ = m.condition
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
        path = self.options.get("spec")
        spec = senfd.query.load(Path(path) if path else None)

        decls: Dict[str, List[str]] = {}
        defs: List[str] = []
        written = set()
        for set_key, cmd_key, command in self.commands(spec):
            if set_key not in decls:
                decls[set_key] = []
                defs += ["/*", f" * {TITLES[set_key]}", " */", ""]
            self.write(set_key, cmd_key, command, decls[set_key], defs)
            written.add(f"xnvme_cmd_prep_{SETS[set_key]}_{cmd_key}")
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
            head = [
                "/**",
                f" * Prepare the commands of the {TITLES[set_key]} from records of their fields",
                " *",
                f" * Written from the data model of senfd {senfd_version()}, by"
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
                f"/** Defined where the preps of the {TITLES[set_key]} are declared; a"
                " program tests it with #ifdef */",
                f"#define XNVME_CMD_PREP_{SETS[set_key].upper()} 1",
                "",
            ]
            header = self.output / name
            header.write_text("\n".join(head + lines), encoding="utf-8")
            headers.append(header)

        helpers = [
            # REUSE-IgnoreStart
            "// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd",
            "//",
            "// SPDX-License-Identifier: BSD-3-Clause",
            # REUSE-IgnoreEnd
            "",
            "/*",
            f" * Written from the data model of senfd {senfd_version()}, by"
            " toolbox/xnvme_cmd_prep.py. Do not edit.",
            " */",
            "#include <errno.h>",
            "#include <string.h>",
            "#include <libxnvme.h>",
            "",
            "/**",
            " * Copy the caller's record into 'a', zero beyond the caller's size",
            " *",
            " * The size of the caller's record is its first member, 'args_nbytes'.",
            " *",
            " * @param a The record of the function, zero on entry",
            " * @param a_nbytes The size of 'a'",
            " * @param args The caller's record",
            " * @param rsvd The reserved bytes of 'a', as 'nrsvd' pairs of a byte offset and a",
            " * number of bytes",
            " * @param nrsvd The number of pairs in 'rsvd'",
            " *",
            " * @return 0 on success; -EINVAL when 'args' is NULL, its size is less than the",
            " * size of 'args_nbytes', or a reserved byte is not zero; -E2BIG when the caller's",
            " * record is larger than 'a' and the bytes beyond 'a_nbytes' are not zero.",
            " */",
            "static int",
            "prep_args(void *a, size_t a_nbytes, const void *args, const uint16_t *rsvd,",
            "\t  unsigned int nrsvd)",
            "{",
            "\tuint32_t nbytes;",
            "",
            "\tif (!args) {",
            "\t\treturn -EINVAL;",
            "\t}",
            "\tmemcpy(&nbytes, args, sizeof(nbytes));",
            "\tif (nbytes < sizeof(nbytes)) {",
            "\t\treturn -EINVAL;",
            "\t}",
            "\tfor (size_t i = a_nbytes; i < nbytes; ++i) {",
            "\t\tif (((const uint8_t *)args)[i]) {",
            "\t\t\treturn -E2BIG;",
            "\t\t}",
            "\t}",
            "\tmemcpy(a, args, nbytes < a_nbytes ? nbytes : a_nbytes);",
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
            "prep_bits(union xnvme_spec_sqe *sqe, unsigned int cdw, unsigned int bit, unsigned int width,",
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
