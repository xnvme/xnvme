import importlib.util
import re
import shutil
import subprocess
import sys
from pathlib import Path

import pytest

spec_model = pytest.importorskip("senfd.documents.spec")

ROOT = Path(__file__).parents[2]
SCRIPT = ROOT / "toolbox" / "xnvme_spec_headers.py"
module = importlib.util.spec_from_file_location("xnvme_spec_headers", SCRIPT)
headers = importlib.util.module_from_spec(module)
module.loader.exec_module(headers)

FOO = "NVM Express Foo Command Set Specification, Revision 1-0 2025-01-01 Ratified"
BAR = "NVM Express Bar Transport Specification, Revision 1-0 2025-01-01 Ratified"
ZNS = "NVM Express Zoned Namespace Command Set Specification, Revision 1-5 2026-07-31 Ratified"


def source(document=FOO, figure_nr=1):
    return spec_model.Source(document=document, figure_nr=figure_nr, section="1")


def bits(
    range_,
    name=None,
    acronym=None,
    unit="bytes",
    fields=(),
    description="",
    formula=None,
    **links,
):
    """A field with the range 'range_', such as "07:00", or with a formula
    as its range and the 'formula' that senfd makes of the formula. 'links'
    are the links of senfd: 'structure', 'array', 'values_in' and 'cases'."""

    upper, _, lower = range_.partition(":")
    try:
        upper, lower = int(upper), int(lower or upper)
    except ValueError:
        upper = lower = None
    return spec_model.Bits(
        role="reserved" if (name or "").startswith("Reserved") else "field",
        kind="formula" if upper is None else "span",
        formula=formula,
        width=None if upper is None else upper - lower + 1,
        name=name or "",
        acronym=acronym,
        unit=unit,
        upper=upper,
        lower=lower,
        range=range_,
        description=description,
        fields=list(fields),
        **links,
    )


def size_of(fields, unit):
    """The size in bytes of a layout of 'fields' of the 'unit'"""

    uppers = [f.upper for f in fields if f.upper is not None]
    if not uppers:
        return None
    return max(uppers) + 1 if unit == "bytes" else (max(uppers) + 1) // 8


def structure(
    title, fields, kind="data_structure", document=FOO, figure_nr=2, unit="bytes"
):
    """A layout with the 'title' of its figure, named and keyed as senfd does,
    by the part of the title after the name of a command"""

    name = title.split(" - ", 1)[-1]
    return spec_model.Structure(
        title=title,
        name=name,
        key=spec_model.alias(name),
        kind=kind,
        unit=unit,
        size=size_of(fields, unit),
        fields=fields,
        source=source(document, figure_nr),
    )


def register(title, offset, fields, figure_nr, base=None, formula=None):
    """A register of the 'title' "ACRONYM - Name" at the byte 'offset' from
    'base', or at the 'formula' when 'offset' is None"""

    acronym, _, name = title.partition(" - ")
    return spec_model.Structure(
        title=title,
        name=name,
        acronym=acronym,
        key=spec_model.alias(name),
        kind="register",
        unit="bits",
        offset=offset,
        offset_base=base,
        offset_formula=formula,
        size=size_of(fields, "bits"),
        fields=fields,
        source=source(figure_nr=figure_nr),
    )


def value(value_, number, name=None):
    typed = {"value": spec_model.Typed(kind="number", lower=number, upper=number)}
    return spec_model.Entry(value=value_, value_column="value", name=name, typed=typed)


def status(qualifier, code_type, scope, number, name, figure_nr):
    """A table of one status code 'number' of the status code type 'code_type'
    and the 'scope' of senfd, such as "fabrics"; without 'code_type', a table
    of the status code types"""

    return spec_model.Values(
        kind="status_values",
        name="Status Code",
        qualifier=qualifier,
        status_code_type=code_type,
        status_scope=scope,
        columns=["value", "description"],
        entries=[value(f"{number:X}h", number, name)],
        source=source(figure_nr=figure_nr),
    )


@pytest.fixture
def model(tmp_path):
    """A data model of two documents, with every construct that the tests
    use"""

    ding = spec_model.Command(
        name="Ding",
        opcode=0x01,
        sqe=[
            spec_model.Dwords(
                lower=10,
                upper=10,
                fields=[
                    bits("15:00", "Foo Count", "FCNT", unit="bits"),
                    bits("18:16", "Foo Action", "FACT", unit="bits").model_copy(
                        update={
                            "values": [
                                value("0h", 0, "Foo Start"),
                                value("1h", 1, "Foo Stop"),
                                value("3h", 3, "Foo Pause"),
                                # A name of more than eight words
                                value(
                                    "5h",
                                    5,
                                    "Foo Statistics data structure for an Allocated"
                                    " Foo ID",
                                ),
                                # A value without a name has no constant
                                value("2h", 2),
                                # Only a sentence, naming the value of another
                                # field that senfd links
                                value("4h", 4).model_copy(
                                    update={
                                        "description": "List the foos in the"
                                        " FSE:Empty state.",
                                        "refers_to": spec_model.ValueLink(
                                            source=source(figure_nr=30),
                                            field="FS",
                                            value="1h",
                                            acronym="FSE",
                                        ),
                                    }
                                ),
                            ]
                        }
                    ),
                    bits("23:20", "Foo Kind", "FKIND", unit="bits"),
                    bits("31:24", "Foo Step", "FSTEP", unit="bits"),
                ],
                source=source(figure_nr=3),
            ),
            spec_model.Dwords(
                lower=11,
                upper=11,
                fields=[
                    bits(
                        "07:00",
                        "Foo Selector",
                        "FSEL",
                        unit="bits",
                        description="Refer to Figure 23.",
                        values_in=source(figure_nr=23),
                    ),
                    bits(
                        "15:08",
                        "Foo Mode",
                        "FMODE",
                        unit="bits",
                        cases=[
                            spec_model.Case(
                                selector="FACT",
                                when=[spec_model.Selected(name="Foo Start", value=0)],
                                name="Foo Options",
                                values=[value("1h", 1, "Foo Quick")],
                                fields=[
                                    bits("07:06", "Reserved", unit="bits"),
                                    bits(
                                        "05:00", "Foo Level", "FLVL", unit="bits"
                                    ).model_copy(
                                        update={"values": [value("3Fh", 63, "Foo Top")]}
                                    ),
                                ],
                            ),
                            spec_model.Case(
                                selector="FACT", otherwise=True, role="reserved"
                            ),
                        ],
                    ),
                    # A case that is a field of all the bits of the field
                    bits(
                        "16",
                        "Foo Option",
                        "FOPT",
                        unit="bits",
                        cases=[
                            spec_model.Case(
                                selector="FACT",
                                when=[
                                    spec_model.Selected(name="Foo Start", value=0),
                                    spec_model.Selected(name="Foo Pause", value=3),
                                ],
                                name="Foo Hold",
                                acronym="FHOLD",
                            ),
                            spec_model.Case(
                                selector="FACT", otherwise=True, role="reserved"
                            ),
                        ],
                    ),
                ],
                source=source(figure_nr=19),
            ),
            # A dword that two figures describe, each under a condition
            spec_model.Dwords(
                lower=13,
                upper=13,
                condition="FCE is cleared to 0h",
                selected_when=spec_model.Comparison(
                    field="FCE", comparison="==", value=0
                ),
                fields=[
                    bits("31:16", "Foo Specific", "FSPC", unit="bits"),
                    bits("15:08", "Reserved", unit="bits"),
                    bits("07:00", "Foo Dataset Management", "FDSM", unit="bits"),
                ],
                source=source(figure_nr=31),
            ),
            spec_model.Dwords(
                lower=13,
                upper=13,
                condition="FCE is non-zero",
                selected_when=spec_model.Comparison(
                    field="FCE", comparison="!=", value=0
                ),
                fields=[
                    bits("31:16", "Foo Specific", "FSPC", unit="bits"),
                    bits("15:00", "Foo Extension Value", "FCEV", unit="bits"),
                ],
                source=source(figure_nr=32),
            ),
        ],
        operations=[
            spec_model.Operation(
                name="Foo Begin",
                selectors={"FKIND": 1, "FSTEP": 1},
                sqe=[
                    spec_model.Dwords(
                        lower=12,
                        upper=12,
                        fields=[bits("07:00", "Foo Again", "FAGN", unit="bits")],
                        source=source(figure_nr=29),
                    )
                ],
                source=source(figure_nr=16),
            ),
            spec_model.Operation(
                name="Foo End",
                selectors={"FKIND": 1, "FSTEP": 2},
                sqe=[
                    spec_model.Dwords(
                        lower=12,
                        upper=12,
                        fields=[
                            bits("31:16", "Reserved", unit="bits"),
                            bits("15:00", "Foo Requested", "FREQ", unit="bits"),
                        ],
                        source=source(figure_nr=27),
                    )
                ],
                cqe=[
                    spec_model.Dwords(
                        lower=0,
                        upper=0,
                        fields=[bits("07:00", "Foo Granted", "FGOT", unit="bits")],
                        source=source(figure_nr=28),
                    )
                ],
                source=source(figure_nr=16),
            ),
            spec_model.Operation(
                name="Foo Peek",
                selectors={"FKIND": 2, "FSTEP": 1},
                source=source(figure_nr=17),
            ),
            spec_model.Operation(
                name="Foo Resume",
                selectors={"FKIND": 2, "FSTEP": 2},
                sqe=[
                    spec_model.Dwords(
                        lower=12,
                        upper=12,
                        fields=[bits("07:00", "Foo Again", "FAGN", unit="bits")],
                        source=source(figure_nr=29),
                    )
                ],
                source=source(figure_nr=17),
            ),
        ],
        cqe=[
            spec_model.Dwords(
                lower=0,
                upper=0,
                fields=[bits("15:00", "Foo Remaining", "FREM", unit="bits")],
                source=source(figure_nr=33),
            )
        ],
        source=source(figure_nr=1),
    )
    document = spec_model.SpecDocument(
        command_sets={
            "foo": spec_model.CommandSet(
                name="Foo",
                key="foo",
                title="Foo Command Set",
                document="foo",
                kind="io",
                commands={"ding": ding},
            )
        },
        structures=[
            structure(
                "Foo Statistics",
                [
                    bits("07:00", "Foo Count", "FCNT"),
                    bits(
                        "08",
                        "Foo Attributes",
                        "FATTR",
                        fields=[bits("0", "Foo On", "FON", unit="bits")],
                    ),
                    # An array, of which the figure omits rows
                    *[
                        bits(
                            f"{16 + 32 * i + 31}:{16 + 32 * i}",
                            f"Foo Slot {i} Descriptor",
                            array=spec_model.Array(
                                name="Foo Slot Descriptor",
                                index=i,
                                first=0,
                                count=7,
                                stride=32,
                                lower=16,
                            ),
                        )
                        for i in (0, 1, 6)
                    ],
                    bits("247:240", "Foo Version", "FVER", encoding="ascii"),
                ],
            ),
            structure(
                "Foo Table",
                [
                    bits("03:00", "Foo Count", "FCNT"),
                    bits(
                        "07:04",
                        "Foo Table Entry 0",
                        "FTE0",
                        structure=source(figure_nr=20),
                        array=spec_model.Array(
                            name="Foo Table Entry",
                            acronym="FTE",
                            index=0,
                            first=0,
                            count=3,
                            stride=4,
                            lower=4,
                        ),
                    ),
                    bits(
                        "15:12",
                        "Foo Table Entry 2",
                        "FTE2",
                        structure=source(figure_nr=20),
                        array=spec_model.Array(
                            name="Foo Table Entry",
                            acronym="FTE",
                            index=2,
                            first=0,
                            count=3,
                            stride=4,
                            lower=4,
                        ),
                    ),
                ],
                figure_nr=21,
            ),
            structure(
                "Foo - Foo Range List",
                [
                    bits(
                        f"{16 * i + 15}:{16 * i:02}",
                        f"Foo Range {i}",
                        array=spec_model.Array(
                            name="Foo Range",
                            index=i,
                            first=0,
                            count=8,
                            stride=16,
                            lower=0,
                        ),
                        # The figure gives the fields of each record by the
                        # bytes of the record
                        fields=[
                            bits("03:00", "Foo Attributes", "FATTR"),
                            bits("07:04", "Foo Length", "FLEN"),
                            bits("15:08", "Foo Start", "FSTA"),
                        ],
                    )
                    for i in (0, 1, 7)
                ],
                figure_nr=41,
            ),
            structure(
                "Foo Cell",
                [
                    bits("01:00", "Foo Cell Id", "FCID"),
                    bits("03:02", "Foo Cell Size", "FCSZ"),
                ],
                figure_nr=20,
            ),
            structure(
                "Foo Tag Format when FS field is non-zero",
                [
                    bits("01:00", "Guard", "GRD", byte_order="big-endian"),
                    bits("03:02", "Application Tag", "AT", byte_order="big-endian"),
                    bits("04", "Foo Flag", "FFLG"),
                    bits("06:05", "Foo Count", "FCT"),
                ],
                figure_nr=22,
            ),
            structure(
                "Foo PDU",
                [
                    bits("03:00", "Foo Header", "FHDR"),
                    bits(
                        "07:04",
                        "Foo Header Digest",
                        "FHDG",
                        placements=[
                            spec_model.Placement(
                                condition="FHDGF=1", range="07:04", upper=7, lower=4
                            ),
                            spec_model.Placement(condition="FHDGF=0"),
                        ],
                    ),
                    bits(
                        "11:08",
                        "Foo Body",
                        "FBDY",
                        placements=[
                            spec_model.Placement(
                                condition="FHDGF=1", range="11:08", upper=11, lower=8
                            ),
                            spec_model.Placement(
                                condition="FHDGF=0", range="07:04", upper=7, lower=4
                            ),
                        ],
                    ),
                ],
                figure_nr=40,
            ),
            structure(
                "Foo Entry",
                [
                    bits("07:00", "Foo Entry Key", "FEK").model_copy(
                        update={
                            "values": [
                                value("1h", 1, "Foo Primary"),
                                value("2h", 2, "Foo Secondary"),
                            ]
                        }
                    ),
                    bits("31:08", "Reserved"),
                ],
                figure_nr=47,
            ),
            structure(
                "Foo Power Table",
                [
                    bits(
                        "01:00",
                        "Foo Format 0",
                        "FFMT0",
                        description="The format is defined in Figure 15.",
                        structure=source(figure_nr=15),
                        array=spec_model.Array(
                            name="Foo Format",
                            acronym="FFMT",
                            index=0,
                            first=0,
                            count=2,
                            stride=2,
                            lower=0,
                        ),
                    ),
                    bits(
                        "03:02",
                        "Foo Format 1",
                        "FFMT1",
                        description="The format is defined in Figure 15.",
                        structure=source(figure_nr=15),
                        array=spec_model.Array(
                            name="Foo Format",
                            acronym="FFMT",
                            index=1,
                            first=0,
                            count=2,
                            stride=2,
                            lower=0,
                        ),
                    ),
                    bits(
                        "19:04",
                        "Foo Power Descriptor 0",
                        "FPD0",
                        description="The format is defined in Figure 14.",
                        structure=source(figure_nr=14),
                        array=spec_model.Array(
                            name="Foo Power Descriptor",
                            acronym="FPD",
                            index=0,
                            first=0,
                            count=2,
                            stride=16,
                            lower=4,
                        ),
                    ),
                    bits(
                        "35:20",
                        "Foo Power Descriptor 1",
                        "FPD1",
                        description="The format is defined in Figure 14.",
                        structure=source(figure_nr=14),
                        array=spec_model.Array(
                            name="Foo Power Descriptor",
                            acronym="FPD",
                            index=1,
                            first=0,
                            count=2,
                            stride=16,
                            lower=4,
                        ),
                    ),
                    bits(
                        "37:36",
                        "Foo Spare Format 0",
                        "FSF0",
                        structure=source(figure_nr=15),
                        array=spec_model.Array(
                            name="Foo Spare Format",
                            acronym="FSF",
                            index=0,
                            first=0,
                            count=2,
                            stride=2,
                            lower=36,
                        ),
                    ),
                    bits(
                        "39:38",
                        "Foo Spare Format 1",
                        "FSF1",
                        structure=source(figure_nr=15),
                        array=spec_model.Array(
                            name="Foo Spare Format",
                            acronym="FSF",
                            index=1,
                            first=0,
                            count=2,
                            stride=2,
                            lower=36,
                        ),
                    ),
                ],
                figure_nr=18,
            ),
            structure(
                "Foo Power Descriptor",
                [
                    bits("127:120", "Reserved", unit="bits"),
                    bits("119", "Valid", "VAL", unit="bits"),
                    bits("100:96", "Foo Rate", "FRATE", unit="bits"),
                    bits("63:32", "Foo Latency", "FLAT", unit="bits"),
                    bits("25", "Foo Off", "FOFF", unit="bits"),
                    bits("24", "Foo Scale", "FSC", unit="bits"),
                    bits("15:00", "Foo Power", "FPWR", unit="bits"),
                ],
                kind="other",
                figure_nr=14,
                unit="bits",
            ),
            structure(
                "Foo Format",
                [
                    bits(
                        "15:08",
                        "Foo Data Size",
                        "FDS",
                        unit="bits",
                        fields=[bits("01:00", "Foo Data Unit", "FDU", unit="bits")],
                    ),
                    bits("07:00", "Foo Metadata Size", "FMS", unit="bits"),
                ],
                kind="other",
                figure_nr=15,
                unit="bits",
            ),
            structure(
                "Foo Directives",
                [
                    bits(
                        "15:00",
                        "Foo Supported",
                        fields=[
                            bits("127:10", "Reserved", unit="bits"),
                            bits("09", "Foo Bar Supported", "FBARS", unit="bits"),
                            bits("00", "Foo Baz Supported", "FBAZS", unit="bits"),
                        ],
                    ),
                    # A string is bytes whatever bits it has
                    bits(
                        "31:16",
                        "Foo Label",
                        encoding="ascii",
                        fields=[bits("00", "Foo Label Bit", "FLB", unit="bits")],
                    ),
                ],
            ),
            structure(
                "Foo Descriptor",
                [
                    bits("07:00", "Foo Sub Type", "FSUBT").model_copy(
                        update={
                            "values": [
                                # A value of a table by the value and the
                                # values of another field, named before a colon
                                value("0h", 0).model_copy(
                                    update={
                                        "columns": {
                                            "types": "0h, 2h",
                                            "meaning": "Foo Address: The Address"
                                            " field specifies the start, in bytes,"
                                            " of the foo.",
                                        }
                                    }
                                ),
                                value("0h", 0).model_copy(
                                    update={
                                        "columns": {
                                            "types": "1h",
                                            "meaning": "For Type 1h, the Sub Type"
                                            " field shall be cleared to 0h.",
                                        }
                                    }
                                ),
                                value("0h", 0).model_copy(
                                    update={
                                        "columns": {
                                            "types": "All other values",
                                            "meaning": "Reserved",
                                        }
                                    }
                                ),
                                value("1h", 1).model_copy(
                                    update={
                                        "columns": {
                                            "types": "0h to 2h",
                                            "meaning": "Foo Offset: The Address"
                                            " field contains an offset.",
                                        }
                                    }
                                ),
                                # A name of the form of a value
                                value("3h", 3).model_copy(
                                    update={"columns": {"meaning": "512B"}}
                                ),
                            ]
                        }
                    ),
                ],
            ),
            structure(
                "Foo Twice",
                [
                    bits("03:00", "Foo Code", "FC"),
                    bits("05:04", "Foo Mode", "FM"),
                    bits("11:08", "Foo Code", "FC"),
                    bits("15:12", "Foo Mode", "FM"),
                ],
            ),
            structure(
                "Foo List",
                [
                    bits("03:00", "Number of Foo Entries", "NFE"),
                    *[
                        bits(
                            range_,
                            f"Foo Entry {index}",
                            formula=formula,
                            structure=source(figure_nr=47),
                            array=spec_model.Array(
                                name="Foo Entry",
                                index=index if isinstance(index, int) else None,
                                first=1,
                                stride=32,
                                lower=16,
                                counted_by="NFE",
                            ),
                        )
                        for range_, index, formula in (
                            ("47:16", 1, None),
                            ("79:48", 2, None),
                            (
                                "(NFE*32)+15:(NFE*32)-16",
                                "NFE",
                                "32*NFE + 15:32*NFE - 16",
                            ),
                        )
                    ],
                ],
            ),
            structure(
                "Foo Dword 0",
                [
                    bits("31:16", "Foo Tag", "FTAG", unit="bits"),
                    bits("07:00", "Foo Opcode", "FOPC", unit="bits"),
                ],
                kind="command_dword",
                figure_nr=11,
                unit="bits",
            ),
            structure(
                "Foo Frame",
                [
                    bits(
                        "03:00",
                        "Foo Dword 0",
                        "FDW0",
                        description="Defined in Figure 11.",
                        structure=source(figure_nr=11),
                        # senfd gives the fields of the layout it links to
                        fields=[
                            bits("31:16", "Foo Tag", "FTAG", unit="bits"),
                            bits("07:00", "Foo Opcode", "FOPC", unit="bits"),
                        ],
                    ),
                    bits(
                        "07:04",
                        "Foo Pointer",
                        "FPTR",
                        description="Used if FDW0.FOPC (refer to Figure 11) is set.",
                    ),
                ],
                kind="other",
            ),
            register(
                "FOOCAP - Foo Capabilities",
                0,
                [
                    bits("63:32", "Reserved", unit="bits"),
                    bits("15:00", "Foo Limit", "FL", unit="bits"),
                ],
                8,
            ),
            register(
                "FOOCC - Foo Configuration",
                0x10,
                [
                    bits(
                        "02:00",
                        "Foo Mode Selected",
                        "FMS",
                        unit="bits",
                    ).model_copy(
                        update={
                            "values": [
                                value("000b", 0, "Foo Fast"),
                                value("001b", 1),
                                value("111b", 7, "Reserved").model_copy(
                                    update={"role": "reserved"}
                                ),
                            ]
                        }
                    ),
                    bits("31:03", "Reserved", unit="bits"),
                ],
                9,
            ),
            register(
                "FOOVS - Foo Version",
                0x18,
                [
                    bits("31:16", "Foo Major", "FMJR", unit="bits"),
                    bits("15:00", "Foo Minor", "FMNR", unit="bits"),
                ],
                42,
            ),
            # A register of a capability, at its offset from the capability
            register(
                "FOOPMC - Foo Power Management Capabilities",
                2,
                [bits("15:00", "Foo Power Version", "FPV", unit="bits")],
                48,
                base="FOOPMCAP",
            ),
            # A register at an offset of a formula
            register(
                "FOODB - Foo Doorbell",
                None,
                [bits("15:00", "Foo Tail", "FTL", unit="bits")],
                49,
                formula="1000h + (4 * y)",
            ),
            # Fields that report a register, linked to its bit layout
            structure(
                "Foo Identify",
                [
                    bits(
                        "03:00",
                        "Foo Version",
                        "FVER",
                        structure=source(figure_nr=42),
                        fields=[
                            bits("31:16", "Foo Major", "FMJR", unit="bits"),
                            bits("15:00", "Foo Minor", "FMNR", unit="bits"),
                        ],
                    ),
                    bits("11:04", "Foo Wide", "FWIDE", structure=source(figure_nr=42)),
                ],
            ),
            # A field of bytes that another field gives as other fields of
            # bytes, at their bytes in the structure
            structure(
                "Foo Command",
                [
                    bits("07:00", "Foo Header", "FHDR"),
                    bits(
                        "23:08",
                        "Foo Data Pointer",
                        "FDPTR",
                        cases=[
                            spec_model.Case(
                                selector="FPSDT",
                                when=[spec_model.Selected(name="00b", value=0)],
                                # A PRP entry links to a bit layout, whose
                                # split depends on the memory page size; the
                                # bytes of a case are of the field
                                fields=[
                                    bits(
                                        "15:08",
                                        "Foo PRP Entry 2",
                                        "FPRP2",
                                        structure=source(figure_nr=44),
                                    ),
                                    bits(
                                        "07:00",
                                        "Foo PRP Entry 1",
                                        "FPRP1",
                                        structure=source(figure_nr=44),
                                    ),
                                ],
                            ),
                            spec_model.Case(
                                selector="FPSDT",
                                when=[
                                    spec_model.Selected(name="01b", value=1),
                                    spec_model.Selected(name="10b", value=2),
                                ],
                                fields=[
                                    bits(
                                        "15:00",
                                        "Foo SGL Entry 1",
                                        "FSGL1",
                                        structure=source(figure_nr=43),
                                    )
                                ],
                            ),
                            spec_model.Case(
                                selector="FPSDT", otherwise=True, role="reserved"
                            ),
                        ],
                    ),
                ],
            ),
            # An array of as many entries as the data gives, up to a maximum
            # that the specification states
            structure(
                "Foo Changes",
                [
                    bits("01:00", "Number of Foo Changes", "NFC"),
                    bits("07:02", "Reserved"),
                    *[
                        bits(
                            range_,
                            f"Foo Change {index}",
                            formula=formula,
                            array=spec_model.Array(
                                name="Foo Change",
                                index=index if isinstance(index, int) else None,
                                first=0,
                                stride=8,
                                lower=8,
                                maximum=3,
                                counted_by="NFC",
                            ),
                        )
                        for range_, index, formula in (
                            ("15:08", 0, None),
                            ("23:16", 1, None),
                            (
                                "((NFC-1)*8+15):((NFC-1)*8+8)",
                                "NFC-1",
                                "8*NFC + 7:8*NFC",
                            ),
                        )
                    ],
                ],
                kind="log_page",
            ),
            # Fields of bytes that link to the record laying them out, ahead
            # of the record
            structure(
                "Foo Fabrics Command",
                [
                    bits("15:00", "Foo SGL Descriptor 1", "FSGL1").model_copy(
                        update={"structure": source(figure_nr=43)}
                    ),
                    bits("31:16", "Foo Host Identifier", "FHID").model_copy(
                        update={"structure": source(figure_nr=45)}
                    ),
                    bits("47:32", "Bar Key", "BKEY").model_copy(
                        update={"structure": source(document=BAR, figure_nr=3)}
                    ),
                    bits("59:48", "Foo Short", "FSHRT").model_copy(
                        update={"structure": source(figure_nr=43)}
                    ),
                ],
                figure_nr=46,
            ),
            structure(
                "Foo SGL Descriptor",
                [
                    bits("14:00", "Foo Descriptor Specific", "FDS"),
                    bits("15", "Foo SGL Identifier", "FSGLID"),
                ],
                kind="other",
                figure_nr=43,
            ),
            structure(
                "Foo PRP Entry",
                [
                    bits("63:02", "Foo Page Base Address", "FPBA", unit="bits"),
                    bits("01:00", "Reserved", unit="bits"),
                ],
                kind="other",
                figure_nr=44,
                unit="bits",
            ),
            structure(
                "Foo Host Identifier",
                [bits("15:00", "Foo Host Identifier", "FHID")],
                figure_nr=45,
            ),
            structure(
                "Bar Key",
                [
                    bits("07:00", "Bar Key Low", "BKL"),
                    bits("15:08", "Bar Key High", "BKH"),
                ],
                kind="other",
                document=BAR,
                figure_nr=3,
            ),
        ],
        values=[
            spec_model.Values(
                kind="other",
                name="Foo Selector",
                columns=["value", "description"],
                entries=[value("0h", 0, "Foo Near"), value("1h", 1, "Foo Far")],
                source=source(figure_nr=23),
            ),
            status("Generic Command", 0, None, 0xC0, "Foo Invalid", 4),
            status("Generic Command", 0, "fabrics", 0xC0, "Foo Clash", 5),
        ],
        features={
            "foo_config": spec_model.Feature(
                identifier=0xC0,
                name="Foo Config",
                source=source(figure_nr=6),
                sqe=[
                    spec_model.Dwords(
                        lower=11,
                        upper=11,
                        fields=[
                            bits("31:18", "Reserved", None, unit="bits"),
                            bits("17:16", "Foo Level", "FLV", unit="bits").model_copy(
                                update={
                                    "values": [
                                        value("0h", 0, "Foo Low"),
                                        value("1h", 1, "Foo High"),
                                    ]
                                }
                            ),
                            bits("15:00", "Foo Limit", "FLIM", unit="bits"),
                        ],
                        source=source(figure_nr=24),
                    ),
                    # A figure of another feature that the data model also
                    # lists here has no mask where it overlaps
                    spec_model.Dwords(
                        lower=11,
                        upper=11,
                        fields=[bits("00", "Foo Other", "FOTH", unit="bits")],
                        source=source(figure_nr=25),
                    ),
                ],
                cqe=[
                    spec_model.Dwords(
                        lower=0,
                        upper=0,
                        fields=[bits("15:00", "Foo Granted", "FGNT", unit="bits")],
                        source=source(figure_nr=26),
                    )
                ],
                # An operation of the feature, which FLV selects
                operations=[
                    spec_model.Operation(
                        name="Foo Keep",
                        selectors={"FLV": 1},
                        sqe=[
                            spec_model.Dwords(
                                lower=12,
                                upper=12,
                                fields=[
                                    bits("00", "Foo Keep Enable", "FKE", unit="bits")
                                ],
                                source=source(figure_nr=92),
                            )
                        ],
                        source=source(figure_nr=92),
                    )
                ],
            )
        },
        log_pages={
            "foo_events": spec_model.LogPage(
                identifier=0xC1, name="Foo Events", source=source(figure_nr=7)
            )
        },
        meta=spec_model.DocumentMeta(version="9.8.7"),
    )
    path = tmp_path / "merged.spec.document.json"
    path.write_text(document.model_dump_json(), encoding="utf-8")
    return path


def generate(model, output, **options):
    return headers.SpecHeaders(output, {"spec": str(model), **options}).generate()


def text_of(wrote, stem):
    (path,) = [p for p in wrote.headers + wrote.sources if p.name == stem]
    return path.read_text(encoding="utf-8")


def test_each_document_has_a_header_named_by_its_abbreviation(model, tmp_path):
    wrote = generate(model, tmp_path / "out")
    assert sorted(p.name for p in wrote.headers) == [
        "libxnvme_spec_bar_transport.h",
        "libxnvme_spec_foo.h",
    ]
    # The command preps make the commands, so there are no sources
    assert wrote.sources == []
    text = text_of(wrote, "libxnvme_spec_foo.h")
    assert "#ifndef LIBXNVME_SPEC_FOO_H" in text
    # The header says what wrote it, and that it is not to be edited
    # The version is of the senfd that wrote the data model
    assert " * Written from the data model of senfd 9.8.7, by" in text
    assert " * edit: see the contributor docs on producing the spec headers." in text


def test_a_data_model_of_another_format_stops_the_generator(model, tmp_path):
    document = spec_model.SpecDocument.model_validate_json(model.read_text())
    document.meta.format = headers.FORMAT - 1
    model.write_text(document.model_dump_json(), encoding="utf-8")
    with pytest.raises(ValueError, match="the generators read format"):
        generate(model, tmp_path / "out")


@pytest.mark.parametrize(
    "title, key",
    [
        ("NVM Express Base Specification, Revision 2.4", "base"),
        ("NVM Express Key Value Command Set Specification, Revision 1.4", "kv"),
        ("NVM Express NVMe over PCIe Transport Specification, Revision 1.4", "pcie"),
        ("NVM Express NVMe over RDMA Transport Specification, Revision 1.3", "rdma"),
        ("NVM Express NVMe over TCP Transport Specification, Revision 1.3", "tcp"),
    ],
)
def test_a_document_key_is_the_abbreviation_without_the_revision(title, key):
    assert headers.document_key(title) == key


def test_a_command_is_its_masks_and_its_opcode_without_a_struct(model, tmp_path):
    wrote = generate(model, tmp_path / "out")
    text = text_of(wrote, "libxnvme_spec_foo.h")

    assert "enum xnvme_spec_foo_opcode {" in text
    assert "\tXNVME_SPEC_FOO_DING_OPCODE = 0x1, ///< Ding (Foo 1.0" in text
    # union xnvme_cmd_sqe holds the dwords of every command
    assert "struct xnvme_spec_foo_ding" not in text
    assert " * The fields of Ding, by the dwords of the command" in text
    assert "#define XNVME_SPEC_FOO_DING_CDW10_FCNT 0xffff" in text
    assert "of Command Dword 10 */" in text
    assert "_init(" not in text


def test_a_structure_is_a_packed_struct_at_the_offsets_of_its_fields(model, tmp_path):
    text = text_of(generate(model, tmp_path / "out"), "libxnvme_spec_foo.h")

    assert "struct __attribute__((packed)) xnvme_spec_foo_foo_statistics {" in text
    assert "\tuint64_t fcnt; ///< Foo Count, bytes 7:0" in text
    assert "#define XNVME_SPEC_FOO_FOO_STATISTICS_FATTR_FON 0x1" in text
    # A gap is a member named by its offset; a run of fields named by index is
    # an array, even when the figure omits rows
    assert "\tuint8_t rsvd9[7]; ///< bytes 15:9, not in the figure" in text
    assert "\tuint8_t foo_slot_descriptor[224]; ///< Foo Slot Descriptor, 7 of 32" in (
        text
    )
    # A string is an array of bytes whatever its size
    assert "\tuint8_t fver[8]; ///< Foo Version, bytes 247:240" in text


def test_entries_that_a_field_counts_follow_the_struct(model, tmp_path):
    text = text_of(generate(model, tmp_path / "out"), "libxnvme_spec_foo.h")

    assert (
        " * Followed, from byte 16, by Foo Entries, counted by NFE, each a"
        " 'struct xnvme_spec_foo_foo_entry'" in text
    )
    assert text.index("xnvme_spec_foo_foo_entry {") < text.index(
        "xnvme_spec_foo_foo_list {"
    )
    text = text_of(
        generate(model, tmp_path / "flexible", flexible="1"), "libxnvme_spec_foo.h"
    )
    assert "\tstruct xnvme_spec_foo_foo_entry foo_entry[]; ///< Foo Entry" in text


def test_an_array_of_a_maximum_is_of_that_many_entries(model, tmp_path):
    text = text_of(generate(model, tmp_path / "out"), "libxnvme_spec_foo.h")

    start = text.index(
        "struct __attribute__((packed)) xnvme_spec_foo_log_foo_changes {"
    )
    struct = text[text.rindex("/**", 0, start) : text.index("};", start)]
    assert "\tuint16_t nfc; ///< Number of Foo Changes, bytes 1:0" in struct
    assert (
        "\tuint64_t foo_change[3]; ///< Foo Change, up to 3 of 8 bytes, counted by NFC,"
        " bytes 31:8" in struct
    )
    assert "Followed" not in struct


def test_a_field_name_that_occurs_twice_has_its_offset_as_a_suffix(model, tmp_path):
    text = text_of(generate(model, tmp_path / "out"), "libxnvme_spec_foo.h")

    assert "\tuint32_t fc_8; ///< Foo Code, bytes 11:8" in text


def test_a_figure_citing_fields_of_other_structures_is_no_struct(tmp_path):
    path = tmp_path / "spec.json"
    path.write_text(
        spec_model.SpecDocument(
            structures=[
                structure(
                    "Size and Capacity Fields",
                    [bits("07:00", "Namespace Size"), bits("15:08", "Namespace")],
                    kind="cited_fields",
                    document=ZNS,
                ),
                structure(
                    "Zone Thing", [bits("07:00", "Zone Key", "ZK")], document=ZNS
                ),
            ]
        ).model_dump_json()
    )

    text = text_of(generate(path, tmp_path / "out"), "libxnvme_spec_zns.h")
    assert "size_and_capacity_fields" not in text
    assert "xnvme_spec_zns_zone_thing" in text

    named = generate(
        path, tmp_path / "named", structures="zns:size_and_capacity_fields"
    )
    assert "xnvme_spec_zns_size_and_capacity_fields" in text_of(
        named, "libxnvme_spec_zns.h"
    )


@pytest.mark.parametrize(
    "kind", ["example", "status_values", "cited_fields", "mapping"]
)
def test_a_structure_of_a_skipped_kind_is_written_only_by_its_key(tmp_path, kind):
    path = tmp_path / "spec.json"
    path.write_text(
        spec_model.SpecDocument(
            structures=[
                structure("Foo Skipped", [bits("07:00", "Foo Skip", "FSK")], kind=kind),
                structure(
                    "Foo Kept", [bits("07:00", "Foo Keep", "FKP")], kind="data_buffer"
                ),
            ]
        ).model_dump_json()
    )

    text = text_of(generate(path, tmp_path / "out"), "libxnvme_spec_foo.h")
    assert "xnvme_spec_foo_foo_skipped" not in text
    assert "struct __attribute__((packed)) xnvme_spec_foo_foo_kept {" in text

    named = generate(path, tmp_path / "named", structures="foo:foo_skipped")
    assert "xnvme_spec_foo_foo_skipped {" in text_of(named, "libxnvme_spec_foo.h")


def test_identifiers_and_status_codes_are_enums_by_document_and_scope(model, tmp_path):
    text = text_of(generate(model, tmp_path / "out"), "libxnvme_spec_foo.h")

    assert "\tXNVME_SPEC_FOO_FEATURE_FOO_CONFIG = 0xc0," in text
    assert "\tXNVME_SPEC_FOO_LOG_PAGE_FOO_EVENTS = 0xc1," in text
    # A value with a different meaning in each of two scopes is in two enums
    assert "enum xnvme_spec_foo_generic_command_status {" in text
    assert "\tXNVME_SPEC_FOO_GENERIC_COMMAND_STATUS_FOO_INVALID = 0xc0," in text
    assert "enum xnvme_spec_foo_generic_command_fabrics_status {" in text
    assert "\tXNVME_SPEC_FOO_GENERIC_COMMAND_FABRICS_STATUS_FOO_CLASH = 0xc0," in text


def test_a_field_has_the_masks_of_the_layout_it_links_to(model, tmp_path):
    text = text_of(generate(model, tmp_path / "out"), "libxnvme_spec_foo.h")

    assert "#define XNVME_SPEC_FOO_FOO_FRAME_FDW0_FTAG 0xffff0000" in text
    assert "#define XNVME_SPEC_FOO_FOO_FRAME_FDW0_FOPC 0xff" in text
    # The masks cite the figure of the layout, not that of Foo Frame
    assert (
        "/** Foo Tag (Foo 1.0, 1, Figure 11); bits 31:16 of 'fdw0' */\n"
        "#define XNVME_SPEC_FOO_FOO_FRAME_FDW0_FTAG" in text
    )
    # A field whose description mentions a figure without a link has no masks
    # of that figure
    assert "XNVME_SPEC_FOO_FOO_FRAME_FPTR_" not in text


def test_a_field_has_the_masks_of_the_register_it_links_to(model, tmp_path):
    text = text_of(generate(model, tmp_path / "out"), "libxnvme_spec_foo.h")

    assert (
        "\tuint32_t fver; ///< Foo Version, bytes 3:0, see"
        " XNVME_SPEC_FOO_FOO_IDENTIFY_FVER_*" in text
    )
    assert "#define XNVME_SPEC_FOO_FOO_IDENTIFY_FVER_FMJR 0xffff0000" in text
    assert "#define XNVME_SPEC_FOO_FOO_IDENTIFY_FVER_FMNR 0xffff" in text
    # The register has its own masks as well
    assert "#define XNVME_SPEC_FOO_REGISTERS_FOOVS_FMJR 0xffff0000" in text
    # A link without the fields of the layout gives no masks
    assert "XNVME_SPEC_FOO_FOO_IDENTIFY_FWIDE_" not in text


def structures_only(tmp_path, structures):
    """A data model of 'structures' alone"""

    document = spec_model.SpecDocument(
        structures=structures, meta=spec_model.DocumentMeta(version="9.8.7")
    )
    path = tmp_path / "structures.spec.document.json"
    path.write_text(document.model_dump_json(), encoding="utf-8")
    return path


BASE = "NVM Express Base Specification, Revision 2.4 2026-07-31 Ratified"


def test_the_dwords_of_the_completion_queue_entry_are_their_halves(tmp_path):
    def dword(nr, fields=()):
        return bits(
            f"{4 * nr + 3}:{4 * nr}",
            f"Dword {nr}",
            f"DW{nr}",
            structure=source(BASE, 90 + nr) if fields else None,
            fields=fields,
        )

    status = bits(
        "31:17",
        "Status",
        "STATUS",
        unit="bits",
        fields=[
            bits("14", "Do Not Retry", "DNR", unit="bits"),
            bits("07:00", "Status Code", "SC", unit="bits"),
        ],
    )
    cqe = structure(
        "Completion Queue Entry",
        [
            dword(0),
            dword(1),
            dword(
                2,
                [
                    bits("31:16", "SQ Identifier", "SQID", unit="bits"),
                    bits("15:00", "SQ Head Pointer", "SQHD", unit="bits"),
                ],
            ),
            dword(
                3,
                [
                    status,
                    bits("16", "Phase Tag", "P", unit="bits"),
                    bits("15:00", "Command Identifier", "CID", unit="bits"),
                ],
            ),
        ],
        document=BASE,
    )
    model = structures_only(tmp_path, [cqe])
    text = text_of(generate(model, tmp_path / "out"), "libxnvme_spec_base.h")

    assert "\tuint32_t dw0; ///< Dword 0, bytes 3:0" in text
    assert "\tuint16_t sqhd; ///< SQ Head Pointer, bytes 9:8" in text
    assert "\tuint16_t sqid; ///< SQ Identifier, bytes 11:10" in text
    assert "\tuint16_t cid; ///< Command Identifier, bytes 13:12" in text
    assert (
        "\tuint16_t status; ///< Status and Phase Tag, bytes 15:14, see"
        " XNVME_SPEC_BASE_COMPLETION_QUEUE_ENTRY_STATUS_*" in text
    )
    # The masks of the fields of a field come first, then those of the rest
    assert "#define XNVME_SPEC_BASE_COMPLETION_QUEUE_ENTRY_STATUS_DNR 0x8000" in text
    assert "#define XNVME_SPEC_BASE_COMPLETION_QUEUE_ENTRY_STATUS_SC 0x1fe" in text
    assert "#define XNVME_SPEC_BASE_COMPLETION_QUEUE_ENTRY_STATUS_P 0x1" in text
    assert "_STATUS_STATUS" not in text
    # A member that one field fills has no mask
    assert "XNVME_SPEC_BASE_COMPLETION_QUEUE_ENTRY_CID_" not in text


def test_a_dword_of_another_layout_is_one_member_with_masks(tmp_path):
    reply = structure(
        "Foo Reply",
        [
            bits("03:00", "Dword 0", "DW0"),
            bits(
                "07:04",
                "Dword 1",
                "DW1",
                fields=[
                    bits("31:16", "Foo Status", "FSTS", unit="bits"),
                    bits("15:00", "Foo Tag", "FTAG", unit="bits"),
                ],
            ),
        ],
    )
    model = structures_only(tmp_path, [reply])
    text = text_of(generate(model, tmp_path / "out"), "libxnvme_spec_foo.h")

    assert "\tuint32_t dw1; ///< Dword 1, bytes 7:4, see" in text
    assert "#define XNVME_SPEC_FOO_FOO_REPLY_DW1_FTAG 0xffff" in text


def test_an_array_numbered_from_another_index_is_a_member_for_each_entry(tmp_path):
    def dword(nr):
        return bits(
            f"{4 * nr + 3}:{4 * nr}",
            f"Foo Dword {nr}",
            f"FDW{nr}",
            array=spec_model.Array(
                name="Foo Dword",
                acronym="FDW",
                index=nr,
                first=2,
                count=2,
                stride=4,
                lower=8,
            ),
        )

    model = structures_only(
        tmp_path,
        [
            structure(
                "Foo Words", [bits("07:00", "Foo Head", "FHD"), dword(2), dword(3)]
            )
        ],
    )
    text = text_of(generate(model, tmp_path / "out"), "libxnvme_spec_foo.h")
    assert "\tuint32_t fdw2; ///< Foo Dword 2" in text
    assert "\tuint32_t fdw3; ///< Foo Dword 3" in text
    assert "fdw[" not in text


def test_two_structures_of_one_key_are_named_by_the_figure_of_the_second(tmp_path):
    model = structures_only(
        tmp_path,
        [
            structure("Foo Stamp", [bits("07:00", "Foo When", "FWHN")]),
            structure(
                "Foo Stamp",
                [bits("07:00", "Foo When", "FWHN"), bits("08", "Foo Late", "FLT")],
                figure_nr=3,
            ),
        ],
    )
    text = text_of(generate(model, tmp_path / "out"), "libxnvme_spec_foo.h")
    assert "struct __attribute__((packed)) xnvme_spec_foo_foo_stamp {" in text
    assert "struct __attribute__((packed)) xnvme_spec_foo_foo_stamp_figure3 {" in text


def test_io_is_one_word_in_names():
    assert headers.alias("Create I/O Completion Queue") == "create_io_completion_queue"
    assert headers.identifier("I/O Command Set") == "IO_COMMAND_SET"

    def command_set(key, kind, document=None):
        return spec_model.CommandSet(
            name=key, key=key, kind=kind, document=document, commands={}
        )

    # An I/O command set is named by its document
    assert headers.set_key_of(command_set("key_value", "io", "kv")) == "kv"
    assert headers.set_key_of(command_set("common_io", "common_io")) == "io"
    assert headers.set_key_of(command_set("pcie_commands", "other")) == "pcie_commands"


def test_an_acronym_that_begins_with_digits_has_them_at_its_end():
    assert headers.part_of("128CCS") == "CCS128"
    assert headers.part_of("CDW10") == "CDW10"


def test_a_member_named_by_a_name_that_begins_with_digits_has_them_at_its_end(
    tmp_path,
):
    rows = [
        bits("00", "Foo Count", "FCNT"),
        bits("02:01", "2-Wire Address"),
        bits("FCNT+2:03", "2-Wire Pointer", formula="FCNT + 2:3"),
    ]
    model = structures_only(tmp_path, [structure("Foo Wires", rows)])
    text = text_of(
        generate(model, tmp_path / "out", flexible="1"), "libxnvme_spec_foo.h"
    )

    assert "\tuint16_t wire_address_2; ///< 2-Wire Address, bytes 2:1\n" in text
    assert "\tuint8_t wire_pointer_2[]; ///< 2-Wire Pointer, from byte 3\n" in text
    assert headers.member_name("Foo 2-Wire") == "foo_2_wire"


def test_the_options_select_what_is_written(model, tmp_path):
    wrote = generate(
        model, tmp_path / "out", commands="none", structures="none", registers="none"
    )
    text = text_of(wrote, "libxnvme_spec_foo.h")
    assert "struct" not in text
    assert wrote.sources == []


def test_yace_parses_the_headers_without_a_diagnostic(model, tmp_path):
    pytest.importorskip("yace.generators")
    if shutil.which("cc") is None and shutil.which("gcc") is None:
        pytest.skip("no C compiler")
    proc = subprocess.run(
        [
            sys.executable,
            "-m",
            "yace",
            "--generate",
            f"{SCRIPT}:SpecHeaders",
            "--generator-option",
            f"xnvme_spec.spec={model}",
            "--output",
            str(tmp_path / "out"),
        ],
        capture_output=True,
        text=True,
    )
    assert proc.returncode == 0, proc.stderr
    (status,) = (tmp_path / "out").glob("*.status")
    assert ", 0 diagnostics," in status.read_text()


def test_the_registers_are_a_struct_at_their_offsets_with_masks_and_values(
    model, tmp_path
):
    text = text_of(generate(model, tmp_path / "out"), "libxnvme_spec_foo.h")

    assert "struct __attribute__((packed)) xnvme_spec_foo_registers {" in text
    # A register has the width of its fields and is at its offset; the bytes
    # between registers are reserved
    assert "\tuint64_t foocap; ///< FOOCAP - Foo Capabilities (Foo 1.0" in text
    assert "\tuint8_t rsvd8[8]; ///< Reserved, 8h to Fh" in text
    assert "\tuint32_t foocc; ///< FOOCC - Foo Configuration" in text
    assert "#define XNVME_SPEC_FOO_REGISTERS_FOOCC_FMS 0x7" in text
    # The named values of a field are an enum; reserved and unnamed values have
    # no constant
    assert "enum xnvme_spec_foo_fms {" in text
    assert "\tXNVME_SPEC_FOO_FMS_FOO_FAST = 0x0," in text
    assert "_FMS_RESERVED" not in text
    # The registers of a capability are a struct at their offsets from it
    assert "struct __attribute__((packed)) xnvme_spec_foo_foopmcap_registers {" in text
    assert "\tuint8_t rsvd0[2]; ///< Reserved, 0h to 1h" in text
    assert "\tuint16_t foopmc; ///< FOOPMC - Foo Power Management Capabilities" in text


def test_a_register_at_an_offset_of_a_formula_is_no_member(model, tmp_path):
    text = text_of(generate(model, tmp_path / "out"), "libxnvme_spec_foo.h")

    start = text.index("struct __attribute__((packed)) xnvme_spec_foo_registers {")
    doc = text[text.rindex("/**", 0, start) : start]
    assert " * FOODB - Foo Doorbell (Foo 1.0, 1, Figure 49): at 1000h + (4 * y)" in doc
    assert "foodb;" not in text


def test_registers_of_the_same_bytes_stop_the_generator(tmp_path):
    fields = [bits("31:00", "Foo Elsewhere", "FEW", unit="bits")]
    model = structures_only(
        tmp_path,
        [
            register("FOOA - Foo A", 0, fields, 10),
            register("FOOB - Foo B", 2, fields, 11),
        ],
    )
    with pytest.raises(ValueError, match="FOOB - Foo B: of bytes that another"):
        generate(model, tmp_path / "out")


def test_a_field_of_the_bytes_of_a_field_before_it_stops_the_generator(tmp_path):
    model = structures_only(
        tmp_path,
        [
            structure(
                "Foo Overlap",
                [bits("07:00", "Foo When", "FWHN"), bits("11:04", "Foo Late", "FLT")],
            )
        ],
    )
    with pytest.raises(ValueError, match="Foo Overlap: 'Foo Late' is of bytes"):
        generate(model, tmp_path / "out")


def test_a_case_whose_fields_do_not_fill_the_field_stops_the_generator(tmp_path):
    case = spec_model.Case(
        selector="FPSDT",
        when=[spec_model.Selected(name="00b", value=0)],
        fields=[bits("15:12", "Foo Late", "FLT"), bits("03:00", "Foo Early", "FEY")],
    )
    model = structures_only(
        tmp_path,
        [
            structure(
                "Foo Command", [bits("15:00", "Foo Pointer", "FPTR", cases=[case])]
            )
        ],
    )
    with pytest.raises(ValueError, match="Foo Command: the fields of Foo Pointer"):
        generate(model, tmp_path / "out")


def test_the_option_registers_none_writes_no_registers(model, tmp_path):
    text = text_of(
        generate(model, tmp_path / "out", registers="none"), "libxnvme_spec_foo.h"
    )
    assert "_registers" not in text


def test_the_named_values_of_a_field_are_an_enum_by_its_mask(model, tmp_path):
    text = text_of(generate(model, tmp_path / "out"), "libxnvme_spec_foo.h")

    # Of a command; the values of an acronym are named by the document and the
    # acronym, as the acronym has the same values wherever the document has it
    assert "enum xnvme_spec_foo_fact {" in text
    assert "\tXNVME_SPEC_FOO_FACT_FOO_START = 0x0," in text
    assert "\tXNVME_SPEC_FOO_FACT_FOO_STOP = 0x1," in text
    assert "\tXNVME_SPEC_FOO_FACT_FOO_PAUSE = 0x3," in text
    assert not re.search(r"_FACT_\w+ = 0x2,", text)
    # A name of more than eight words
    assert (
        "\tXNVME_SPEC_FOO_FACT_FOO_STATISTICS_DATA_STRUCTURE_FOR_AN"
        "_ALLOCATED_FOO_ID = 0x5," in text
    )
    # By the acronym of the value of another field that it refers to
    assert (
        "\tXNVME_SPEC_FOO_FACT_FSE = 0x4,"
        " ///< List the foos in the FSE:Empty state. (Foo 1.0" in text
    )
    # Of a field that is the whole member of a structure
    assert "enum xnvme_spec_foo_fek {" in text
    assert "\tXNVME_SPEC_FOO_FEK_FOO_PRIMARY = 0x1," in text


def test_operations_that_two_fields_select_are_an_enum_by_the_first(model, tmp_path):
    text = text_of(generate(model, tmp_path / "out"), "libxnvme_spec_foo.h")

    assert "enum xnvme_spec_foo_ding_cdw10_fstep_fkind1h {" in text
    assert "\tXNVME_SPEC_FOO_DING_CDW10_FSTEP_FKIND1H_FOO_BEGIN = 0x1," in text
    assert "\tXNVME_SPEC_FOO_DING_CDW10_FSTEP_FKIND1H_FOO_END = 0x2," in text
    assert "enum xnvme_spec_foo_ding_cdw10_fstep_fkind2h {" in text
    assert "\tXNVME_SPEC_FOO_DING_CDW10_FSTEP_FKIND2H_FOO_PEEK = 0x1," in text


def test_the_dwords_of_an_operation_are_masks_by_command_and_operation(model, tmp_path):
    text = text_of(generate(model, tmp_path / "out"), "libxnvme_spec_foo.h")

    assert (
        "/** Foo Requested (Foo 1.0, 1, Figure 27); bits 15:0 of Command Dword 12 of"
        " Foo End */\n#define XNVME_SPEC_FOO_DING_FOO_END_CDW12_FREQ 0xffff" in text
    )
    assert "#define XNVME_SPEC_FOO_DING_FOO_END_CQE_DW0_FGOT 0xff" in text
    assert "bits 7:0 of Completion Queue Entry Dword 0 of Foo End */" in text
    assert "#define XNVME_SPEC_FOO_DING_FOO_BEGIN_CDW12_FAGN 0xff" in text
    assert "#define XNVME_SPEC_FOO_DING_FOO_RESUME_CDW12_FAGN 0xff" in text


def commands_only(tmp_path, commands, structures=(), values=(), features=None):
    """A data model of the command set Foo with 'commands' alone, or with the
    'structures', the tables of 'values' and the 'features' as well"""

    document = spec_model.SpecDocument(
        command_sets={
            "foo": spec_model.CommandSet(
                name="Foo",
                key="foo",
                title="Foo Command Set",
                document="foo",
                kind="io",
                commands=commands,
            )
        },
        structures=list(structures),
        values=list(values),
        features=dict(features or {}),
        meta=spec_model.DocumentMeta(version="9.8.7"),
    )
    path = tmp_path / "commands.spec.document.json"
    path.write_text(document.model_dump_json(), encoding="utf-8")
    return path


def test_the_whole_sqe_of_an_operation_has_masks_by_the_dwords_of_its_fields(tmp_path):
    whole = spec_model.Dwords(
        lower=0,
        upper=15,
        fields=[
            bits("39:32", "Foo Type", "FTYP", unit="bits"),
            bits("319:192", "Foo Descriptor", "FDSC", unit="bits"),
            bits("335:320", "Foo Format", "FFMT", unit="bits"),
            bits("351:336", "Foo Queue", "FQID", unit="bits"),
            bits("447:384", "Foo Value", "FVAL", unit="bits"),
        ],
        source=source(figure_nr=40),
    )
    tune = spec_model.Command(
        name="Tune",
        opcode=0x7F,
        operations=[
            spec_model.Operation(
                name="Foo Hello",
                selectors={"FTYP": 1},
                sqe=[whole],
                source=source(figure_nr=40),
            )
        ],
        source=source(figure_nr=39),
    )
    text = text_of(
        generate(commands_only(tmp_path, {"tune": tune}), tmp_path / "out"),
        "libxnvme_spec_foo.h",
    )
    assert "#define XNVME_SPEC_FOO_TUNE_FOO_HELLO_CDW1_FTYP 0xff\n" in text
    assert "#define XNVME_SPEC_FOO_TUNE_FOO_HELLO_CDW10_FFMT 0xffff\n" in text
    assert "#define XNVME_SPEC_FOO_TUNE_FOO_HELLO_CDW10_FQID 0xffff0000\n" in text
    assert (
        "#define XNVME_SPEC_FOO_TUNE_FOO_HELLO_CDW12_13_FVAL 0xffffffffffffffff\n"
        in text
    )
    assert (
        "bits 63:0 of Command Dwords 12 and 13 of Foo Hello, as one 64-bit value */"
        in text
    )
    # Across four dwords, wider than a mask
    assert "FDSC" not in text


def test_the_values_most_fields_of_an_acronym_have_are_the_shared_enum(tmp_path):
    def kind(figure_nr, *values):
        return spec_model.Dwords(
            lower=10,
            upper=10,
            fields=[
                bits("01:00", "Foo Kind", "FKND", unit="bits").model_copy(
                    update={"values": list(values)}
                )
            ],
            source=source(figure_nr=figure_nr),
        )

    common = [value("0h", 0, "Foo Plain"), value("1h", 1, "Foo Fancy")]
    commands = {
        key: spec_model.Command(
            name=key.capitalize(),
            opcode=opcode,
            sqe=[dwords],
            source=source(figure_nr=figure_nr),
        )
        for key, opcode, figure_nr, dwords in (
            ("tick", 0x10, 50, kind(51, *common)),
            ("tock", 0x11, 52, kind(53, *common)),
            ("tack", 0x12, 54, kind(55, value("2h", 2, "Foo Other"))),
        )
    }
    text = text_of(
        generate(commands_only(tmp_path, commands), tmp_path / "out"),
        "libxnvme_spec_foo.h",
    )
    assert text.count("enum xnvme_spec_foo_fknd {") == 1
    assert "\tXNVME_SPEC_FOO_FKND_FOO_FANCY = 0x1," in text
    assert "enum xnvme_spec_foo_tack_cdw10_fknd {" in text
    assert "\tXNVME_SPEC_FOO_TACK_CDW10_FKND_FOO_OTHER = 0x2," in text
    assert "xnvme_spec_foo_tick_cdw10_fknd" not in text


def test_a_field_inside_a_field_of_a_dword_decides_no_shared_enum(tmp_path):
    """senfd 0.26.0 gives CDW0 of each Fabrics Command Type the fields of
    Base 2.4 Figure 96 inside the field CDW0; those fields do not count for
    the enum of the document, so they do not outvote PSDT of Figure 92, and
    each has an enum named by its mask"""

    def kind(*values):
        return bits("01:00", "Foo Kind", "FKND", unit="bits").model_copy(
            update={"values": list(values)}
        )

    common = kind(value("0h", 0, "Foo Plain"), value("1h", 1, "Foo Fancy"))
    other = kind(value("2h", 2, "Foo Other"))
    commands = {}
    for key, opcode, field in (
        ("tick", 0x10, common),
        ("tock", 0x11, common),
        ("tack", 0x12, bits("31:00", "Foo Word", "FWRD", "bits", [other])),
        ("tuck", 0x13, bits("31:00", "Foo Word", "FWRD", "bits", [other])),
        ("tyck", 0x14, bits("31:00", "Foo Word", "FWRD", "bits", [other])),
    ):
        commands[key] = spec_model.Command(
            name=key.capitalize(),
            opcode=opcode,
            sqe=[
                spec_model.Dwords(
                    lower=10,
                    upper=10,
                    fields=[field],
                    source=source(figure_nr=opcode),
                )
            ],
            source=source(figure_nr=opcode),
        )
    text = text_of(
        generate(commands_only(tmp_path, commands), tmp_path / "out"),
        "libxnvme_spec_foo.h",
    )
    assert text.count("enum xnvme_spec_foo_fknd {") == 1
    assert "\tXNVME_SPEC_FOO_FKND_FOO_FANCY = 0x1," in text
    assert "xnvme_spec_foo_tick_cdw10_fknd" not in text
    # No field of the dwords has the values of FKND inside FWRD, so each of
    # Tack, Tuck and Tyck has an enum of its own
    for key in ("tack", "tuck", "tyck"):
        assert (
            f"enum xnvme_spec_foo_{key}_cdw10_fwrd_fknd {{\n"
            f"\tXNVME_SPEC_FOO_{key.upper()}_CDW10_FWRD_FKND_FOO_OTHER = 0x2," in text
        )


def test_a_field_inside_a_field_has_no_enum_where_a_field_of_the_dwords_has(
    tmp_path,
):
    """PSDT within CDW0 of each Fabrics Command Type has the values of PSDT
    of the Fabrics command, a field of the dwords, whose enum the document has
    already; the fields within a field have no enum of their own"""

    def kind(*values):
        return bits("01:00", "Foo Kind", "FKND", unit="bits").model_copy(
            update={"values": list(values)}
        )

    common = kind(value("0h", 0, "Foo Plain"), value("1h", 1, "Foo Fancy"))
    other = kind(value("2h", 2, "Foo Other"))
    commands = {}
    for key, opcode, field in (
        ("tick", 0x10, common),
        ("tock", 0x11, common),
        ("teck", 0x12, other),
        ("tack", 0x13, bits("31:00", "Foo Word", "FWRD", "bits", [other])),
        ("tuck", 0x14, bits("31:00", "Foo Word", "FWRD", "bits", [other])),
    ):
        commands[key] = spec_model.Command(
            name=key.capitalize(),
            opcode=opcode,
            sqe=[
                spec_model.Dwords(
                    lower=10,
                    upper=10,
                    fields=[field],
                    source=source(figure_nr=opcode),
                )
            ],
            source=source(figure_nr=opcode),
        )
    text = text_of(
        generate(commands_only(tmp_path, commands), tmp_path / "out"),
        "libxnvme_spec_foo.h",
    )
    assert (
        "enum xnvme_spec_foo_teck_cdw10_fknd {\n"
        "\tXNVME_SPEC_FOO_TECK_CDW10_FKND_FOO_OTHER = 0x2," in text
    )
    assert "enum xnvme_spec_foo_tack_cdw10_fwrd_fknd" not in text
    assert "enum xnvme_spec_foo_tuck_cdw10_fwrd_fknd" not in text
    assert "#define XNVME_SPEC_FOO_TUCK_CDW10_FWRD_FKND 0x3" in text


def test_a_field_inside_a_field_writes_the_enum_of_the_document(tmp_path):
    """FKND inside FWRD of Tack has the values of the enum of the document,
    and is the first field to have them: the case of FKND in Tick does not
    count for that enum, so the enum of the document is written there"""

    def kind(*values, **kw):
        return bits("01:00", "Foo Kind", "FKND", unit="bits", **kw).model_copy(
            update={"values": list(values)}
        )

    other = value("2h", 2, "Foo Other")
    cased = kind(
        cases=[
            spec_model.Case(
                selector="FACT",
                when=[spec_model.Selected(name="Foo Start", value=0)],
                name="Foo Options",
                values=[other],
            )
        ]
    )
    commands = {}
    for key, opcode, field in (
        ("tick", 0x10, cased),
        ("tack", 0x12, bits("31:00", "Foo Word", "FWRD", "bits", [kind(other)])),
        ("tyck", 0x14, kind(other)),
    ):
        commands[key] = spec_model.Command(
            name=key.capitalize(),
            opcode=opcode,
            sqe=[
                spec_model.Dwords(
                    lower=10, upper=10, fields=[field], source=source(figure_nr=opcode)
                )
            ],
            source=source(figure_nr=opcode),
        )
    text = text_of(
        generate(commands_only(tmp_path, commands), tmp_path / "out"),
        "libxnvme_spec_foo.h",
    )
    assert text.count("enum xnvme_spec_foo_fknd {") == 1
    assert "\tXNVME_SPEC_FOO_FKND_FOO_OTHER = 0x2," in text
    assert "enum xnvme_spec_foo_tick_cdw10_fknd {" in text
    assert "enum xnvme_spec_foo_tack_cdw10_fwrd_fknd" not in text
    assert "enum xnvme_spec_foo_tyck_cdw10_fknd" not in text


def test_the_enum_of_a_field_inside_a_field_cites_the_layout_it_is_of(tmp_path):
    """QT within MOS of Controller Data Queue has its values in Base 2.4
    Figure 171, the layout that MOS links to, not in the figure of the
    dword"""

    kind = bits("01:00", "Foo Kind", "FKND", unit="bits").model_copy(
        update={"values": [value("2h", 2, "Foo Other")]}
    )
    word = bits(
        "31:00", "Foo Word", "FWRD", "bits", [kind], structure=source(figure_nr=99)
    )
    commands = {
        "ding": spec_model.Command(
            name="Ding",
            opcode=0x10,
            sqe=[
                spec_model.Dwords(
                    lower=10, upper=10, fields=[word], source=source(figure_nr=16)
                )
            ],
            source=source(figure_nr=16),
        )
    }
    text = text_of(
        generate(commands_only(tmp_path, commands), tmp_path / "out"),
        "libxnvme_spec_foo.h",
    )

    assert " * The values of Foo Kind (Foo 1.0, 1, Figure 99)\n" in text
    assert (
        "\tXNVME_SPEC_FOO_DING_CDW10_FWRD_FKND_FOO_OTHER = 0x2, ///< Foo Other"
        " (Foo 1.0, 1, Figure 99)" in text
    )


def test_an_array_has_the_layout_of_the_figure_its_entries_link_to(model, tmp_path):
    text = text_of(generate(model, tmp_path / "out"), "libxnvme_spec_foo.h")

    # An entry that is a scalar keeps its type, with masks named by the entry
    assert (
        "\tuint16_t ffmt[2]; ///< Foo Format, 2 of 2 bytes of"
        " XNVME_SPEC_FOO_FOO_FORMAT_*, bytes 3:0" in text
    )
    assert "#define XNVME_SPEC_FOO_FOO_FORMAT_FDS 0xff00" in text
    assert "#define XNVME_SPEC_FOO_FOO_FORMAT_FMS 0xff" in text
    # yace reports two masks of one integer whose bits overlap
    assert "_FDS_FDU" not in text
    assert (
        " * Without masks, as fields within a field that has a mask:\n"
        " * Foo Data Unit (FDU) (Foo 1.0, 1, Figure 15); bits 9:8 of 'Foo Format'\n"
        in struct_doc(text, "xnvme_spec_foo_foo_power_table")
    )
    # A wider entry is a struct written before the struct of the array
    entry = text.index(
        "struct __attribute__((packed)) xnvme_spec_foo_foo_power_descriptor {"
    )
    array = text.index("\tstruct xnvme_spec_foo_foo_power_descriptor fpd[2];")
    assert entry < array
    # A byte-aligned field of 1, 2, 4 or 8 bytes is a member
    assert "\tuint16_t fpwr; ///< Foo Power, bytes 1:0" in text
    assert "\tuint32_t flat; ///< Foo Latency, bytes 7:4" in text
    # Fields that share a byte are one member, with masks
    assert "\tuint8_t fsc_foff; ///< Foo Scale; Foo Off, byte 3, see" in text
    assert "#define XNVME_SPEC_FOO_FOO_POWER_DESCRIPTOR_FSC_FOFF_FOFF 0x2" in text
    # A field alone in its byte from bit 0 is the member, without a mask
    assert "\tuint8_t frate; ///< Foo Rate, bits 4:0 of byte 12" in text
    assert "_FRATE_FRATE" not in text
    # yace prints the value of a field under the name VAL, so VAL has no mask
    assert "\tuint8_t val; ///< Valid, bit 7 of byte 14" in text
    assert "_VAL_VAL" not in text
    assert "\tuint8_t rsvd15; ///< Reserved, byte 15\n" in text


def test_a_field_has_the_values_of_the_table_it_links_to(model, tmp_path):
    text = text_of(generate(model, tmp_path / "out"), "libxnvme_spec_foo.h")

    assert "enum xnvme_spec_foo_fsel {" in text
    assert (
        "\tXNVME_SPEC_FOO_FSEL_FOO_FAR = 0x1, ///< Foo Far (Foo 1.0, 1, Figure 23)"
        in text
    )


def test_an_array_that_senfd_links_is_an_array_of_its_entry(model, tmp_path):
    text = text_of(generate(model, tmp_path / "out"), "libxnvme_spec_foo.h")

    # The figure omits the entries between the first and the last, and
    # the entry is the structure that the field links to
    assert (
        "\tstruct xnvme_spec_foo_foo_cell fte[3]; ///< Foo Table Entry, 3 of 4 bytes,"
        " bytes 15:4" in text
    )
    assert text.index("struct __attribute__((packed)) xnvme_spec_foo_foo_cell {") < (
        text.index("struct __attribute__((packed)) xnvme_spec_foo_foo_table {")
    )


def test_an_array_whose_figure_gives_the_fields_of_a_record_is_of_their_struct(
    model, tmp_path
):
    text = text_of(generate(model, tmp_path / "out"), "libxnvme_spec_foo.h")

    record = text.index("struct __attribute__((packed)) xnvme_spec_foo_foo_range {")
    assert record < text.index(
        "struct __attribute__((packed)) xnvme_spec_foo_foo_range_list {"
    )
    assert "\tuint32_t fattr; ///< Foo Attributes, bytes 3:0" in text[record:]
    assert "\tuint64_t fsta; ///< Foo Start, bytes 15:8" in text[record:]
    assert (
        "\tstruct xnvme_spec_foo_foo_range foo_range[8]; ///< Foo Range, 8 of 16 bytes,"
        " bytes 127:0" in text
    )


def test_a_field_with_cases_has_their_masks_and_values(model, tmp_path):
    text = text_of(generate(model, tmp_path / "out"), "libxnvme_spec_foo.h")

    assert (
        "/** Foo Mode (Foo 1.0, 1, Figure 19); bits 15:8 of Command Dword 11;"
        " Foo Options when FACT is Foo Start */" in text
    )
    # A field of a case is a mask at its bits in the dword
    assert "#define XNVME_SPEC_FOO_DING_CDW11_FMODE_FLVL 0x3f00" in text
    # The values of the one case that names them are an enum of the field
    assert "\tXNVME_SPEC_FOO_DING_CDW11_FMODE_FOO_QUICK = 0x1," in text
    assert "\tXNVME_SPEC_FOO_DING_CDW11_FMODE_FLVL_FOO_TOP = 0x3f," in text


def test_a_case_that_is_a_field_of_all_the_bits_has_their_mask(model, tmp_path):
    text = text_of(generate(model, tmp_path / "out"), "libxnvme_spec_foo.h")

    assert "#define XNVME_SPEC_FOO_DING_CDW11_FOPT 0x10000" in text
    assert (
        "/** Foo Hold (FHOLD) (Foo 1.0, 1, Figure 19), when FACT is Foo Start or Foo"
        " Pause; bit 16 of Command Dword 11 */\n"
        "#define XNVME_SPEC_FOO_DING_CDW11_FOPT_FHOLD 0x10000" in text
    )


def test_a_big_endian_field_is_an_array_of_bytes(model, tmp_path):
    text = text_of(generate(model, tmp_path / "out"), "libxnvme_spec_foo.h")

    assert (
        "struct __attribute__((packed)) xnvme_spec_foo_foo_tag_format_when_fs_field_is_non_zero {"
        in text
    )
    assert "\tuint8_t grd[2]; ///< Guard, big-endian, bytes 1:0" in text
    assert "\tuint8_t at[2]; ///< Application Tag, big-endian, bytes 3:2" in text
    assert "\tuint8_t fflg; ///< Foo Flag, byte 4" in text
    # Without a byte order, a field is an integer of its size
    assert "\tuint16_t fct; ///< Foo Count, bytes 6:5" in text


def test_the_cases_of_a_field_of_bytes_are_a_union(model, tmp_path):
    text = text_of(generate(model, tmp_path / "out"), "libxnvme_spec_foo.h")

    union = text.index(
        "union __attribute__((packed)) xnvme_spec_foo_foo_command_fdptr {"
    )
    record = text.index(
        "struct __attribute__((packed)) xnvme_spec_foo_foo_command_fdptr_fprp1_fprp2 {"
    )
    struct = text.index("struct __attribute__((packed)) xnvme_spec_foo_foo_command {")
    assert record < union < struct
    # A case of other fields is a struct of them at their bytes in the field
    assert (
        " * Foo PRP Entry 1 and Foo PRP Entry 2 of Foo Data Pointer (Foo 1.0, 1,"
        " Figure 2), when FPSDT is 00b" in text
    )
    assert "\tuint64_t fprp1; ///< Foo PRP Entry 1, bytes 7:0" in text[record:]
    assert "\tuint64_t fprp2; ///< Foo PRP Entry 2, bytes 15:8" in text[record:]
    # Each member of the union names its case
    assert (
        "\tstruct xnvme_spec_foo_foo_command_fdptr_fprp1_fprp2 fprp1_fprp2; ///< Foo"
        " PRP Entry 1 and Foo PRP Entry 2, when FPSDT is 00b" in text[union:]
    )
    # A case of one field of all the bytes is a member of the union, of the
    # struct of the record the field links to
    assert (
        "\tstruct xnvme_spec_foo_foo_sgl_descriptor fsgl1; ///< Foo SGL Entry 1,"
        " when FPSDT is 01b or 10b" in text[union:]
    )
    assert (
        text.index("struct __attribute__((packed)) xnvme_spec_foo_foo_sgl_descriptor {")
        < union
    )
    assert (
        "\tunion xnvme_spec_foo_foo_command_fdptr fdptr; ///< Foo Data Pointer,"
        " bytes 23:8" in text[struct:]
    )


def test_a_field_of_bytes_is_of_the_struct_of_the_record_it_links_to(model, tmp_path):
    text = text_of(generate(model, tmp_path / "out"), "libxnvme_spec_foo.h")

    start = text.index(
        "struct __attribute__((packed)) xnvme_spec_foo_foo_fabrics_command {"
    )
    struct = text[start : text.index("};", start)]
    assert (
        "\tstruct xnvme_spec_foo_foo_sgl_descriptor fsgl1; ///< Foo SGL Descriptor 1,"
        " bytes 15:0" in struct
    )
    # The record comes before the struct that has a member of it
    assert (
        text.index("struct __attribute__((packed)) xnvme_spec_foo_foo_sgl_descriptor {")
        < start
    )
    # A record whose only field is the field itself is no member
    assert "\tuint8_t fhid[16]; ///< Foo Host Identifier, bytes 31:16" in struct
    # A record of another document is in another header
    assert "\tuint8_t bkey[16]; ///< Bar Key, bytes 47:32" in struct
    # A record of another size is no member
    assert "\tuint8_t fshrt[12]; ///< Foo Short, bytes 59:48" in struct


def test_a_field_whose_place_depends_on_a_condition_ends_the_struct(model, tmp_path):
    text = text_of(generate(model, tmp_path / "out"), "libxnvme_spec_foo.h")

    start = text.index("struct __attribute__((packed)) xnvme_spec_foo_foo_pdu {")
    struct = text[start : text.index("};", start)]
    assert "\tuint32_t fhdr; ///< Foo Header, bytes 3:0" in struct
    assert "fhdg" not in struct
    assert "fbdy" not in struct
    doc = text[text.rindex("/**", 0, start) : start]
    assert " * Foo Header Digest: at an offset the data gives" in doc
    assert " * Foo Body: at an offset the data gives" in doc


def test_arrays_of_one_entry_share_its_masks(model, tmp_path):
    text = text_of(generate(model, tmp_path / "out"), "libxnvme_spec_foo.h")

    assert (
        "\tuint16_t fsf[2]; ///< Foo Spare Format, 2 of 2 bytes of"
        " XNVME_SPEC_FOO_FOO_FORMAT_*, bytes 39:36" in text
    )
    assert text.count("#define XNVME_SPEC_FOO_FOO_FORMAT_FDS ") == 1
    assert text.count("enum xnvme_spec_foo_foo_format_") == 0
    assert text.count(" * Foo Data Unit (") == 1


def test_a_feature_is_the_masks_of_its_dwords_and_values(model, tmp_path):
    text = text_of(generate(model, tmp_path / "out"), "libxnvme_spec_foo.h")

    assert " * The fields of the feature Foo Config (Foo 1.0" in text
    assert "#define XNVME_SPEC_FOO_FEATURE_FOO_CONFIG_CDW11_FLV 0x30000" in text
    assert "#define XNVME_SPEC_FOO_FEATURE_FOO_CONFIG_CDW11_FLIM 0xffff" in text
    assert "bits 17:16 of Command Dword 11 */" in text
    assert "_FOO_CONFIG_CDW11_FOTH" not in text
    assert "#define XNVME_SPEC_FOO_FEATURE_FOO_CONFIG_CQE_DW0_FGNT 0xffff" in text
    assert "bits 15:0 of Completion Queue Entry Dword 0 */" in text
    assert "enum xnvme_spec_foo_flv {" in text
    assert "\tXNVME_SPEC_FOO_FLV_FOO_HIGH = 0x1," in text
    assert "#define XNVME_SPEC_FOO_FEATURE_FOO_CONFIG_FOO_KEEP_CDW12_FKE 0x1" in text
    assert "bit 0 of Command Dword 12 of Foo Keep */" in text


def test_a_wide_field_of_bits_is_an_array_with_masks_of_its_first_element(
    model, tmp_path
):
    text = text_of(generate(model, tmp_path / "out"), "libxnvme_spec_foo.h")

    assert (
        "\tuint64_t foo_supported[2]; ///< Foo Supported, bytes 15:0, see"
        " XNVME_SPEC_FOO_FOO_DIRECTIVES_FOO_SUPPORTED_*" in text
    )
    assert (
        "bit 9 of 'foo_supported[0]' */\n"
        "#define XNVME_SPEC_FOO_FOO_DIRECTIVES_FOO_SUPPORTED_FBARS 0x200" in text
    )
    assert "#define XNVME_SPEC_FOO_FOO_DIRECTIVES_FOO_SUPPORTED_FBAZS 0x1" in text
    assert "\tuint8_t foo_label[16]; ///< Foo Label, bytes 31:16" in text
    assert "_FOO_LABEL_FLB" not in text


def test_a_dword_described_under_conditions_has_the_masks_of_each(model, tmp_path):
    text = text_of(generate(model, tmp_path / "out"), "libxnvme_spec_foo.h")

    assert (
        " * Command Dword 13 is described if FCE is cleared to 0h, or if FCE is"
        " non-zero; the doc of a mask names the conditions of the descriptions that"
        " have its field, unless they cover every value" in text
    )
    # Both descriptions have FSPC, and FCE == 0 and FCE != 0 cover every value
    assert (
        "/** Foo Specific (Foo 1.0, 1, Figure 31); bits 31:16 of Command Dword 13"
        " */\n#define XNVME_SPEC_FOO_DING_CDW13_FSPC 0xffff0000" in text
    )
    assert text.count("#define XNVME_SPEC_FOO_DING_CDW13_FSPC ") == 1
    # A field of each condition has its mask, where the bits of the two overlap
    assert (
        "/** Foo Dataset Management (Foo 1.0, 1, Figure 31), when FCE is cleared to"
        " 0h; bits 7:0 of Command Dword 13 */\n#define XNVME_SPEC_FOO_DING_CDW13_FDSM"
        " 0xff" in text
    )
    assert (
        "/** Foo Extension Value (Foo 1.0, 1, Figure 32), when FCE is non-zero; bits"
        " 15:0 of Command Dword 13 */\n#define XNVME_SPEC_FOO_DING_CDW13_FCEV"
        " 0xffff" in text
    )


def test_a_value_is_named_as_value_names_of_senfd_returns(model, tmp_path):
    text = text_of(generate(model, tmp_path / "out"), "libxnvme_spec_foo.h")

    start = text.index("enum xnvme_spec_foo_fsubt {")
    enum = text[start : text.index("};", start)]
    assert enum.count("\tXNVME_SPEC_FOO_FSUBT_") == 3
    assert "\tXNVME_SPEC_FOO_FSUBT_FOO_ADDRESS = 0x0," in enum
    assert "\tXNVME_SPEC_FOO_FSUBT_FOO_OFFSET = 0x1," in enum
    assert "\tXNVME_SPEC_FOO_FSUBT_512B = 0x3," in enum


def test_the_fields_of_the_completion_of_a_command_have_masks(model, tmp_path):
    text = text_of(generate(model, tmp_path / "out"), "libxnvme_spec_foo.h")

    assert (
        "/** Foo Remaining (Foo 1.0, 1, Figure 33); bits 15:0 of Completion Queue"
        " Entry Dword 0 */\n#define XNVME_SPEC_FOO_DING_CQE_DW0_FREM 0xffff\n" in text
    )


def tick(*sqe, **more):
    """The command Tick of the dwords 'sqe'"""

    return spec_model.Command(
        name="Tick", opcode=0x10, sqe=list(sqe), source=source(figure_nr=60), **more
    )


def dword(lower, upper, *fields, condition=None, figure_nr=61):
    return spec_model.Dwords(
        lower=lower,
        upper=upper,
        fields=list(fields),
        condition=condition,
        source=source(figure_nr=figure_nr),
    )


def foo_text(tmp_path, commands, **more):
    return text_of(
        generate(commands_only(tmp_path, commands, **more), tmp_path / "out"),
        "libxnvme_spec_foo.h",
    )


def test_a_pair_of_dwords_is_one_64_bit_value_only_where_aligned_to_64_bits(
    tmp_path,
):
    text = foo_text(
        tmp_path,
        {
            "tick": tick(
                dword(9, 10, bits("47:00", "Foo Nine", "FNIN", unit="bits")),
                dword(
                    11,
                    12,
                    bits("47:00", "Foo Odd", "FODD", unit="bits"),
                    bits("63:48", "Foo Tag", "FTAG", unit="bits"),
                ),
                dword(14, 15, bits("47:00", "Foo Even", "FEVN", unit="bits")),
                cqe=[dword(0, 1, bits("47:00", "Foo Result", "FRES", unit="bits"))],
            )
        },
    )
    assert "#define XNVME_SPEC_FOO_TICK_CDW14_15_FEVN 0xffffffffffff\n" in text
    assert (
        "bits 47:0 of Completion Queue Entry Dwords 0 and 1, as one 64-bit value */\n"
        "#define XNVME_SPEC_FOO_TICK_CQE_DW0_1_FRES 0xffffffffffff\n" in text
    )
    assert not re.search("_(FNIN|FODD|FTAG) ", text)
    assert (
        " * FNIN of Command Dwords 9 and 10 has no mask: a pair of dwords that"
        " starts at an odd dword is no 64-bit value\n" in text
    )
    assert (
        " * FODD, FTAG of Command Dwords 11 and 12 have no masks: a pair of dwords"
        " that starts at an odd dword is no 64-bit value\n" in text
    )
    assert "FEVN of Command Dwords" not in text


def test_a_pair_of_dwords_described_under_two_conditions_is_named_once(tmp_path):
    def odd(condition, figure_nr):
        return dword(
            11,
            12,
            bits("47:00", "Foo Odd", "FODD", unit="bits"),
            bits("63:48", "Foo Tag", "FTAG", unit="bits"),
            condition=condition,
            figure_nr=figure_nr,
        )

    text = foo_text(
        tmp_path,
        {"tick": tick(odd("FSEL is cleared to 0h", 61), odd("FSEL is non-zero", 62))},
    )

    assert text.count(" * FODD, FTAG of Command Dwords 11 and 12 have no masks:") == 1


def test_a_pair_of_dwords_of_an_operation_a_feature_or_a_cqe_aligns_to_64_bits(
    tmp_path,
):
    whole = spec_model.Dwords(
        lower=0,
        upper=15,
        fields=[
            bits("39:32", "Foo Type", "FTYP", unit="bits"),
            bits("415:352", "Foo Odd Value", "FOVL", unit="bits"),
            bits("511:448", "Foo Even Value", "FEVL", unit="bits"),
        ],
        source=source(figure_nr=40),
    )
    tune = spec_model.Command(
        name="Tune",
        opcode=0x7F,
        operations=[
            spec_model.Operation(
                name="Foo Hello",
                selectors={"FTYP": 1},
                sqe=[whole],
                source=source(figure_nr=40),
            ),
            spec_model.Operation(
                name="Foo Bye",
                selectors={"FTYP": 2},
                sqe=[dword(11, 12, bits("47:00", "Foo Odd", "FODD", unit="bits"))],
                source=source(figure_nr=41),
            ),
        ],
        cqe=[dword(1, 2, bits("47:00", "Foo Completion", "FCQE", unit="bits"))],
        source=source(figure_nr=39),
    )
    feature = spec_model.Feature(
        identifier=0xC1,
        name="Foo Tune",
        source=source(figure_nr=42),
        sqe=[dword(11, 12, bits("47:00", "Foo Setting", "FSET", unit="bits"))],
        cqe=[dword(1, 2, bits("31:00", "Foo Got", "FGOT", unit="bits"))],
        operations=[
            spec_model.Operation(
                name="Foo Twist",
                selectors={"FSET": 1},
                sqe=[dword(13, 14, bits("47:00", "Foo Turn", "FTRN", unit="bits"))],
                source=source(figure_nr=43),
            )
        ],
    )
    model = commands_only(tmp_path, {"tune": tune}, features={"foo_tune": feature})
    text = text_of(generate(model, tmp_path / "out"), "libxnvme_spec_foo.h")

    assert (
        "#define XNVME_SPEC_FOO_TUNE_FOO_HELLO_CDW14_15_FEVL 0xffffffffffffffff\n"
        in text
    )
    assert not re.search("_(FOVL|FODD|FCQE|FSET|FGOT|FTRN) ", text)
    assert " * The fields of the feature Foo Tune (" in text
    odd = ": a pair of dwords that starts at an odd dword is no 64-bit value\n"
    for named in (
        "FOVL of Command Dwords 11 and 12 of Foo Hello has no mask",
        "FODD of Command Dwords 11 and 12 of Foo Bye has no mask",
        "FCQE of Completion Queue Entry Dwords 1 and 2 has no mask",
        "FSET of Command Dwords 11 and 12 has no mask",
        "FGOT of Completion Queue Entry Dwords 1 and 2 has no mask",
        "FTRN of Command Dwords 13 and 14 of Foo Twist has no mask",
    ):
        assert f" * {named}{odd}" in text


def test_a_field_beyond_its_dwords_or_a_later_alternative_has_no_mask(tmp_path):
    text = foo_text(
        tmp_path,
        {
            "tick": tick(
                dword(
                    10,
                    10,
                    bits("07:00", "Foo In", "FIN", unit="bits"),
                    bits(
                        "15:08", "Foo One", "FONE", unit="bits", union=0, alternative=0
                    ),
                    bits(
                        "23:16", "Foo Two", "FTWO", unit="bits", union=0, alternative=1
                    ),
                    bits("39:32", "Foo Out", "FOUT", unit="bits"),
                )
            )
        },
    )
    assert "#define XNVME_SPEC_FOO_TICK_CDW10_FIN 0xff\n" in text
    assert "#define XNVME_SPEC_FOO_TICK_CDW10_FONE 0xff00\n" in text
    assert "FTWO" not in text and "FOUT" not in text


def test_a_field_of_a_case_beyond_its_field_has_no_mask(tmp_path):
    zm = bits(
        "07:00",
        "Foo Zone Mode",
        "FZM",
        unit="bits",
        cases=[
            spec_model.Case(
                selector="FSEL",
                when=[spec_model.Selected(name="Foo Low", value=1)],
                name="Foo Low Mode",
                fields=[
                    bits("03:00", "Foo Low", "FLO", unit="bits"),
                    bits("11:08", "Foo High", "FHI", unit="bits"),
                ],
            )
        ],
    )
    text = foo_text(tmp_path, {"tick": tick(dword(10, 10, zm))})
    assert "#define XNVME_SPEC_FOO_TICK_CDW10_FZM_FLO 0xf\n" in text
    assert "FHI" not in text


def test_a_range_of_values_has_no_constant(tmp_path):
    span = spec_model.Entry(
        value="2h to 5h",
        value_column="value",
        name="Foo Span",
        typed={"value": spec_model.Typed(kind="number", lower=2, upper=5)},
    )
    kind = bits("03:00", "Foo Kind", "FKND", unit="bits").model_copy(
        update={"values": [value("0h", 0, "Foo Zero"), span]}
    )
    text = foo_text(tmp_path, {"tick": tick(dword(10, 10, kind))})
    assert "_FKND_FOO_ZERO = 0x0," in text
    assert "FOO_SPAN" not in text


def kinds(*values_of_each):
    """The commands Tick, Tock, ... each of a field FKND with the values of
    one item of 'values_of_each'"""

    commands = {}
    for nr, values in enumerate(values_of_each):
        key = ["tick", "tock", "tack"][nr]
        commands[key] = spec_model.Command(
            name=key.capitalize(),
            opcode=0x10 + nr,
            sqe=[
                dword(
                    10,
                    10,
                    bits("01:00", "Foo Kind", "FKND", unit="bits").model_copy(
                        update={"values": list(values)}
                    ),
                    figure_nr=70 + nr,
                )
            ],
            source=source(figure_nr=60 + nr),
        )
    return commands


def test_a_reserved_value_does_not_tell_the_values_of_two_fields_apart(tmp_path):
    common = [value("0h", 0, "Foo Plain"), value("1h", 1, "Foo Fancy")]
    reserved = value("2h", 2, "Reserved").model_copy(update={"role": "reserved"})
    text = foo_text(tmp_path, kinds(common, [*common, reserved]))
    assert text.count("enum xnvme_spec_foo_fknd {") == 1
    assert "_cdw10_fknd {" not in text


def test_a_tie_between_the_values_of_an_acronym_shares_no_enum(tmp_path):
    text = foo_text(
        tmp_path, kinds([value("0h", 0, "Foo Plain")], [value("1h", 1, "Foo Fancy")])
    )
    assert "enum xnvme_spec_foo_fknd {" not in text
    assert "enum xnvme_spec_foo_tick_cdw10_fknd {" in text
    assert "enum xnvme_spec_foo_tock_cdw10_fknd {" in text


def test_a_name_of_two_values_is_one_constant_for_each_value():
    lines = headers.enum_lines(
        "xnvme_spec_foo_ff",
        "The form factors",
        [
            ("XNVME_SPEC_FOO_FF_PCIE", 0x1, "PCIe"),
            ("XNVME_SPEC_FOO_FF_PCIE", 0x1, "PCIe again"),
            ("XNVME_SPEC_FOO_FF_PCIE", 0x2, "PCIe"),
            ("XNVME_SPEC_FOO_FF_PCIE", 0x2, "PCIe again"),
            ("XNVME_SPEC_FOO_FF_SATA", 0x3, "SATA"),
        ],
    )
    assert [line for line in lines if line.startswith("\t")] == [
        "\tXNVME_SPEC_FOO_FF_PCIE = 0x1, ///< PCIe",
        "\tXNVME_SPEC_FOO_FF_PCIE_2H = 0x2, ///< PCIe",
        "\tXNVME_SPEC_FOO_FF_SATA = 0x3, ///< SATA",
    ]


@pytest.mark.parametrize("small", [True, False])
def test_a_value_that_an_int_cannot_hold_is_a_macro_after_the_enum(small):
    constants = [("XNVME_SPEC_FOO_BIG_LARGE", 0x80000000, "Large")]
    if small:
        constants.insert(0, ("XNVME_SPEC_FOO_BIG_SMALL", 0x7FFFFFFF, "Small"))
    lines = headers.enum_lines("xnvme_spec_foo_big", "The big values", constants)
    macro = ["/** Large */", "#define XNVME_SPEC_FOO_BIG_LARGE 0x80000000", ""]
    if not small:
        assert lines == ["/*", " * The big values", " */", *macro]
        return
    assert lines == [
        "/**",
        " * The big values",
        " *",
        " * The values that an int cannot hold follow the enum as macros",
        " */",
        "enum xnvme_spec_foo_big {",
        "\tXNVME_SPEC_FOO_BIG_SMALL = 0x7fffffff, ///< Small",
        "};",
        "",
        *macro,
    ]


def test_operations_of_two_selectors_have_an_enum_only_when_both_have_masks(
    tmp_path,
):
    command = tick(
        dword(10, 10, bits("01:00", "Foo Kind", "FKND", unit="bits")),
        operations=[
            spec_model.Operation(
                name="Foo Go",
                selectors={"FKND": 1, "FNONE": 2},
                source=source(figure_nr=62),
            )
        ],
    )
    text = foo_text(tmp_path, {"tick": command})
    assert "#define XNVME_SPEC_FOO_TICK_CDW10_FKND 0x3\n" in text
    assert "FNONE" not in text and "FOO_GO" not in text


def test_a_status_table_of_a_command_or_within_another_is_no_enum_of_its_type(
    tmp_path,
):
    both = spec_model.Values(
        kind="status_values",
        name="Status Code",
        qualifier="Generic Command",
        status_code_type=0,
        columns=["value", "description"],
        entries=[value("C0h", 0xC0, "Foo Invalid"), value("C1h", 0xC1, "Foo Busy")],
        source=source(figure_nr=63),
    )
    text = foo_text(
        tmp_path,
        {"tick": tick(status_values=[source(figure_nr=64)])},
        values=[
            both,
            # Cited by the command
            status("Generic Command", 0, None, 0xC2, "Foo Own", 64),
            # Its codes are within those of the first table
            status("Generic Command", 0, None, 0xC1, "Foo Within", 65),
        ],
    )
    assert "\tXNVME_SPEC_FOO_GENERIC_COMMAND_STATUS_FOO_BUSY = 0xc1," in text
    assert "FOO_OWN" not in text and "FOO_WITHIN" not in text


def test_two_status_tables_of_one_scope_with_equal_codes_are_one_enum(tmp_path):
    text = foo_text(
        tmp_path,
        {"tick": tick()},
        values=[
            status("Command Specific", 1, None, 0x7F, "Foo Failed", 66),
            status("Command Specific", 1, None, 0x7F, "Foo Failed", 67),
        ],
    )

    assert (
        text.count("\tXNVME_SPEC_FOO_COMMAND_SPECIFIC_STATUS_FOO_FAILED = 0x7f,") == 1
    )


def test_the_status_code_types_are_an_enum_of_that_name(tmp_path):
    text = foo_text(
        tmp_path,
        {"tick": tick()},
        values=[
            status("Status Code Type", None, None, 0x1, "Foo Specific", 66),
            status("Foo Zoning Operations", None, None, 0x0, "Foo Successful", 67),
        ],
    )

    assert "enum xnvme_spec_foo_status_code_type {" in text
    assert "\tXNVME_SPEC_FOO_STATUS_CODE_TYPE_FOO_SPECIFIC = 0x1," in text
    assert "enum xnvme_spec_foo_foo_zoning_operations_status {" in text


def test_a_dword_with_a_description_without_a_condition_has_its_masks_alone(tmp_path):
    text = foo_text(
        tmp_path,
        {
            "tick": tick(
                dword(13, 13, bits("07:00", "Foo Plain", "FPLN", unit="bits")),
                dword(
                    13,
                    13,
                    bits("15:08", "Foo Else", "FELS", unit="bits"),
                    condition="FCE is non-zero",
                    figure_nr=62,
                ),
            )
        },
    )
    assert "#define XNVME_SPEC_FOO_TICK_CDW13_FPLN 0xff\n" in text
    assert "FELS" not in text


def test_an_entry_whose_layout_is_one_field_of_all_its_bytes_is_no_struct(tmp_path):
    sensors = structure(
        "Foo Sensors",
        [
            bits(
                f"{2 * i + 1:02}:{2 * i:02}",
                f"Foo Sensor {i}",
                f"FSNS{i}",
                structure=source(figure_nr=71),
                array=spec_model.Array(
                    name="Foo Sensor",
                    acronym="FSNS",
                    index=i,
                    first=0,
                    count=2,
                    stride=2,
                    lower=0,
                ),
            )
            for i in range(2)
        ],
        figure_nr=70,
    )
    sensor = structure(
        "Foo Sensor", [bits("01:00", "Foo Temperature", "FTMP")], figure_nr=71
    )
    text = foo_text(tmp_path, {}, structures=[sensors, sensor])
    assert "\tuint16_t fsns[2]; ///< Foo Sensor" in text
    assert "struct xnvme_spec_foo_foo_sensor fsns" not in text


def test_a_field_whose_bits_an_earlier_field_of_its_condition_covers_has_no_mask(
    tmp_path,
):
    text = foo_text(
        tmp_path,
        {
            "tick": tick(
                dword(
                    13,
                    13,
                    bits("07:00", "Foo Zeroed", "FZRO", unit="bits"),
                    condition="FCE is cleared to 0h",
                ),
                dword(
                    13,
                    13,
                    bits("15:00", "Foo Wide", "FWID", unit="bits"),
                    bits("07:00", "Foo Narrow", "FNRW", unit="bits"),
                    condition="FCE is non-zero",
                    figure_nr=62,
                ),
            )
        },
    )
    assert "#define XNVME_SPEC_FOO_TICK_CDW13_FZRO 0xff\n" in text
    assert "#define XNVME_SPEC_FOO_TICK_CDW13_FWID 0xffff\n" in text
    assert "FNRW" not in text


def struct_doc(text, name):
    """The doc of the struct 'name' in 'text'"""

    start = text.index(f"struct __attribute__((packed)) {name} {{")
    return text[text.rindex("/**", 0, start) : start]


def struct_of(text, name):
    """The members of the struct 'name' in 'text', without their docs"""

    body = text[text.index(f" {name} {{\n") : text.index("\n};", text.index(name))]
    return [line.split("///<")[0].strip() for line in body.splitlines()[1:]]


def test_the_rows_of_a_figure_are_members_from_the_lowest_offset_and_one_alternative(
    tmp_path,
):
    """A figure can give its rows from the highest offset down; of the fields
    that share bits, the first alternative is the member"""

    model = structures_only(
        tmp_path,
        [
            structure(
                "Foo Address",
                [
                    bits("15:08", "Foo High", "FHI"),
                    bits("07:04", "Foo Six", "FSIX", union=1, alternative=1),
                    bits("07:04", "Foo Four", "FFOR", union=1, alternative=0),
                    bits("03:00", "Foo Low", "FLO"),
                ],
            )
        ],
    )
    text = text_of(generate(model, tmp_path / "out"), "libxnvme_spec_foo.h")
    assert struct_of(text, "xnvme_spec_foo_foo_address") == [
        "uint32_t flo;",
        "uint32_t ffor;",
        "uint64_t fhi;",
    ]


def test_an_acronym_that_is_the_name_of_its_field_has_no_shared_enum(tmp_path):
    common = [value("0h", 0, "Foo Plain"), value("1h", 1, "Foo Fancy")]
    commands = {
        key: spec_model.Command(
            name=key.capitalize(),
            opcode=0x10 + nr,
            sqe=[
                dword(
                    10,
                    10,
                    bits("01:00", "Status", "STATUS", unit="bits").model_copy(
                        update={"values": common}
                    ),
                    figure_nr=70 + nr,
                )
            ],
            source=source(figure_nr=60 + nr),
        )
        for nr, key in enumerate(["tick", "tock"])
    }
    text = foo_text(tmp_path, commands)
    assert "enum xnvme_spec_foo_status {" not in text
    assert "enum xnvme_spec_foo_tick_cdw10_status {" in text
    assert "enum xnvme_spec_foo_tock_cdw10_status {" in text


def test_a_reserved_status_code_has_no_constant(tmp_path):
    table = status("Generic Command", 0, None, 0xC0, "Foo Invalid", 4)
    reserved = value("C1h", 0xC1, "Reserved").model_copy(update={"role": "reserved"})
    table = table.model_copy(update={"entries": [*table.entries, reserved]})
    text = foo_text(tmp_path, {"tick": tick()}, values=[table])
    assert "\tXNVME_SPEC_FOO_GENERIC_COMMAND_STATUS_FOO_INVALID = 0xc0," in text
    assert "= 0xc1," not in text


def test_a_layout_of_only_the_entries_that_a_count_gives_is_no_struct(tmp_path):
    items = [
        bits(
            range_,
            f"Foo Item {index}",
            formula=formula,
            array=spec_model.Array(
                name="Foo Item",
                index=index if isinstance(index, int) else None,
                first=1,
                stride=8,
                lower=0,
                counted_by="NFI",
            ),
        )
        for range_, index, formula in (
            ("07:00", 1, None),
            ("15:08", 2, None),
            ("(NFI*8)-1:(NFI*8)-8", "NFI", "8*NFI - 1:8*NFI - 8"),
        )
    ]
    model = structures_only(
        tmp_path,
        [
            structure("Foo Items", items),
            structure("Foo Stamp", [bits("07:00", "Foo When", "FWHN")], figure_nr=3),
        ],
    )
    text = text_of(generate(model, tmp_path / "out"), "libxnvme_spec_foo.h")
    assert "xnvme_spec_foo_foo_stamp {" in text
    assert "foo_items" not in text


def test_a_field_of_bytes_of_one_field_of_all_its_bits_is_an_array_of_bytes(tmp_path):
    wide = bits(
        "15:00",
        "Foo Wide",
        fields=[bits("127:00", "Foo Wide Value", "FWV", unit="bits")],
    )
    model = structures_only(tmp_path, [structure("Foo Wide", [wide])])
    text = text_of(generate(model, tmp_path / "out"), "libxnvme_spec_foo.h")
    assert struct_of(text, "xnvme_spec_foo_foo_wide") == ["uint8_t foo_wide[16];"]


def yace_status(model, tmp_path):
    """The status that yace writes for the headers of 'model', or None where
    yace or a C compiler is missing"""

    if importlib.util.find_spec("yace") is None:
        return None
    if shutil.which("cc") is None and shutil.which("gcc") is None:
        return None
    proc = subprocess.run(
        [
            sys.executable,
            "-m",
            "yace",
            "--generate",
            f"{SCRIPT}:SpecHeaders",
            "--generator-option",
            f"xnvme_spec.spec={model}",
            "--output",
            str(tmp_path / "yace"),
        ],
        capture_output=True,
        text=True,
    )
    assert proc.returncode == 0, proc.stderr
    (status,) = (tmp_path / "yace").glob("*.status")
    return status.read_text()


def test_a_field_of_a_formula_before_a_field_at_an_offset_is_in_the_bytes_before_it(
    tmp_path,
):
    """The Stuff Bytes of the RPMB Data Frame end where the next row begins,
    and the Data after the last row at an offset follow the struct"""

    model = structures_only(
        tmp_path,
        [
            structure(
                "Foo Frame",
                [
                    bits("05-N:00", "Foo Stuff", "FSTF", formula="-N + 5:0"),
                    bits("05:05-(N-1)", "Foo Key", "FKEY", formula="5:-N + 6"),
                    bits("06", "Foo Kind", "FKND"),
                    bits("07", "Foo Result", "FRES"),
                    bits("(M-1)+8:8", "Foo Data", "FDAT", formula="M + 7:8"),
                ],
            )
        ],
    )
    text = text_of(generate(model, tmp_path / "out"), "libxnvme_spec_foo.h")

    assert struct_of(text, "xnvme_spec_foo_foo_frame") == [
        "uint8_t fstf_fkey[6];",
        "uint8_t fknd;",
        "uint8_t fres;",
    ]
    assert "\tuint8_t fstf_fkey[6]; ///< Foo Stuff and Foo Key, bytes 5:0" in text
    assert " * Followed, from byte 8, by Foo Data, at bytes (M-1)+8:8" in text
    assert " * Foo Stuff: bytes 05-N:00, in 'fstf_fkey'" in text
    assert " * Foo Key: bytes 05:05-(N-1), in 'fstf_fkey'" in text
    assert "Reserved" not in text


def test_the_bytes_that_no_row_covers_cite_the_structure_it_extends(tmp_path):
    extended = structure(
        "Foo Header", [bits("15:00", "Foo Common", "FCOM")], figure_nr=3
    )
    extends = structure(
        "Foo Detail",
        [
            bits("17:16", "Foo Detail Code", "FDC"),
            bits("21:20", "Foo Detail Tail", "FDT", elided=True),
        ],
        figure_nr=4,
    ).model_copy(update={"extends": source(figure_nr=3)})
    field_of = structure(
        "Foo Part", [bits("17:16", "Foo Part Code", "FPC")], figure_nr=5
    ).model_copy(update={"extends": source(figure_nr=3), "extends_field": "FCOM"})
    alone = structure(
        "Foo Alone", [bits("17:16", "Foo Alone Code", "FAC")], figure_nr=6
    )
    model = structures_only(tmp_path, [extended, extends, field_of, alone])
    text = text_of(generate(model, tmp_path / "out"), "libxnvme_spec_foo.h")

    cited = "Foo Header (Foo 1.0, 1, Figure 3)"
    assert f" * Extends {cited}\n" in text
    assert f" * Extends FCOM of {cited}\n" in text
    assert f"\tuint8_t rsvd0[16]; ///< bytes 15:0, see {cited}\n" in text
    assert (
        f"\tuint8_t rsvd18[2]; ///< bytes 19:18, rows that the figure elides, see"
        f" {cited}\n" in text
    )
    alone_struct = text[
        text.index("Foo Alone (") : text.index("xnvme_spec_foo_foo_alone {")
    ]
    assert "Extends" not in alone_struct
    assert "\tuint8_t rsvd0[16]; ///< bytes 15:0, not in the figure\n" in text
    assert "Reserved" not in text


def test_the_bytes_before_a_row_after_elided_rows_are_the_elided_entries(tmp_path):
    def slot(index, range_, elided=False):
        return bits(
            range_,
            f"Foo Slot {index}",
            f"FSL{index}",
            elided=elided,
            array=spec_model.Array(name="Foo Slot", index=index, first=1, stride=2),
        )

    model = structures_only(
        tmp_path,
        [structure("Foo Slots", [slot(1, "01:00"), slot(9, "17:16", elided=True)])],
    )
    text = text_of(generate(model, tmp_path / "out"), "libxnvme_spec_foo.h")

    assert (
        "\tuint8_t rsvd2[14]; ///< bytes 15:2, Foo Slot entries that the figure"
        " elides\n" in text
    )


def test_the_fields_after_the_entries_name_no_member_without_flexible(tmp_path):
    def entry(index, range_, formula):
        return bits(
            range_,
            f"Foo Entry {index}",
            formula=formula,
            array=spec_model.Array(name="Foo Entry", index=index, first=0, stride=4),
        )

    rows = [
        bits("03:00", "Foo Count", "FCNT"),
        entry(0, "(FCNT*4)+3:(FCNT*4)", "4*FCNT + 3:4*FCNT"),
        entry(1, "(FCNT*4)+7:(FCNT*4)+4", "4*FCNT + 7:4*FCNT + 4"),
        bits(
            "(FCNT*4)+11:(FCNT*4)+8",
            "Foo Check",
            "FCHK",
            formula="4*FCNT + 11:4*FCNT + 8",
        ),
    ]
    event = [
        bits("03:00", "Foo Length", "FLEN"),
        bits("(FLEN+3):04", "Foo Info", formula="FLEN + 3:4"),
        bits("(FLEN+4)", "Foo Size", "FSIZ", formula="FLEN + 4"),
        bits(
            "(FSIZ+FLEN+5):(FLEN+5)",
            "Foo Extra",
            formula="FSIZ + FLEN + 5:FLEN + 5",
        ),
    ]
    model = structures_only(
        tmp_path,
        [structure("Foo Tail", rows), structure("Foo Event", event, figure_nr=3)],
    )
    text = text_of(generate(model, tmp_path / "out"), "libxnvme_spec_foo.h")

    # The doc gives the range of the data model, as it has no bounds, and the
    # range of the first entry, which defines FCNT for the later ranges
    given = " * Foo {}: bytes {}, {} the Foo Entries\n"
    assert (
        " * Followed, from byte 4, by Foo Entries, the first at bytes"
        " (FCNT*4)+3:(FCNT*4), as many as the data says, each of 4 bytes\n" in text
    )
    assert " * Followed, from byte 4, by Foo Info, at bytes (FLEN+3):04\n" in text
    assert given.format("Entry 1", "(FCNT*4)+7:(FCNT*4)+4", "one of") in text
    assert given.format("Check (FCHK)", "(FCNT*4)+11:(FCNT*4)+8", "after") in text
    # A later range names FSIZ, so the doc gives the acronym of Foo Size
    assert " * Foo Size (FSIZ): byte (FLEN+4), after the Foo Info\n" in text
    assert " * Foo Extra: bytes (FSIZ+FLEN+5):(FLEN+5), after the Foo Info\n" in text
    assert "at an offset the data gives" not in text
    assert "'foo_entry'" not in text
    text = text_of(
        generate(model, tmp_path / "flexible", flexible="1"), "libxnvme_spec_foo.h"
    )
    assert " * Foo Check (FCHK): bytes (FCNT*4)+11:(FCNT*4)+8, in 'foo_entry'" in text


def test_a_field_of_open_size_after_the_members_is_as_long_as_the_data_says(
    tmp_path,
):
    """A field whose range is a phrase, as "Variable Sized", and that is no
    array, has no range to give: its size is in the data"""

    sized = spec_model.Bits(
        role="field",
        kind="open",
        name="Foo Data",
        unit="bytes",
        range="Variable Sized",
        description="",
    )
    rows = [bits("03:00", "Foo Length", "FLEN"), sized]
    model = structures_only(tmp_path, [structure("Foo Blob", rows)])
    text = text_of(generate(model, tmp_path / "out"), "libxnvme_spec_foo.h")

    assert (
        " * Followed, from byte 4, by Foo Data, of as many bytes as the data says\n"
        in struct_doc(text, "xnvme_spec_foo_foo_blob")
    )


def test_a_field_within_a_field_of_a_dword_has_a_mask_at_its_bits(tmp_path):
    check = bits(
        "02:00",
        "Foo Check",
        "FCHK",
        unit="bits",
        fields=[bits("01", "Foo Guard", "FGRD", unit="bits")],
    )
    info = bits(
        "29:26",
        "Foo Info",
        "FINF",
        unit="bits",
        fields=[
            bits("03", "Foo Action", "FACT", unit="bits"),
            check,
            # A field of all the bits of its field is that field again
            bits("03:00", "Foo Info", "FINF", unit="bits"),
        ],
    )
    queue = bits(
        "07:00",
        "Foo Queue",
        "FQUE",
        unit="bits",
        structure=source(figure_nr=62),
        fields=[bits("01:00", "Foo Queue Type", "FQTY", unit="bits")],
    )
    result = bits(
        "03:00",
        "Foo Result",
        "FRES",
        unit="bits",
        fields=[bits("00", "Foo Done", "FDON", unit="bits")],
    )
    text = foo_text(
        tmp_path,
        {
            "tick": tick(
                dword(12, 12, info), dword(13, 13, queue), cqe=[dword(0, 0, result)]
            )
        },
    )

    assert "#define XNVME_SPEC_FOO_TICK_CDW12_FINF 0x3c000000" in text
    assert (
        "/** Foo Action (Foo 1.0, 1, Figure 61); bit 29 of Command Dword 12 */\n"
        "#define XNVME_SPEC_FOO_TICK_CDW12_FINF_FACT 0x20000000" in text
    )
    assert "#define XNVME_SPEC_FOO_TICK_CDW12_FINF_FCHK 0x1c000000" in text
    assert (
        "/** Foo Guard (Foo 1.0, 1, Figure 61); bit 27 of Command Dword 12 */\n"
        "#define XNVME_SPEC_FOO_TICK_CDW12_FINF_FCHK_FGRD 0x8000000" in text
    )
    assert "_FINF_FINF" not in text
    assert (
        "/** Foo Queue Type (Foo 1.0, 1, Figure 62); bits 1:0 of Command Dword 13 */\n"
        "#define XNVME_SPEC_FOO_TICK_CDW13_FQUE_FQTY 0x3\n" in text
    )
    assert "#define XNVME_SPEC_FOO_TICK_CQE_DW0_FRES_FDON 0x1\n" in text


def test_a_field_of_bits_without_a_mask_is_named_by_the_doc_of_the_struct(tmp_path):
    """yace reports two masks of one member whose bits overlap, and an array
    of bytes has no masks"""

    opcode = bits(
        "07:00",
        "Foo Opcode",
        "FOPC",
        unit="bits",
        fields=[bits("01:00", "Foo Direction", "FDIR", unit="bits")],
    )
    location = bits(
        "04:02",
        "Foo Location",
        "FLOC",
        fields=[
            bits("23:08", "Foo Byte", "FBYTE", unit="bits"),
            bits("02:00", "Foo Bit", "FBIT", unit="bits"),
        ],
    )
    model = structures_only(
        tmp_path,
        [
            structure(
                "Foo Error",
                [bits("01:00", "Foo Word", "FWRD", fields=[opcode]), location],
            ),
            register(
                "FCC - Foo Class Code",
                0,
                [
                    bits("23:16", "Foo Base Class", "FBCC", unit="bits"),
                    bits("15:00", "Foo Sub Class", "FSCC", unit="bits"),
                ],
                figure_nr=9,
            ),
            register(
                "FST - Foo Status",
                4,
                [
                    bits(
                        "03:00",
                        "Foo State",
                        "FSTA",
                        unit="bits",
                        fields=[bits("01", "Foo Ready", "FRDY", unit="bits")],
                    ),
                    bits("07:04", "Foo Spare", "FSPR", unit="bits"),
                ],
                figure_nr=10,
            ),
        ],
    )
    text = text_of(generate(model, tmp_path / "out"), "libxnvme_spec_foo.h")

    assert "#define XNVME_SPEC_FOO_FOO_ERROR_FWRD_FOPC 0xff" in text
    assert "_FOPC_FDIR" not in text
    assert (
        " * Without masks, as fields within a field that has a mask:\n"
        " * Foo Direction (FDIR) (Foo 1.0, 1, Figure 2); bits 1:0 of 'fwrd'\n" in text
    )
    assert "\tuint8_t floc[3]; ///< Foo Location, bytes 4:2\n" in text
    assert (
        " * Without masks, as fields of an array of bytes:\n"
        " * Foo Byte (FBYTE) (Foo 1.0, 1, Figure 2); bits 23:8 of 'floc'\n"
        " * Foo Bit (FBIT) (Foo 1.0, 1, Figure 2); bits 2:0 of 'floc'\n" in text
    )
    register_ = "\tuint8_t fcc[3]; ///< FCC - Foo Class Code (Foo 1.0, 1, Figure 9)"
    assert f"{register_}, at 0h\n" in text
    assert (
        " * Foo Base Class (FBCC) (Foo 1.0, 1, Figure 9); bits 23:16 of 'fcc'\n" in text
    )
    assert "#define XNVME_SPEC_FOO_REGISTERS_FST_FSTA 0xf\n" in text
    assert (
        " * Without masks, as fields within a field that has a mask:\n"
        " * Foo Ready (FRDY) (Foo 1.0, 1, Figure 10); bit 1 of 'fst'\n" in text
    )
    status = yace_status(model, tmp_path)
    assert status is None or ", 0 diagnostics," in status


def conditioned(figure_nr, condition, comparison=None, value=None):
    """Command Dword 13 with FSPC under 'condition', which senfd compares as
    FCE 'comparison' 'value', or not at all without 'comparison'"""

    return spec_model.Dwords(
        lower=13,
        upper=13,
        condition=condition,
        selected_when=(
            None
            if comparison is None
            else spec_model.Comparison(field="FCE", comparison=comparison, value=value)
        ),
        fields=[bits("31:16", "Foo Specific", "FSPC", unit="bits")],
        source=source(figure_nr=figure_nr),
    )


@pytest.mark.parametrize(
    "first, second, when",
    [
        (("FCE is cleared to 0h", "==", 0), ("FCE is non-zero", "!=", 0), ""),
        (
            ("FCE is set to 1h", "==", 1),
            ("FCE is set to 2h", "==", 2),
            ", when FCE is set to 1h or FCE is set to 2h",
        ),
        (("FCE is 1h",), ("FCE is 2h",), ", when FCE is 1h or FCE is 2h"),
    ],
)
def test_the_doc_of_a_mask_names_the_conditions_unless_they_cover_every_value(
    tmp_path, first, second, when
):
    one, two = conditioned(61, *first), conditioned(62, *second)
    text = foo_text(tmp_path, {"tick": tick(one, two)})

    assert (
        f"/** Foo Specific (Foo 1.0, 1, Figure 61){when}; bits 31:16 of Command Dword"
        " 13 */\n#define XNVME_SPEC_FOO_TICK_CDW13_FSPC 0xffff0000\n" in text
    )


def test_a_field_whose_values_are_a_table_of_status_values_has_no_enum(tmp_path):
    """The codes of a table of status values are values of the Status Code of
    one Status Code Type, not values of the field that links to the table"""

    info = bits(
        "01:00",
        "Foo Status Info",
        "FSTI",
        fields=[
            bits("15:01", "Foo Status", "FSTS", unit="bits", values_in=source(FOO, 7)),
            bits("00", "Foo Mode", "FMOD", unit="bits", values_in=source(FOO, 8)),
        ],
    )
    modes = spec_model.Values(
        kind="other",
        name="Foo Mode",
        columns=["value", "description"],
        entries=[value("1h", 1, "Foo Fast")],
        source=source(figure_nr=8),
    )
    text = foo_text(
        tmp_path,
        {"tick": tick()},
        structures=[structure("Foo Response", [info], figure_nr=9)],
        values=[status("Command Specific", 1, None, 0x80, "Foo Wrong", 7), modes],
    )

    assert "#define XNVME_SPEC_FOO_FOO_RESPONSE_FSTI_FSTS 0xfffe\n" in text
    assert "_FSTS_FOO_WRONG" not in text
    assert "XNVME_SPEC_FOO_COMMAND_SPECIFIC_STATUS_FOO_WRONG = 0x80," in text
    assert "XNVME_SPEC_FOO_FMOD_FOO_FAST = 0x1," in text


def entries(name, lower, nbytes, first=0, **more):
    """Two entries of 'nbytes' bytes from the byte 'lower' of the array
    'name', which the figure numbers from 'first'"""

    return [
        bits(
            f"{lower + nbytes * (i + 1) - 1}:{lower + nbytes * i:02}",
            f"Foo {name} {first + i}",
            array=spec_model.Array(
                name=f"Foo {name}",
                index=first + i,
                first=first,
                count=2,
                stride=nbytes,
                lower=lower,
            ),
            **more,
        )
        for i in range(2)
    ]


def test_an_array_of_strings_or_of_big_endian_entries_is_of_bytes(tmp_path):
    rows = [
        *entries("Revision", 0, 8, encoding="ascii"),
        *entries("Key", 16, 8, byte_order="big-endian"),
        *entries("Size", 32, 8),
    ]
    model = structures_only(tmp_path, [structure("Foo Slots", rows)])
    text = text_of(generate(model, tmp_path / "out"), "libxnvme_spec_foo.h")

    assert (
        "\tuint8_t foo_revision[16]; ///< Foo Revision, 2 of 8 bytes, bytes 15:0\n"
        in text
    )
    assert (
        "\tuint8_t foo_key[16]; ///< Foo Key, 2 of 8 bytes, big-endian, bytes 31:16\n"
        in text
    )
    assert "\tuint64_t foo_size[2]; ///< Foo Size, 2 of 8 bytes, bytes 47:32\n" in text


def test_the_doc_of_an_array_says_when_the_figure_numbers_it_from_1(tmp_path):
    rows = [*entries("Dword", 0, 4, first=1), *entries("Word", 8, 2)]
    model = structures_only(tmp_path, [structure("Foo Entry", rows)])
    text = text_of(generate(model, tmp_path / "out"), "libxnvme_spec_foo.h")

    assert (
        "\tuint32_t foo_dword[2]; ///< Foo Dword 1 to 2, 2 of 4 bytes, bytes 7:0\n"
        in text
    )
    assert "\tuint16_t foo_word[2]; ///< Foo Word, 2 of 2 bytes, bytes 11:8\n" in text


def test_a_field_within_a_field_beyond_its_bits_is_named_by_the_comment(tmp_path):
    """The data model can give the bits of a field within a field as bits of
    the dword, as Key Value 1.4 Figure 31 gives NOCOMP as bit 10 of SO, which
    is bits 15:8 of Command Dword 11"""

    options = bits(
        "15:08",
        "Foo Options",
        "FOPT",
        unit="bits",
        fields=[
            bits("10", "Foo Plain", "FPLN", unit="bits"),
            bits("09:08", "Foo Fresh", "FFRS", unit="bits"),
            bits("03", "Foo Inside", "FINS", unit="bits"),
        ],
    )
    result = bits(
        "07:00",
        "Foo Result",
        "FRES",
        unit="bits",
        fields=[bits("09", "Foo Late", "FLAT", unit="bits")],
    )
    text = foo_text(
        tmp_path, {"tick": tick(dword(11, 11, options), cqe=[dword(0, 0, result)])}
    )

    assert "#define XNVME_SPEC_FOO_TICK_CDW11_FOPT 0xff00\n" in text
    assert "#define XNVME_SPEC_FOO_TICK_CDW11_FOPT_FINS 0x800\n" in text
    assert not re.search("_(FPLN|FFRS|FLAT) ", text)
    assert (
        " * FPLN (bit 10) and FFRS (bits 9:8) within FOPT of Command Dword 11 have"
        " no masks: the data model gives them bits beyond the 8 bits of FOPT\n" in text
    )
    assert (
        " * FLAT (bit 9) within FRES of Completion Queue Entry Dword 0 has no mask:"
        " the data model gives it bits beyond the 8 bits of FRES\n" in text
    )


def test_a_feature_without_fields_has_no_comment(tmp_path):
    quiet = spec_model.Feature(
        identifier=0xC2, name="Foo Quiet", source=source(figure_nr=44)
    )
    text = foo_text(tmp_path, {"tick": tick()}, features={"foo_quiet": quiet})

    assert "XNVME_SPEC_FOO_FEATURE_FOO_QUIET = 0xc2," in text
    assert "The fields of the feature Foo Quiet" not in text


def test_a_field_without_an_acronym_is_named_by_the_comment(tmp_path):
    key = bits("63:00", "Foo Key[63:00]", unit="bits")
    span = bits("63:00", "Foo Span", "FSPN", unit="bits")
    quiet = spec_model.Feature(
        identifier=0xC2,
        name="Foo Quiet",
        source=source(figure_nr=44),
        sqe=[
            dword(
                11,
                11,
                bits("21", "Foo Lost Notices", unit="bits"),
                bits("20", "Reserved", unit="bits"),
                bits("07:00", "Foo Level", "FLVL", unit="bits"),
                figure_nr=45,
            )
        ],
    )
    text = foo_text(
        tmp_path,
        {"tick": tick(dword(2, 3, key), dword(5, 6, span))},
        features={"foo_quiet": quiet},
    )

    assert "_CDW2_3_" not in text
    assert (
        " * FSPN of Command Dwords 5 and 6 has no mask: a pair of dwords that"
        " starts at an odd dword is no 64-bit value\n"
        " *\n"
        " * Without masks, as fields without an acronym:\n"
        " * Foo Key[63:00] (Foo 1.0, 1, Figure 61); bits 63:0 of Command Dwords 2"
        " and 3, as one 64-bit value\n"
        " */\n" in text
    )
    assert (
        " * The fields of the feature Foo Quiet (Foo 1.0, 1, Figure 44)\n"
        " *\n"
        " * Without masks, as fields without an acronym:\n"
        " * Foo Lost Notices (Foo 1.0, 1, Figure 45); bit 21 of Command Dword 11\n"
        " */\n" in text
    )
    assert "Reserved (" not in text
    assert "#define XNVME_SPEC_FOO_FEATURE_FOO_QUIET_CDW11_FLVL 0xff\n" in text


def test_a_field_of_a_struct_without_an_acronym_is_named_by_its_doc(tmp_path):
    opcode = bits(
        "07:00",
        "Foo Opcode",
        "FOPC",
        unit="bits",
        fields=[bits("07:04", "Foo Mode", unit="bits")],
    )
    word = bits(
        "01:00",
        "Foo Word",
        "FWRD",
        fields=[bits("15", "Foo Spare Bit", unit="bits"), opcode],
    )
    location = bits(
        "04:02",
        "Foo Location",
        "FLOC",
        fields=[
            bits("23", "Foo Flag", unit="bits"),
            bits("22:00", "Reserved", unit="bits"),
        ],
    )
    model = structures_only(tmp_path, [structure("Foo Error", [word, location])])
    text = text_of(generate(model, tmp_path / "out"), "libxnvme_spec_foo.h")

    assert (
        " * Without masks, as fields without an acronym:\n"
        " * Foo Spare Bit (Foo 1.0, 1, Figure 2); bit 15 of 'fwrd'\n"
        " * Foo Mode (Foo 1.0, 1, Figure 2); bits 7:4 of 'fwrd'\n"
        " * Foo Flag (Foo 1.0, 1, Figure 2); bit 23 of 'floc'\n"
        " */\n" in text
    )
    assert "Reserved (" not in text


def test_a_field_of_bytes_within_a_member_is_named_by_the_doc_of_the_struct(
    tmp_path,
):
    reference = bits(
        "13:08",
        "Foo Reference",
        "FREF",
        fields=[
            bits("03:00", "Foo Offset", "FOFS"),
            bits(
                "05:04",
                "Foo Length",
                "FLEN",
                fields=[bits("03:00", "Foo Scope", "FSCP", unit="bits")],
            ),
        ],
    )
    pair = bits(
        "FEXT+31:16",
        "Foo Pair 0",
        formula="FEXT + 31:16",
        array=spec_model.Array(
            name="Foo Pair", index=0, first=0, lower=16, counted_by="FCNT"
        ),
        fields=[
            bits("15:00", "Foo Descriptor 0", structure=source(figure_nr=3)),
            bits("FEXT+15:16", "Foo Extension 0", formula="FEXT + 15:16"),
        ],
    )
    descriptor = structure(
        "Foo Descriptor",
        [bits("07:00", "Foo Start", "FSTR"), bits("15:08", "Foo End", "FEND")],
        figure_nr=3,
    )
    rows = [bits("07:00", "Foo Count", "FCNT"), reference, bits("15:14", "Reserved")]
    model = structures_only(
        tmp_path, [structure("Foo Pairs", [*rows, pair]), descriptor]
    )
    text = text_of(generate(model, tmp_path / "out"), "libxnvme_spec_foo.h")

    assert (
        " * Followed, from byte 16, by Foo Pair entries, the first at bytes"
        " FEXT+31:16, counted by FCNT\n"
        " *\n"
        " * Not members:\n"
        " * Foo Offset (FOFS): bytes 3:0 of 'fref'\n"
        " * Foo Length (FLEN): bytes 5:4 of 'fref'\n"
        " * Foo Descriptor 0: bytes 15:0 of Foo Pair 0, a"
        " 'struct xnvme_spec_foo_foo_descriptor'\n"
        " * Foo Extension 0: bytes FEXT+15:16, in Foo Pair 0\n"
        " *\n"
        " * Without masks, as fields of an array of bytes:\n"
        " * Foo Scope (FSCP) (Foo 1.0, 1, Figure 2); bits 35:32 of 'fref'\n"
        in struct_doc(text, "xnvme_spec_foo_foo_pairs")
    )
    assert text.index("xnvme_spec_foo_foo_descriptor {") < text.index(
        "xnvme_spec_foo_foo_pairs {"
    )
    assert "\tuint8_t fref[6]; ///< Foo Reference, bytes 13:8\n" in text
    status = yace_status(model, tmp_path)
    assert status is None or ", 0 diagnostics," in status


def test_the_doc_names_the_number_and_the_size_of_the_entries_that_follow(tmp_path):
    def first(name, stride, formula, index=0):
        return bits(
            formula.replace(" ", ""),
            f"{name} 0" if index == 0 else f"{name} List",
            formula=formula,
            array=spec_model.Array(
                name=name, index=index, first=0, stride=stride, counted_by="FNUM"
            ),
        )

    model = structures_only(
        tmp_path,
        [
            structure(
                "Foo Ids",
                [
                    bits("03:00", "Foo Number", "FNUM"),
                    first("Foo Id", 4, "n:4"),
                ],
            ),
            structure(
                "Foo Tags",
                [
                    bits("03:00", "Foo Number", "FNUM"),
                    first("Foo Tag", 1, "m:4"),
                ],
                figure_nr=3,
            ),
            structure(
                "Foo Keys",
                [
                    bits("03:00", "Foo Number", "FNUM"),
                    first("Foo Key", 2, "2*FNUM + 3:4", index=None),
                ],
                figure_nr=5,
            ),
            structure(
                "Foo Blob",
                [
                    bits("03:00", "Foo Size", "FSZ"),
                    bits("FSZ+3:04", "Foo Payload", formula="FSZ + 3:4"),
                ],
                figure_nr=4,
            ),
        ],
    )
    text = text_of(generate(model, tmp_path / "out"), "libxnvme_spec_foo.h")

    assert (
        " * Followed, from byte 4, by Foo Id entries, the first at bytes"
        " n:4, counted by FNUM, each of 4 bytes\n" in text
    )
    assert (
        " * Followed, from byte 4, by Foo Tag entries, the first at bytes"
        " m:4, counted by FNUM, each of 1 byte\n" in text
    )
    # A field of the whole list, with no index, is not the first entry
    assert (
        " * Followed, from byte 4, by Foo Key entries, at bytes"
        " 2*FNUM+3:4, counted by FNUM, each of 2 bytes\n" in text
    )
    assert " * Followed, from byte 4, by Foo Payload, at bytes FSZ+3:04\n" in text


def test_the_fields_within_a_later_field_without_bounds_are_named(tmp_path):
    """The AE Occurrence of Management Interface 2.2 Figure 62 has the Vendor
    Specific Header, with the UUID Index in its bits, within the Vendor
    Specific Info that follows the Specific Info"""

    info = bits(
        "FSIL+7:08",
        "Foo Specific Info",
        "FSI",
        formula="FSIL + 7:8",
        fields=[
            bits(
                "01:00",
                "Foo Specific Kind",
                "FSK",
                fields=[bits("3:0", "Foo Kind Code", "FKC", unit="bits")],
            )
        ],
    )
    vendor = bits(
        "N:M",
        "Foo Vendor Info",
        "FVI",
        formula="N:M",
        fields=[
            bits(
                "M",
                "Foo Vendor Header",
                "FVH",
                formula="M:M",
                fields=[
                    bits("7", "Reserved", unit="bits"),
                    bits("6:0", "Foo UUID Index", "FUI", unit="bits"),
                ],
            ),
            bits(
                "07:04",
                "Foo Vendor Tag",
                "FVT",
                fields=[bits("3:0", "Foo Tag Kind", "FTK", unit="bits")],
            ),
        ],
    )
    sized = spec_model.Bits(
        role="field",
        kind="open",
        name="Foo Trailer",
        unit="bytes",
        range="Variable Sized",
        description="",
    )
    rows = [
        bits("03:00", "Foo Specific Info Length", "FSIL"),
        bits("07:04", "Reserved"),
        info,
        vendor,
        sized,
    ]
    model = structures_only(tmp_path, [structure("Foo Occurrence", rows)])
    text = text_of(generate(model, tmp_path / "out"), "libxnvme_spec_foo.h")

    assert (
        " * Not members:\n"
        " * Foo Specific Kind (FSK): bytes 1:0 of Foo Specific Info\n"
        " * Foo Kind Code (FKC): bits 3:0 of Foo Specific Kind\n"
        " * Foo Vendor Info (FVI): bytes N:M, after the Foo Specific Info\n"
        " * Foo Vendor Header (FVH): byte M, in Foo Vendor Info\n"
        " * Foo UUID Index (FUI): bits 6:0 of Foo Vendor Header\n"
        " * Foo Vendor Tag (FVT): bytes 7:4 of Foo Vendor Info\n"
        " * Foo Tag Kind (FTK): bits 3:0 of Foo Vendor Tag\n"
        " * Foo Trailer: at an offset the data gives, after the Foo Specific Info\n"
        in struct_doc(text, "xnvme_spec_foo_foo_occurrence")
    )
    assert "Reserved" not in struct_doc(text, "xnvme_spec_foo_foo_occurrence")


def test_a_field_without_a_mask_has_an_enum_of_its_values(tmp_path):
    """DTD within OPC of Command Dword 0 has no mask in a struct, nor has AESS
    within AESSI of the AE Occurrence Unique ID of Management Interface 2.2
    Figure 62, an array of bytes; the enum of each is named as its mask would
    be, or by the document where another field has the same values"""

    def direction(*values):
        return bits("01:00", "Foo Direction", "FDIR", unit="bits").model_copy(
            update={"values": list(values)}
        )

    word = bits(
        "03:00",
        "Foo Word",
        "FWRD",
        fields=[
            bits(
                "07:00",
                "Foo Opcode",
                "FOPC",
                unit="bits",
                fields=[
                    direction(value("0h", 0, "Foo None"), value("3h", 3, "Foo Both"))
                ],
            )
        ],
    )
    scope = bits("03:00", "Foo Scope", "FSCO", unit="bits").model_copy(
        update={"values": [value("0h", 0, "Foo Namespace"), value("1h", 1, "Foo Port")]}
    )
    unique = bits(
        "09:04",
        "Foo Unique",
        "FUID",
        fields=[bits("05", "Foo Scope Info", "FSSI", fields=[scope])],
    )
    other = bits(
        "11:10", "Foo Other Word", "FOWD", fields=[direction(value("1h", 1, "Foo Out"))]
    )
    shared = bits(
        "13:12",
        "Foo Shared Word",
        "FSWD",
        fields=[direction(value("1h", 1, "Foo Out"))],
    )
    model = structures_only(
        tmp_path, [structure("Foo Unit", [word, unique, other, shared])]
    )
    text = text_of(generate(model, tmp_path / "out"), "libxnvme_spec_foo.h")

    assert "#define XNVME_SPEC_FOO_FOO_UNIT_FWRD_FOPC_FDIR" not in text
    assert (
        "enum xnvme_spec_foo_foo_unit_fwrd_fopc_fdir {\n"
        "\tXNVME_SPEC_FOO_FOO_UNIT_FWRD_FOPC_FDIR_FOO_NONE = 0x0,"
    ) in text
    assert "\tXNVME_SPEC_FOO_FOO_UNIT_FWRD_FOPC_FDIR_FOO_BOTH = 0x3," in text
    # FSCO is the one field of its acronym in the document
    assert (
        "enum xnvme_spec_foo_fsco {\n\tXNVME_SPEC_FOO_FSCO_FOO_NAMESPACE = 0x0,"
    ) in text
    # FDIR of FOWD and of FSWD have the same values, which two fields against
    # one make the enum of the document, written once
    assert text.count("enum xnvme_spec_foo_fdir {") == 1
    assert "\tXNVME_SPEC_FOO_FDIR_FOO_OUT = 0x1," in text
    assert "enum xnvme_spec_foo_foo_unit_fowd_fdir" not in text


def test_a_field_within_a_field_of_an_entry_or_register_has_an_enum(tmp_path):
    """The struct of an entry that a bit layout defines, such as the Power
    State Descriptor, and a register, have no mask for a field within a field
    that has one, and an enum of its values"""

    def kind(name, acronym):
        return bits("01:00", name, acronym, unit="bits").model_copy(
            update={"values": [value("1h", 1, f"{name} One")]}
        )

    gear = structure(
        "Foo Gear Descriptor",
        [
            bits("127:29", "Reserved", unit="bits"),
            bits("28", "Foo Flag", "FFLG", unit="bits"),
            bits(
                "27:24",
                "Foo Gear",
                "FGR",
                unit="bits",
                fields=[kind("Foo Gear Kind", "FGK")],
            ),
            bits("15:00", "Foo Base", "FBASE", unit="bits"),
        ],
        kind="other",
        figure_nr=3,
        unit="bits",
    )

    def descriptor(index, range_):
        return bits(
            range_,
            f"Foo Gear Descriptor {index}",
            f"FGD{index}",
            structure=source(figure_nr=3),
            array=spec_model.Array(
                name="Foo Gear Descriptor",
                acronym="FGD",
                index=index,
                first=0,
                count=2,
                stride=16,
                lower=0,
            ),
        )

    state = register(
        "FST - Foo Status",
        0,
        [
            bits(
                "03:00",
                "Foo State",
                "FSTA",
                unit="bits",
                fields=[kind("Foo Ready", "FRDY")],
            ),
            bits("07:04", "Foo Spare", "FSPR", unit="bits"),
        ],
        figure_nr=10,
    )
    code = register(
        "FCC - Foo Class Code",
        4,
        [
            kind("Foo Base Class", "FBCC"),
            bits("23:16", "Foo Sub Class", "FSCC", unit="bits"),
        ],
        figure_nr=9,
    )
    model = structures_only(
        tmp_path,
        [
            structure("Foo Gears", [descriptor(0, "15:00"), descriptor(1, "31:16")]),
            gear,
            state,
            code,
        ],
    )
    text = text_of(generate(model, tmp_path / "out"), "libxnvme_spec_foo.h")

    assert (
        "#define XNVME_SPEC_FOO_FOO_GEAR_DESCRIPTOR_FFLG_FGR_FBASE_FGR 0xf000000"
        in text
    )
    assert "_FGR_FGK 0x" not in text
    assert "\tXNVME_SPEC_FOO_FGK_FOO_GEAR_KIND_ONE = 0x1," in text
    assert "\tXNVME_SPEC_FOO_FRDY_FOO_READY_ONE = 0x1," in text
    assert "\tXNVME_SPEC_FOO_FBCC_FOO_BASE_CLASS_ONE = 0x1," in text


def test_the_entries_that_follow_have_the_masks_of_their_layout_of_bits(tmp_path):
    """The fields of the Metadata Element Descriptor of Base 2.4 Figure 533 lie
    in its first 4 bytes, before an Element Value whose size the data gives"""

    element = structure(
        "Foo Element",
        [
            bits(
                "31+(FELEN*8):32",
                "Foo Element Value",
                "FEVAL",
                unit="bits",
                formula="31 + FELEN*8:32",
            ),
            bits("31:16", "Foo Element Length", "FELEN", unit="bits"),
            bits(
                "04:00",
                "Foo Element Type",
                "FETY",
                unit="bits",
                fields=[
                    bits("01:00", "Foo Element Class", "FECL", unit="bits").model_copy(
                        update={"values": [value("2h", 2, "Foo Plain")]}
                    )
                ],
            ),
        ],
        kind="data_structure",
        figure_nr=3,
        unit="bits",
    ).model_copy(update={"size": None})
    first = bits(
        "x:02",
        "Foo Element 0",
        formula="x:2",
        structure=source(figure_nr=3),
        array=spec_model.Array(name="Foo Element", index=0, first=0),
    )
    rows = [bits("00", "Foo Elements", "FNEL"), bits("01", "Reserved"), first]
    model = structures_only(tmp_path, [structure("Foo Metadata", rows), element])
    text = text_of(generate(model, tmp_path / "out"), "libxnvme_spec_foo.h")

    assert (
        " * Followed, from byte 2, by Foo Element entries, the first at bytes"
        " x:02, as many as the data says, each with XNVME_SPEC_FOO_FOO_ELEMENT_*"
        " in its bytes 3:0\n" in text
    )
    assert (
        "/** Foo Element Length (Foo 1.0, 1, Figure 3); bits 31:16 of"
        " 'Foo Element' */\n"
        "#define XNVME_SPEC_FOO_FOO_ELEMENT_FELEN 0xffff0000\n" in text
    )
    assert "#define XNVME_SPEC_FOO_FOO_ELEMENT_FETY 0x1f\n" in text
    assert "\tXNVME_SPEC_FOO_FECL_FOO_PLAIN = 0x2," in text
    assert "FEVAL" not in text
    status = yace_status(model, tmp_path)
    assert status is None or ", 0 diagnostics," in status


# The documents that the spec headers of include/ are written from
DOCUMENTS = [
    "base",
    "boot",
    "cp",
    "kv",
    "mi",
    "nvm",
    "pcie",
    "rdma",
    "slm",
    "tcp",
    "zns",
]


def test_pre_commit_formats_every_header_but_the_spec_headers():
    """clang-format alternates between two layouts of the long trailing
    comments of the spec headers, so each run changes the headers, and the hook
    C-format-headers excludes them by name"""

    yaml = pytest.importorskip("yaml")
    config = yaml.safe_load((ROOT / ".pre-commit-config.yaml").read_text())
    (hook,) = [
        hook
        for repo in config["repos"]
        for hook in repo["hooks"]
        if hook.get("name") == "C-format-headers"
    ]
    exclude = re.compile(hook["exclude"])
    for doc in DOCUMENTS:
        assert exclude.search(f"include/libxnvme_spec_{doc}.h")
    for path in sorted((ROOT / "include").glob("*.h")):
        written = "by toolbox/xnvme_spec_headers.py;" in path.read_text(
            encoding="utf-8"
        )
        assert bool(exclude.search(f"include/{path.name}")) == written, path.name
    for name in ("libxnvme_spec.h", "libxnvme_spec_fs.h", "libxnvme_spec_pp.h"):
        assert not exclude.search(f"include/{name}")
