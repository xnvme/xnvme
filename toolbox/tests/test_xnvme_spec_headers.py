import importlib.util
import re
import shutil
import subprocess
import sys
from pathlib import Path

import pytest

spec_model = pytest.importorskip("senfd.documents.spec")

SCRIPT = Path(__file__).parents[2] / "toolbox" / "xnvme_spec_headers.py"
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
    """A field whose range is 'range_': a range as the documents write it, such
    as "07:00", or a formula together with the 'formula' that senfd makes of it.
    'links' are what senfd links the field to: 'structure', 'array',
    'values_in' and 'cases'"""

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


def entry(index):
    """The group of a field that is a part of entry 'index' of a list of 512
    entries of 8 bytes, as senfd gives it"""

    return spec_model.Array(
        name="Foo Range Entry", index=index, first=0, count=512, stride=8, lower=0
    )


def structure(
    name, fields, kind="data_structure", document=FOO, figure_nr=2, unit="bytes"
):
    return spec_model.Structure(
        name=name,
        kind=kind,
        unit=unit,
        fields=fields,
        source=source(document, figure_nr),
    )


def register(name, offset, fields, figure_nr):
    return spec_model.Structure(
        name=name,
        kind="register",
        unit="bits",
        offset=offset,
        fields=fields,
        source=source(figure_nr=figure_nr),
    )


def value(value_, number, name=None):
    typed = {"value": spec_model.Typed(kind="number", lower=number, upper=number)}
    return spec_model.Entry(value=value_, value_column="value", name=name, typed=typed)


def status(qualifier, scope, value, name, figure_nr):
    return spec_model.Values(
        kind="status_values",
        name="Status Code",
        qualifier=qualifier,
        scope=scope,
        columns=["value", "description"],
        entries=[spec_model.Entry(value=value, value_column="value", name=name)],
        source=source(figure_nr=figure_nr),
    )


@pytest.fixture
def model(tmp_path):
    """A data model of two documents, in the format that senfd writes, with
    every form that the generator handles"""

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
                                # A value without a name makes no constant
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
                fields=[
                    bits("31:16", "Foo Specific", "FSPC", unit="bits"),
                    bits("15:00", "Reserved", unit="bits"),
                ],
                source=source(figure_nr=31),
            ),
            spec_model.Dwords(
                lower=13,
                upper=13,
                condition="FCE is non-zero",
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
            # An operation of the name of another one
            spec_model.Operation(
                name="Foo Begin",
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
        source=source(figure_nr=1),
    )
    document = spec_model.SpecDocument(
        command_sets={
            "foo": spec_model.CommandSet(name="Foo", commands={"ding": ding})
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
                    bits("31:00", "Foo Slot 0 Descriptor").model_copy(
                        update={"lower": 16, "upper": 47, "range": "47:16"}
                    ),
                    bits("79:48", "Foo Slot 1 Descriptor"),
                    bits("239:208", "Foo Slot 6 Descriptor"),
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
                    ),
                    bits(
                        "03:02",
                        "Foo Format 1",
                        "FFMT1",
                        description="The format is defined in Figure 15.",
                        structure=source(figure_nr=15),
                    ),
                    bits(
                        "19:04",
                        "Foo Power Descriptor 0",
                        "FPD0",
                        description="The format is defined in Figure 14.",
                        structure=source(figure_nr=14),
                    ),
                    bits(
                        "35:20",
                        "Foo Power Descriptor 1",
                        "FPD1",
                        description="The format is defined in Figure 14.",
                        structure=source(figure_nr=14),
                    ),
                    bits(
                        "37:36",
                        "Foo Spare Format 0",
                        "FSF0",
                        structure=source(figure_nr=15),
                    ),
                    bits(
                        "39:38",
                        "Foo Spare Format 1",
                        "FSF1",
                        structure=source(figure_nr=15),
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
                    bits("15:08", "Foo Data Size", "FDS", unit="bits"),
                    bits("07:00", "Foo Metadata Size", "FMS", unit="bits"),
                ],
                kind="other",
                figure_nr=15,
                unit="bits",
            ),
            # The entries of a list, each field with the entry it is part of;
            # the later entries are by acronym alone, as in the documents
            structure(
                "Foo Range Entries",
                [
                    bits("03:00", "Foo Source", "FSRC", group=entry(0)),
                    bits("07:04", "Foo Length", "FLEN", group=entry(0)),
                    bits("11:08", "FSRC", group=entry(1)),
                    bits("15:12", "FLEN", group=entry(1)),
                    bits("4091:4088", "FSRC", group=entry(511)),
                    bits("4095:4092", group=entry(511)),
                ],
                kind="other",
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
                    bits("47:16", "Foo Entry 1"),
                    bits("79:48", "Foo Entry 2"),
                    bits(
                        "(NFE*32)+15:(NFE*32)-16",
                        "Foo Entry NFE",
                        formula="32*NFE + 15:32*NFE - 16",
                    ),
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
                "Foo Format",
                [
                    bits(
                        "03:00",
                        "Foo Dword 0",
                        "FDW0",
                        description="Defined in Figure 11.",
                        structure=source(figure_nr=11),
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
            # A list of dwords, linked to the layouts of some of them, and a
            # field of a dword linked to its own layout
            structure(
                "Foo Reply",
                [
                    bits("03:00", "Dword 0", "DW0"),
                    bits("07:04", "Dword 1", "DW1", structure=source(figure_nr=12)),
                ],
                kind="other",
                figure_nr=91,
            ),
            structure(
                "Foo Reply: DW 1",
                [
                    bits(
                        "31:17",
                        "Foo Status",
                        "FSTS",
                        unit="bits",
                        structure=source(figure_nr=13),
                    ),
                    bits("16", "Foo Phase", "FP", unit="bits"),
                    bits("15:00", "Foo Tag", "FTAG", unit="bits"),
                ],
                kind="other",
                figure_nr=12,
                unit="bits",
            ),
            structure(
                "Foo Reply: Foo Status Field",
                [
                    bits("31", "Foo Final", "FFIN", unit="bits"),
                    bits("24:17", "Foo Code", "FCODE", unit="bits"),
                ],
                kind="other",
                figure_nr=13,
                unit="bits",
            ),
            register(
                "FOOCAP - Foo Capabilities",
                "0h",
                [
                    bits("63:32", "Reserved", unit="bits"),
                    bits("15:00", "Foo Limit", "FL", unit="bits"),
                ],
                8,
            ),
            register(
                "FOOCC - Foo Configuration",
                "10h",
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
                                value("111b", 7, "Reserved"),
                            ]
                        }
                    ),
                    bits("31:03", "Reserved", unit="bits"),
                ],
                9,
            ),
            register("FOOX - Foo Elsewhere", "1000h + (4 * y)", [], 10),
            register(
                "FOOVS - Foo Version",
                "18h",
                [
                    bits("31:16", "Foo Major", "FMJR", unit="bits"),
                    bits("15:00", "Foo Minor", "FMNR", unit="bits"),
                ],
                42,
            ),
            # Fields that report a register, linked to its bit layout
            structure(
                "Foo Identify",
                [
                    bits(
                        "03:00", "Foo Version", "FVER", structure=source(figure_nr=42)
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
                                # split depends on the memory page size
                                fields=[
                                    bits(
                                        "23:16",
                                        "Foo PRP Entry 2",
                                        "FPRP2",
                                        structure=source(figure_nr=44),
                                    ),
                                    bits(
                                        "15:08",
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
                                        "23:08",
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
            structure(
                "Bar Request PDU (BREQ)",
                [bits("07:00", "Bar Tag", "BTAG")],
                kind="other",
                document=BAR,
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
            status("Generic Command", None, "C0h", "Foo Invalid", 4),
            status("Generic Command", "Foo Admin Commands", "C0h", "Foo Clash", 5),
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
                    # lists here gets no mask where it overlaps
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
        ("NVM Express PCI Express Transport Specification, Revision 1.1", "pcie"),
        ("NVM Express NVMe over PCIe Transport Specification, Revision 1.4", "pcie"),
        ("NVM Express NVMe over RDMA Transport Specification, Revision 1.3", "rdma"),
        ("NVM Express NVMe over TCP Transport Specification, Revision 1.3", "tcp"),
    ],
)
def test_a_document_is_named_by_its_abbreviation_whatever_its_revision_titles_it(
    title, key
):
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
    # A gap is a reserved member; a run of fields named by index is an array,
    # even with rows left out
    assert "\tuint8_t rsvd9[7]; ///< Reserved, bytes 15:9" in text
    assert "\tuint8_t foo_slot_descriptor[224]; ///< Foo Slot Descriptor, 7 of 32" in (
        text
    )
    # A string is an array of bytes whatever its size
    assert "\tuint8_t fver[8]; ///< Foo Version, bytes 247:240" in text


def test_entries_as_many_as_the_data_says_follow_the_struct(model, tmp_path):
    text = text_of(generate(model, tmp_path / "out"), "libxnvme_spec_foo.h")

    assert (
        " * Followed, from byte 16, by as many Foo Entry as the data says, each a"
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
        "\tuint64_t foo_change[3]; ///< Foo Change, up to 3 of 8 bytes, as many as NFC"
        " says, bytes 31:8" in struct
    )
    assert "Followed" not in struct


def test_a_figure_of_the_entries_of_a_list_is_a_struct_of_one(model, tmp_path):
    text = text_of(generate(model, tmp_path / "out"), "libxnvme_spec_foo.h")

    assert " * One entry of 8 bytes; the figure shows them at bytes 0, 8, 4088" in text
    assert "\tuint32_t fsrc; ///< Foo Source, bytes 3:0" in text
    assert "\tuint32_t flen; ///< Foo Length, bytes 7:4" in text
    assert "fsrc_8" not in text


def test_a_repeated_name_at_another_place_in_its_period_is_no_list(model, tmp_path):
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


def test_identifiers_and_status_codes_are_enums_by_document_and_scope(model, tmp_path):
    text = text_of(generate(model, tmp_path / "out"), "libxnvme_spec_foo.h")

    assert "\tXNVME_SPEC_FOO_FEATURE_FOO_CONFIG = 0xc0," in text
    assert "\tXNVME_SPEC_FOO_LOG_PAGE_FOO_EVENTS = 0xc1," in text
    # A value with a different meaning in each of two scopes is in two enums
    assert "\tXNVME_SPEC_FOO_GENERIC_COMMAND_STATUS_FOO_INVALID = 0xc0," in text
    assert (
        "\tXNVME_SPEC_FOO_GENERIC_COMMAND_FOO_ADMIN_COMMANDS_STATUS_FOO_CLASH" in text
    )


def test_a_layout_of_no_named_kind_is_named_without_its_abbreviation(model, tmp_path):
    text = text_of(generate(model, tmp_path / "out"), "libxnvme_spec_bar_transport.h")
    assert (
        "struct __attribute__((packed)) xnvme_spec_bar_transport_bar_request_pdu {"
        in (text)
    )


def test_a_field_takes_the_masks_of_the_layout_it_is_defined_in(model, tmp_path):
    text = text_of(generate(model, tmp_path / "out"), "libxnvme_spec_foo.h")

    assert "#define XNVME_SPEC_FOO_FOO_FORMAT_FDW0_FTAG 0xffff0000" in text
    assert "#define XNVME_SPEC_FOO_FOO_FORMAT_FDW0_FOPC 0xff" in text
    # A field whose description only mentions a figure does not get the layout
    # of that figure
    assert "XNVME_SPEC_FOO_FOO_FORMAT_FPTR_" not in text


def test_a_field_takes_the_masks_of_the_register_it_reports(model, tmp_path):
    text = text_of(generate(model, tmp_path / "out"), "libxnvme_spec_foo.h")

    assert (
        "\tuint32_t fver; ///< Foo Version, bytes 3:0, see"
        " XNVME_SPEC_FOO_FOO_IDENTIFY_FVER_*" in text
    )
    assert "#define XNVME_SPEC_FOO_FOO_IDENTIFY_FVER_FMJR 0xffff0000" in text
    assert "#define XNVME_SPEC_FOO_FOO_IDENTIFY_FVER_FMNR 0xffff" in text
    # The register keeps its own masks
    assert "#define XNVME_SPEC_FOO_REGISTERS_FOOVS_FMJR 0xffff0000" in text
    # A field of another width than the layout gets none of its masks
    assert "XNVME_SPEC_FOO_FOO_IDENTIFY_FWIDE_" not in text


def test_an_entry_of_layouts_of_its_dwords_is_a_struct_of_their_halves(model, tmp_path):
    text = text_of(generate(model, tmp_path / "out"), "libxnvme_spec_foo.h")

    assert "struct __attribute__((packed)) xnvme_spec_foo_foo_reply {" in text
    assert "\tuint32_t dw0; ///< Dword 0, bytes 3:0" in text
    assert "\tuint16_t ftag; ///< Foo Tag, bytes 5:4" in text
    assert (
        "\tuint16_t fsts; ///< Foo Status and Foo Phase, bytes 7:6, see"
        " XNVME_SPEC_FOO_FOO_REPLY_FSTS_*" in text
    )
    # The masks of the layout of the field come first, then the masks of the
    # rest of the dword
    assert "#define XNVME_SPEC_FOO_FOO_REPLY_FSTS_FFIN 0x8000" in text
    assert "#define XNVME_SPEC_FOO_FOO_REPLY_FSTS_FCODE 0x1fe" in text
    assert "#define XNVME_SPEC_FOO_FOO_REPLY_FSTS_FP 0x1" in text
    assert "XNVME_SPEC_FOO_FOO_REPLY_FSTS_FSTS" not in text
    # A member that one field fills has no mask for that field
    assert "XNVME_SPEC_FOO_FOO_REPLY_FTAG_" not in text


def structures_only(tmp_path, structures):
    """A data model of 'structures' alone"""

    document = spec_model.SpecDocument(
        structures=structures, meta=spec_model.DocumentMeta(version="9.8.7")
    )
    path = tmp_path / "structures.spec.document.json"
    path.write_text(document.model_dump_json(), encoding="utf-8")
    return path


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


def test_two_structures_of_one_name_are_told_apart_by_their_parenthesis(tmp_path):
    model = structures_only(
        tmp_path,
        [
            structure("Foo Stamp (Set Foo)", [bits("07:00", "Foo When", "FWHN")]),
            structure(
                "Foo Stamp (Get Foo)",
                [bits("07:00", "Foo When", "FWHN"), bits("08", "Foo Late", "FLT")],
                figure_nr=3,
            ),
        ],
    )
    text = text_of(generate(model, tmp_path / "out"), "libxnvme_spec_foo.h")
    assert "struct __attribute__((packed)) xnvme_spec_foo_foo_stamp {" in text
    assert "struct __attribute__((packed)) xnvme_spec_foo_foo_stamp_get_foo {" in text


def test_io_is_one_word_in_names():
    assert headers.alias("Create I/O Completion Queue") == "create_io_completion_queue"
    assert headers.identifier("I/O Command Set") == "IO_COMMAND_SET"
    assert headers.set_key_of("common_io") == "io"
    assert headers.set_key_of("pcie_commands") == "pcie_commands"


def test_only_what_is_asked_for_is_written(model, tmp_path):
    wrote = generate(
        model, tmp_path / "out", commands="none", structures="none", registers="none"
    )
    text = text_of(wrote, "libxnvme_spec_foo.h")
    assert "struct" not in text
    assert wrote.sources == []


def test_yace_takes_the_headers_in_its_c_subset(model, tmp_path):
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
    # The named values of a field are an enum; reserved and unnamed values are
    # left out
    assert "enum xnvme_spec_foo_registers_foocc_fms {" in text
    assert "\tXNVME_SPEC_FOO_REGISTERS_FOOCC_FMS_FOO_FAST = 0x0," in text
    assert "_FMS_RESERVED" not in text
    # A register whose offset is not a number is named in the doc, not placed
    assert " * FOOX - Foo Elsewhere: at no offset of a number" in text


def test_the_registers_are_left_out_when_asked(model, tmp_path):
    text = text_of(
        generate(model, tmp_path / "out", registers="none"), "libxnvme_spec_foo.h"
    )
    assert "_registers" not in text


def test_the_named_values_of_a_field_are_an_enum_by_its_mask(model, tmp_path):
    text = text_of(generate(model, tmp_path / "out"), "libxnvme_spec_foo.h")

    # Of a command
    assert "enum xnvme_spec_foo_ding_cdw10_fact {" in text
    assert "\tXNVME_SPEC_FOO_DING_CDW10_FACT_FOO_START = 0x0," in text
    assert "\tXNVME_SPEC_FOO_DING_CDW10_FACT_FOO_STOP = 0x1," in text
    assert "\tXNVME_SPEC_FOO_DING_CDW10_FACT_FOO_PAUSE = 0x3," in text
    assert not re.search(r"_CDW10_FACT_\w+ = 0x2,", text)
    # A name of more than eight words
    assert (
        "\tXNVME_SPEC_FOO_DING_CDW10_FACT_FOO_STATISTICS_DATA_STRUCTURE_FOR_AN"
        "_ALLOCATED_FOO_ID = 0x5," in text
    )
    # By the acronym of the value of another field that it refers to
    assert (
        "\tXNVME_SPEC_FOO_DING_CDW10_FACT_FSE = 0x4,"
        " ///< List the foos in the FSE:Empty state. (Foo 1.0" in text
    )
    # Of a field that is the whole member of a structure
    assert "enum xnvme_spec_foo_foo_entry_fek {" in text
    assert "\tXNVME_SPEC_FOO_FOO_ENTRY_FEK_FOO_PRIMARY = 0x1," in text


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
    # Operations of one name are named by their first selector too
    assert "#define XNVME_SPEC_FOO_DING_FOO_BEGIN_FKIND1H_CDW12_FAGN 0xff" in text
    assert "#define XNVME_SPEC_FOO_DING_FOO_BEGIN_FKIND2H_CDW12_FAGN 0xff" in text


def test_an_array_takes_the_layout_of_its_entry_from_the_figure_named(model, tmp_path):
    text = text_of(generate(model, tmp_path / "out"), "libxnvme_spec_foo.h")

    # An entry that is a scalar keeps its type, with masks named by the entry
    # The entry is named as the struct of Foo Format, so its masks take its
    # figure
    assert (
        "\tuint16_t ffmt[2]; ///< Foo Format, 2 of 2 bytes of"
        " XNVME_SPEC_FOO_FOO_FORMAT_FIGURE15_*, bytes 3:0" in text
    )
    assert "#define XNVME_SPEC_FOO_FOO_FORMAT_FIGURE15_FDS 0xff00" in text
    assert "#define XNVME_SPEC_FOO_FOO_FORMAT_FIGURE15_FMS 0xff" in text
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
    # VAL is the name yace prints the value of a field by, so it gets no mask
    assert "\tuint8_t val; ///< Valid, bit 7 of byte 14" in text
    assert "_VAL_VAL" not in text
    assert "\tuint8_t rsvd15[1]; ///< Reserved" in text


def test_a_field_takes_its_values_from_the_table_it_refers_to(model, tmp_path):
    text = text_of(generate(model, tmp_path / "out"), "libxnvme_spec_foo.h")

    assert "enum xnvme_spec_foo_ding_cdw11_fsel {" in text
    assert (
        "\tXNVME_SPEC_FOO_DING_CDW11_FSEL_FOO_FAR = 0x1, ///< Foo Far (Foo 1.0, 1,"
        " Figure 23)" in text
    )


def test_an_array_that_senfd_links_is_an_array_of_its_entry(model, tmp_path):
    text = text_of(generate(model, tmp_path / "out"), "libxnvme_spec_foo.h")

    # The figure leaves out the entries between the first and the last, and
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


def test_a_field_with_cases_gets_their_masks_and_values(model, tmp_path):
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


def test_a_case_that_is_a_field_of_all_the_bits_gets_their_mask(model, tmp_path):
    text = text_of(generate(model, tmp_path / "out"), "libxnvme_spec_foo.h")

    assert "#define XNVME_SPEC_FOO_DING_CDW11_FOPT 0x10000" in text
    assert (
        "/** Foo Hold (FHOLD) (Foo 1.0, 1, Figure 19), when FACT is Foo Start or Foo"
        " Pause; bit 16 of Command Dword 11 */\n"
        "#define XNVME_SPEC_FOO_DING_CDW11_FOPT_FHOLD 0x10000" in text
    )


def test_a_field_the_data_model_gives_as_big_endian_is_bytes(model, tmp_path):
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
    # A record of no field but the field itself adds nothing
    assert "\tuint8_t fhid[16]; ///< Foo Host Identifier, bytes 31:16" in struct
    # A record of another document is in another header
    assert "\tuint8_t bkey[16]; ///< Bar Key, bytes 47:32" in struct
    # A record of another size does not lay the field out
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
        " XNVME_SPEC_FOO_FOO_FORMAT_FIGURE15_*, bytes 39:36" in text
    )
    assert text.count("#define XNVME_SPEC_FOO_FOO_FORMAT_FIGURE15_FDS ") == 1
    assert text.count("enum xnvme_spec_foo_foo_format_figure15_") == 0


def test_a_feature_is_the_masks_of_its_dwords_and_values(model, tmp_path):
    text = text_of(generate(model, tmp_path / "out"), "libxnvme_spec_foo.h")

    assert " * The fields of the feature Foo Config (Foo 1.0" in text
    assert "#define XNVME_SPEC_FOO_FEATURE_FOO_CONFIG_CDW11_FLV 0x30000" in text
    assert "#define XNVME_SPEC_FOO_FEATURE_FOO_CONFIG_CDW11_FLIM 0xffff" in text
    assert "bits 17:16 of Command Dword 11 */" in text
    assert "_FOO_CONFIG_CDW11_FOTH" not in text
    assert "#define XNVME_SPEC_FOO_FEATURE_FOO_CONFIG_CQE_DW0_FGNT 0xffff" in text
    assert "bits 15:0 of Completion Queue Entry Dword 0 */" in text
    assert "enum xnvme_spec_foo_feature_foo_config_cdw11_flv {" in text
    assert "\tXNVME_SPEC_FOO_FEATURE_FOO_CONFIG_CDW11_FLV_FOO_HIGH = 0x1," in text
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
        " non-zero; a field of a later condition gets a mask where no field of an"
        " earlier one has its bits" in text
    )
    assert (
        "/** Foo Specific (Foo 1.0, 1, Figure 31), when FCE is cleared to 0h; bits"
        " 31:16 of Command Dword 13 */\n#define XNVME_SPEC_FOO_DING_CDW13_FSPC"
        " 0xffff0000" in text
    )
    assert text.count("#define XNVME_SPEC_FOO_DING_CDW13_FSPC ") == 1
    assert (
        "/** Foo Extension Value (Foo 1.0, 1, Figure 32), when FCE is non-zero; bits"
        " 15:0 of Command Dword 13 */\n#define XNVME_SPEC_FOO_DING_CDW13_FCEV"
        " 0xffff" in text
    )


def test_a_value_is_named_by_no_column_of_values_and_by_a_name_before_a_colon(
    model, tmp_path
):
    text = text_of(generate(model, tmp_path / "out"), "libxnvme_spec_foo.h")

    start = text.index("enum xnvme_spec_foo_foo_descriptor_fsubt {")
    enum = text[start : text.index("};", start)]
    assert enum.count("\tXNVME_SPEC_FOO_FOO_DESCRIPTOR_FSUBT_") == 3
    assert "\tXNVME_SPEC_FOO_FOO_DESCRIPTOR_FSUBT_FOO_ADDRESS = 0x0," in enum
    assert "\tXNVME_SPEC_FOO_FOO_DESCRIPTOR_FSUBT_FOO_OFFSET = 0x1," in enum
    assert "\tXNVME_SPEC_FOO_FOO_DESCRIPTOR_FSUBT_512B = 0x3," in enum
