import importlib.util
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
):
    """A field whose range is 'range_': a range as the documents write it, such
    as "07:00", or a formula together with the 'formula' that senfd makes of it"""

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
                fields=[bits("15:00", "Foo Count", "FCNT", unit="bits")],
                source=source(figure_nr=3),
            )
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
                    bits("247:240", "Foo Version", "FVER", description="ASCII"),
                ],
            ),
            structure(
                "Foo Entry",
                [bits("07:00", "Foo Entry Key", "FEK"), bits("31:08", "Reserved")],
            ),
            structure(
                "Foo Range Entries",
                [
                    bits("03:00", "Foo Source", "FSRC"),
                    bits("07:04", "Foo Length", "FLEN"),
                    # The later entries are by acronym alone, as in the documents
                    bits("11:08", "FSRC"),
                    bits("15:12", "FLEN"),
                    bits("4091:4088", "FSRC"),
                    bits("4095:4092"),
                ],
                kind="other",
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
            structure(
                "Foo Reply: DW 1",
                [
                    bits("31:17", "Foo Status", "FSTS", unit="bits"),
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
            structure(
                "Bar Request PDU (BREQ)",
                [bits("07:00", "Bar Tag", "BTAG")],
                kind="other",
                document=BAR,
            ),
        ],
        values=[
            status("Generic Command", None, "C0h", "Foo Invalid", 4),
            status("Generic Command", "Foo Admin Commands", "C0h", "Foo Clash", 5),
        ],
        features={
            "foo_config": spec_model.Feature(
                identifier=0xC0, name="Foo Config", source=source(figure_nr=6)
            )
        },
        log_pages={
            "foo_events": spec_model.LogPage(
                identifier=0xC1, name="Foo Events", source=source(figure_nr=7)
            )
        },
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
        "xnvme_spec_bar_transport.h",
        "xnvme_spec_foo.h",
    ]
    # Only a document with commands has a source file, which holds the functions
    # that make the commands
    assert [p.name for p in wrote.sources] == ["xnvme_spec_foo.c"]
    text = text_of(wrote, "xnvme_spec_foo.h")
    assert "#ifndef XNVME_SPEC_FOO_H" in text
    # The header says what wrote it, and that it is not to be edited
    assert " * Written from the data model of senfd " in text
    assert " * edit: see the contributor docs on producing the spec headers." in text


def test_a_command_is_a_struct_of_its_dwords_its_masks_and_its_opcode(model, tmp_path):
    wrote = generate(model, tmp_path / "out")
    text = text_of(wrote, "xnvme_spec_foo.h")

    assert "enum xnvme_spec_foo_opcode {" in text
    assert "\tXNVME_SPEC_FOO_DING_OPCODE = 0x1, ///< Ding (Foo 1.0" in text
    assert "struct xnvme_spec_foo_ding {" in text
    assert (
        "\tuint32_t cdw10; ///< Command Dword 10, see XNVME_SPEC_FOO_DING_CDW10_*"
        in (text)
    )
    assert "#define XNVME_SPEC_FOO_DING_CDW10_FCNT 0xffff" in text
    assert "\tcmd->cdw0 = XNVME_SPEC_FOO_DING_OPCODE;" in text_of(
        wrote, "xnvme_spec_foo.c"
    )


def test_a_structure_is_a_packed_struct_at_the_offsets_of_its_fields(model, tmp_path):
    text = text_of(generate(model, tmp_path / "out"), "xnvme_spec_foo.h")

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
    text = text_of(generate(model, tmp_path / "out"), "xnvme_spec_foo.h")

    assert (
        " * Followed, from byte 16, by as many Foo Entry as the data says, each a"
        " 'struct xnvme_spec_foo_foo_entry'" in text
    )
    assert text.index("xnvme_spec_foo_foo_entry {") < text.index(
        "xnvme_spec_foo_foo_list {"
    )
    text = text_of(
        generate(model, tmp_path / "flexible", flexible="1"), "xnvme_spec_foo.h"
    )
    assert "\tstruct xnvme_spec_foo_foo_entry foo_entry[]; ///< Foo Entry" in text


def test_a_figure_of_the_entries_of_a_list_is_a_struct_of_one(model, tmp_path):
    text = text_of(generate(model, tmp_path / "out"), "xnvme_spec_foo.h")

    assert " * One entry of 8 bytes; the figure shows them at bytes 0, 8, 4088" in text
    assert "\tuint32_t fsrc; ///< Foo Source, bytes 3:0" in text
    assert "\tuint32_t flen; ///< Foo Length, bytes 7:4" in text
    assert "fsrc_8" not in text


def test_a_repeated_name_at_another_place_in_its_period_is_no_list(model, tmp_path):
    text = text_of(generate(model, tmp_path / "out"), "xnvme_spec_foo.h")

    assert "\tuint32_t fc_8; ///< Foo Code, bytes 11:8" in text


def test_a_figure_citing_fields_of_other_structures_is_no_struct(tmp_path):
    path = tmp_path / "spec.json"
    path.write_text(
        spec_model.SpecDocument(
            structures=[
                structure(
                    "Size and Capacity Fields",
                    [bits("07:00", "Namespace Size"), bits("15:08", "Namespace")],
                    kind="other",
                    document=ZNS,
                ),
                structure(
                    "Zone Thing", [bits("07:00", "Zone Key", "ZK")], document=ZNS
                ),
            ]
        ).model_dump_json()
    )

    text = text_of(generate(path, tmp_path / "out"), "xnvme_spec_zns.h")
    assert "size_and_capacity_fields" not in text
    assert "xnvme_spec_zns_zone_thing" in text

    named = generate(
        path, tmp_path / "named", structures="zns:size_and_capacity_fields"
    )
    assert "xnvme_spec_zns_size_and_capacity_fields" in text_of(
        named, "xnvme_spec_zns.h"
    )


def test_identifiers_and_status_codes_are_enums_by_document_and_scope(model, tmp_path):
    text = text_of(generate(model, tmp_path / "out"), "xnvme_spec_foo.h")

    assert "\tXNVME_SPEC_FOO_FEATURE_FOO_CONFIG = 0xc0," in text
    assert "\tXNVME_SPEC_FOO_LOG_PAGE_FOO_EVENTS = 0xc1," in text
    # A value with a different meaning in each of two scopes is in two enums
    assert "\tXNVME_SPEC_FOO_GENERIC_COMMAND_STATUS_FOO_INVALID = 0xc0," in text
    assert (
        "\tXNVME_SPEC_FOO_GENERIC_COMMAND_FOO_ADMIN_COMMANDS_STATUS_FOO_CLASH" in text
    )


def test_a_layout_of_no_named_kind_is_named_without_its_abbreviation(model, tmp_path):
    text = text_of(generate(model, tmp_path / "out"), "xnvme_spec_bar_transport.h")
    assert (
        "struct __attribute__((packed)) xnvme_spec_bar_transport_bar_request_pdu {"
        in (text)
    )


def test_a_field_takes_the_masks_of_the_layout_it_is_defined_in(model, tmp_path):
    text = text_of(generate(model, tmp_path / "out"), "xnvme_spec_foo.h")

    assert "#define XNVME_SPEC_FOO_FOO_FORMAT_FDW0_FTAG 0xffff0000" in text
    assert "#define XNVME_SPEC_FOO_FOO_FORMAT_FDW0_FOPC 0xff" in text
    # A field whose description only mentions a figure does not get the layout
    # of that figure
    assert "XNVME_SPEC_FOO_FOO_FORMAT_FPTR_" not in text


def test_an_entry_of_layouts_of_its_dwords_is_a_struct_of_their_halves(model, tmp_path):
    text = text_of(generate(model, tmp_path / "out"), "xnvme_spec_foo.h")

    assert "struct __attribute__((packed)) xnvme_spec_foo_foo_reply {" in text
    assert "\tuint32_t dw0; ///< Dword 0, of no layout in the data model" in text
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


def test_io_is_one_word_in_names():
    assert headers.alias("Create I/O Completion Queue") == "create_io_completion_queue"
    assert headers.identifier("I/O Command Set") == "IO_COMMAND_SET"
    assert headers.set_key_of("i_o") == "io"


def test_only_what_is_asked_for_is_written(model, tmp_path):
    wrote = generate(
        model, tmp_path / "out", commands="none", structures="none", registers="none"
    )
    text = text_of(wrote, "xnvme_spec_foo.h")
    assert "struct" not in text
    assert wrote.sources == []


def test_yace_takes_the_headers_in_its_strict_subset(model, tmp_path):
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
            "--strict",
        ],
        capture_output=True,
        text=True,
    )
    assert proc.returncode == 0, proc.stderr
    (status,) = (tmp_path / "out").glob("*.status")
    assert status.read_text().endswith("0 diagnostics\n")


def test_the_registers_are_a_struct_at_their_offsets_with_masks_and_values(
    model, tmp_path
):
    text = text_of(generate(model, tmp_path / "out"), "xnvme_spec_foo.h")

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
        generate(model, tmp_path / "out", registers="none"), "xnvme_spec_foo.h"
    )
    assert "_registers" not in text
