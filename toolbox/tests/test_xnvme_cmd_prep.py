import importlib.util
import shutil
import subprocess
from pathlib import Path

import pytest

spec_model = pytest.importorskip("senfd.documents.spec")

SCRIPT = Path(__file__).parents[2] / "toolbox" / "xnvme_cmd_prep.py"
module = importlib.util.spec_from_file_location("xnvme_cmd_prep", SCRIPT)
prep = importlib.util.module_from_spec(module)
module.loader.exec_module(prep)

FOO = "NVM Express Foo Command Set Specification, Revision 1-0 2025-01-01 Ratified"


def source(figure_nr=1, section="1"):
    return spec_model.Source(document=FOO, figure_nr=figure_nr, section=section)


def bits(range_, name, acronym=None):
    upper, _, lower = range_.partition(":")
    upper, lower = int(upper), int(lower or upper)
    return spec_model.Bits(
        role="reserved" if name.startswith("Reserved") else "field",
        kind="span",
        width=upper - lower + 1,
        name=name,
        acronym=acronym,
        unit="bits",
        upper=upper,
        lower=lower,
        range=range_,
        description="",
        fields=[],
    )


def dwords(lower, upper, fields, condition=None, figure_nr=2, section="1"):
    return spec_model.Dwords(
        lower=lower,
        upper=upper,
        fields=fields,
        condition=condition,
        source=source(figure_nr, section),
    )


def command(name, opcode, sqe, columns=None, operations=()):
    return spec_model.Command(
        name=name,
        opcode=opcode,
        sqe=sqe,
        columns=columns or {},
        operations=list(operations),
        source=source(),
    )


def operation(name, sel, sqe):
    return spec_model.Operation(
        name=name, selectors={"SEL": sel}, sqe=sqe, source=source()
    )


def move(extra=(), operations=None):
    """An admin command that leaves CDW11 and CDW12 to its operations"""

    if operations is None:
        operations = [
            operation(
                "Go",
                0,
                [
                    dwords(11, 11, [bits("15:00", "Target", "TGT")], figure_nr=3),
                    dwords(12, 12, [bits("31:00", "Count", "CNT")], figure_nr=4),
                ],
            ),
            operation(
                "Stop", 1, [dwords(11, 11, [bits("0", "Now", "NOW")], figure_nr=5)]
            ),
        ]
    return command(
        "Move",
        0x43,
        [dwords(10, 10, [bits("07:00", "Select", "SEL")]), *extra],
        {"namespace_identifier_used": "No"},
        operations,
    )


def ding(extra=()):
    """A command with a plain field, a value split in halves, a dword with two
    descriptions under a condition, and a key split by its bit ranges"""

    return command(
        "Ding",
        0x81,
        [
            dwords(10, 11, [bits("63:00", "Starting Ding", "SDING")]),
            dwords(
                12,
                12,
                [
                    bits("31", "Force", "FRC"),
                    bits("19:16", "Extension Type", "CETYPE"),
                    *extra,
                    bits("15:00", "Ding Count Lower", "DCNTL"),
                ],
            ),
            dwords(13, 13, [bits("15:00", "Ding Count Upper", "DCNTU")]),
            dwords(14, 14, [bits("07:00", "Hint", "HNT")], "CETYPE is cleared to 0h"),
            dwords(
                14, 14, [bits("15:00", "Extension Value", "CEV")], "CETYPE is non-zero"
            ),
            dwords(2, 3, [bits("63:00", "Ding Key[63:00]")]),
        ],
    )


def admin():
    """An admin command without a namespace, and one without fields"""

    return {
        "dong": command(
            "Dong",
            0x42,
            [dwords(10, 10, [bits("07:00", "Dong Select", "DSEL")])],
            {"namespace_identifier_used": "No"},
        ),
        "keep": command("Keep", 0x18, [], {"namespace_identifier_used": "No"}),
    }


def write_model(path, commands, admin=()):
    document = spec_model.SpecDocument(
        command_sets={
            "nvm": spec_model.CommandSet(name="NVM", commands=commands),
            "admin": spec_model.CommandSet(name="Admin", commands=dict(admin)),
        },
    )
    path.write_text(document.model_dump_json(), encoding="utf-8")
    return path


@pytest.fixture
def model(tmp_path):
    return write_model(
        tmp_path / "merged.spec.document.json", {"ding": ding()}, admin()
    )


def generate(model, output, committed=None):
    options = {"spec": str(model), "committed": str(committed or output / "none.h")}
    return prep.CmdPrep(output, options).generate()


def header(wrote, abbr):
    (found,) = [h for h in wrote.headers if h.name == f"libxnvme_cmd_prep_{abbr}.h"]
    return found


def members(header, record):
    text = header.read_text(encoding="utf-8")
    body = text[
        text.index(f"struct {record} {{") : text.index("};", text.index(record))
    ]
    return [line.split("///<")[0].strip() for line in body.splitlines()[1:]]


def fields(header, record):
    """The members of 'record' without the reserved ones"""

    return [m for m in members(header, record) if not m.startswith("uint8_t rsvd")]


def test_a_record_is_its_size_then_its_fields(model, tmp_path):
    nvm = header(generate(model, tmp_path / "out"), "nvm")
    assert members(nvm, "xnvme_cmd_prep_nvm_ding_args") == [
        "uint32_t args_nbytes;",
        "uint32_t nsid;",
        "uint64_t sding;",
        "uint8_t frc;",
        "uint8_t cetype;",
        "uint8_t hnt;",
        "uint8_t rsvd19[1];",
        "uint16_t cev;",
        "uint8_t rsvd22[2];",
        "uint32_t dcnt;",
        "uint8_t ding_key[8];",
        "uint8_t rsvd36[4];",
    ]


def test_an_admin_command_without_a_namespace_has_no_nsid(model, tmp_path):
    adm = header(generate(model, tmp_path / "out"), "adm")
    assert members(adm, "xnvme_cmd_prep_adm_dong_args") == [
        "uint32_t args_nbytes;",
        "uint8_t dsel;",
        "uint8_t rsvd5[3];",
    ]


def test_a_command_without_fields_has_a_record_of_its_size(model, tmp_path):
    adm = header(generate(model, tmp_path / "out"), "adm")
    assert fields(adm, "xnvme_cmd_prep_adm_keep_args") == ["uint32_t args_nbytes;"]


def test_a_condition_the_generator_does_not_follow_stops_it(tmp_path):
    odd = command(
        "Odd", 0x01, [dwords(10, 10, [bits("0", "Bit", "BIT")], "MOON is full")]
    )
    with pytest.raises(ValueError, match="MOON is full"):
        generate(write_model(tmp_path / "m.json", {"odd": odd}), tmp_path / "out")


def test_new_fields_follow_the_committed_ones(model, tmp_path):
    first = header(generate(model, tmp_path / "first"), "nvm")
    grown = write_model(
        tmp_path / "grown.json",
        {"ding": ding(extra=[bits("20", "New Bit", "NEW")])},
        admin(),
    )
    nvm = header(generate(grown, tmp_path / "out", committed=first), "nvm")
    assert fields(nvm, "xnvme_cmd_prep_nvm_ding_args")[-1] == "uint8_t new;"


def test_a_record_ends_at_a_multiple_of_eight_bytes(model, tmp_path):
    adm = header(generate(model, tmp_path / "out"), "adm")
    assert members(adm, "xnvme_cmd_prep_adm_keep_args") == [
        "uint32_t args_nbytes;",
        "uint8_t rsvd4[4];",
    ]


def test_a_new_field_takes_the_place_of_the_reserved_bytes(model, tmp_path):
    first = header(generate(model, tmp_path / "first"), "nvm")
    before = members(first, "xnvme_cmd_prep_nvm_ding_args")
    grown = write_model(
        tmp_path / "grown.json",
        {"ding": ding(extra=[bits("20", "New Bit", "NEW")])},
        admin(),
    )
    nvm = header(generate(grown, tmp_path / "out", committed=first), "nvm")
    assert members(nvm, "xnvme_cmd_prep_nvm_ding_args") == before[:-1] + [
        "uint8_t new;",
        "uint8_t rsvd37[3];",
    ]


def test_a_record_grows_by_eight_bytes_when_the_reserved_bytes_are_used_up(
    model, tmp_path
):
    first = header(generate(model, tmp_path / "first"), "adm")
    wide = admin()
    wide["dong"].sqe.append(dwords(11, 11, [bits("31:00", "Wide", "WIDE")]))
    grown = write_model(tmp_path / "grown.json", {"ding": ding()}, wide)
    adm = header(generate(grown, tmp_path / "out", committed=first), "adm")
    assert members(adm, "xnvme_cmd_prep_adm_dong_args") == [
        "uint32_t args_nbytes;",
        "uint8_t dsel;",
        "uint8_t rsvd5[3];",
        "uint32_t wide;",
        "uint8_t rsvd12[4];",
    ]


def test_a_field_named_as_a_reserved_member_stops_the_generator(tmp_path):
    odd = command("Odd", 0x01, [dwords(10, 10, [bits("0", "Bit", "RSVD7")])])
    with pytest.raises(ValueError, match="'rsvd7' is the name of a reserved member"):
        generate(write_model(tmp_path / "m.json", {"odd": odd}), tmp_path / "out")


def test_the_dwords_of_the_operations_are_raw_members(tmp_path):
    model = write_model(tmp_path / "m.json", {}, {"move": move()})
    adm = header(generate(model, tmp_path / "out"), "adm")
    assert fields(adm, "xnvme_cmd_prep_adm_move_args") == [
        "uint32_t args_nbytes;",
        "uint8_t sel;",
        "uint32_t cdw11;",
        "uint32_t cdw12;",
    ]
    text = adm.read_text(encoding="utf-8")
    assert " * - CDW11, of Go: Foo 1.0, 1, Figure 3" in text
    assert " * - CDW11, of Stop: Foo 1.0, 1, Figure 5" in text
    assert "///< CDW11, operation-specific, as the operation describes it" in text


def test_a_dword_of_the_command_and_of_an_operation_stops_the_generator(tmp_path):
    both = move(extra=[dwords(11, 11, [bits("07:00", "Mine", "MINE")])])
    with pytest.raises(ValueError, match="CDW11 is described by the command and by"):
        generate(write_model(tmp_path / "m.json", {}, {"move": both}), tmp_path / "out")


def test_a_foreign_dword_is_left_out(tmp_path, monkeypatch):
    stray = [
        dwords(11, 11, [bits("15:00", "Stray", "STRAY")], section="9.9.9"),
        dwords(13, 13, [bits("31:00", "Astray", "ASTRAY")], section="9.9.9"),
    ]
    model = write_model(tmp_path / "m.json", {}, {"move": move(extra=stray)})
    monkeypatch.setitem(prep.FOREIGN, ("admin", "move"), (11, 13))
    adm = header(generate(model, tmp_path / "out"), "adm")
    assert fields(adm, "xnvme_cmd_prep_adm_move_args") == [
        "uint32_t args_nbytes;",
        "uint8_t sel;",
        "uint32_t cdw11;",
        "uint32_t cdw12;",
    ]


def test_a_foreign_dword_that_is_gone_stops_the_generator(tmp_path, monkeypatch):
    model = write_model(tmp_path / "m.json", {}, {"move": move()})
    monkeypatch.setitem(prep.FOREIGN, ("admin", "move"), (13,))
    with pytest.raises(ValueError, match="FOREIGN lists CDW13, which is gone"):
        generate(model, tmp_path / "out")


def test_dwords_from_two_sections_of_a_document_stop_the_generator(tmp_path):
    stray = [dwords(13, 13, [bits("31:00", "Astray", "ASTRAY")], section="5.2.30.1")]
    odd = move(extra=stray, operations=[])
    odd.sqe[0].source.section = "5.2.5"
    with pytest.raises(ValueError, match="CDW10 is from section 5.2.5 and CDW13"):
        generate(write_model(tmp_path / "m.json", {}, {"move": odd}), tmp_path / "out")


def test_each_header_defines_the_macro_of_its_command_set(model, tmp_path):
    wrote = generate(model, tmp_path / "out")
    assert "\n#define XNVME_CMD_PREP_ADM 1\n" in header(wrote, "adm").read_text()
    assert "\n#define XNVME_CMD_PREP_NVM 1\n" in header(wrote, "nvm").read_text()


@pytest.mark.parametrize(
    "edit, says",
    [
        (
            lambda t: t.replace("\tuint8_t hnt;", "\tuint8_t hnt;\n\tuint8_t gone;"),
            "'gone' is gone",
        ),
        (
            lambda t: t.replace("\tuint8_t frc;", "\tuint16_t frc;"),
            "'uint16_t frc;' is now",
        ),
        (
            lambda t: t
            + "struct xnvme_cmd_prep_nvm_lost_args {\n\tuint32_t args_nbytes;\n};\n",
            r"xnvme_cmd_prep_nvm_lost\(\): the command is gone",
        ),
    ],
)
def test_a_change_that_breaks_callers_stops_the_generator(model, tmp_path, edit, says):
    committed = tmp_path / "committed.h"
    first = header(generate(model, tmp_path / "first"), "nvm")
    committed.write_text(edit(first.read_text(encoding="utf-8")), encoding="utf-8")
    with pytest.raises(ValueError, match=says):
        generate(model, tmp_path / "out", committed=committed)


CHECK = r"""
#include <errno.h>
#include <stdio.h>
#include <libxnvme.h>

int
main(void)
{
    union xnvme_spec_sqe sqe;
    struct xnvme_cmd_prep_nvm_ding_args args = {
        .args_nbytes = sizeof(args),
        .nsid = 7,
        .sding = 0x1122334455667788ULL,
        .frc = 1,
        .dcnt = 0x12345678,
        .hnt = 0xAB,
        .ding_key = {1, 2, 3, 4, 5, 6, 7, 8},
    };
    struct xnvme_cmd_prep_adm_keep_args keep = {.args_nbytes = sizeof(keep)};

    if (xnvme_cmd_prep_nvm_ding(&sqe, &args)) {
        return 1;
    }
    for (int i = 0; i < 16; ++i) {
        printf("%08x\n", sqe.cdw[i]);
    }
    args.cev = 1;
    printf("%d\n", xnvme_cmd_prep_nvm_ding(&sqe, &args) == -EINVAL);
    args.frc = 2;
    args.cev = 0;
    printf("%d\n", xnvme_cmd_prep_nvm_ding(&sqe, &args) == -EINVAL);
    keep.args_nbytes = 0;
    printf("%d\n", xnvme_cmd_prep_adm_keep(&sqe, &keep) == -EINVAL);

    /* The reserved bytes of a record, and a record without compiler padding */
    args.frc = 1;
    printf("%d\n", xnvme_cmd_prep_nvm_ding(&sqe, &args) == 0);
    ((unsigned char *)&args)[sizeof(args) - 1] = 1;
    printf("%d\n", xnvme_cmd_prep_nvm_ding(&sqe, &args) == -EINVAL);
    printf("%d\n", sizeof(args) == 40 && sizeof(keep) == 8);
#ifdef XNVME_CMD_PREP_NVM
    printf("1\n");
#endif

    return 0;
}
"""

STUB = """#include <stddef.h>
#include <stdint.h>
#define XNVME_STATIC_ASSERT(cond, msg) _Static_assert(cond, msg);
union xnvme_spec_sqe {
    uint32_t cdw[16];
};
#include "libxnvme_cmd_prep_adm.h"
#include "libxnvme_cmd_prep_nvm.h"
"""


def test_the_functions_write_each_field_at_its_dword_and_bit(model, tmp_path):
    cc = shutil.which("cc") or shutil.which("gcc")
    if cc is None:
        pytest.skip("no C compiler")
    wrote = generate(model, tmp_path / "out")
    include = wrote.sources[0].parent
    (include / "libxnvme.h").write_text(STUB, encoding="utf-8")
    (tmp_path / "check.c").write_text(CHECK, encoding="utf-8")
    binary = tmp_path / "check"
    subprocess.run(
        [
            cc,
            "-std=c11",
            "-Wall",
            "-Wextra",
            "-Werror",
            "-I",
            str(include),
            "-o",
            str(binary),
            str(tmp_path / "check.c"),
            str(wrote.sources[0]),
        ],
        check=True,
    )
    lines = subprocess.run(
        [str(binary)], check=True, capture_output=True, text=True
    ).stdout.split()
    cdw = [int(line, 16) for line in lines[:16]]
    assert cdw[0] == 0x81 and cdw[1] == 7
    assert cdw[2:4] == [0x04030201, 0x08070605]
    assert cdw[10:12] == [0x55667788, 0x11223344]
    assert cdw[12] == (1 << 31) | 0x5678
    assert cdw[13] == 0x1234
    assert cdw[14] == 0xAB
    assert lines[16:] == ["1", "1", "1", "1", "1", "1", "1"]


def test_each_command_set_has_its_header(model, tmp_path):
    wrote = generate(model, tmp_path / "out")
    assert sorted(h.name for h in wrote.headers) == [
        "libxnvme_cmd_prep_adm.h",
        "libxnvme_cmd_prep_nvm.h",
    ]
    adm = header(wrote, "adm").read_text(encoding="utf-8")
    assert "Admin Command Set" in adm and "_nvm_" not in adm


def test_the_source_has_the_sets_in_order_each_by_opcode(model, tmp_path):
    (source,) = generate(model, tmp_path / "out").sources
    text = source.read_text(encoding="utf-8")
    order = [
        text.index(" * Admin Command Set"),
        text.index("xnvme_cmd_prep_adm_keep("),
        text.index("xnvme_cmd_prep_adm_dong("),
        text.index(" * NVM Command Set"),
        text.index("xnvme_cmd_prep_nvm_ding("),
    ]
    assert order == sorted(order)


def test_the_committed_headers_of_a_directory_are_kept(model, tmp_path):
    first = generate(model, tmp_path / "first")
    grown = write_model(
        tmp_path / "grown.json",
        {"ding": ding(extra=[bits("20", "New Bit", "NEW")])},
        admin(),
    )
    nvm = header(
        generate(grown, tmp_path / "out", committed=first.sources[0].parent), "nvm"
    )
    assert fields(nvm, "xnvme_cmd_prep_nvm_ding_args")[-1] == "uint8_t new;"


def test_the_record_cites_the_figures_and_its_members_name_their_bits(model, tmp_path):
    text = header(generate(model, tmp_path / "out"), "nvm").read_text(encoding="utf-8")
    record = text[: text.index("};", text.index("struct xnvme_cmd_prep_nvm_ding_args"))]
    assert " * - CDW12: Foo 1.0, 1, Figure 2" in record
    assert " * - CDW14, if CETYPE is cleared to 0h: Foo 1.0, 1, Figure 2" in record
    assert "\tuint8_t frc; ///< Force, CDW12 bit 31\n" in record
    assert "\tuint32_t dcnt; ///< Ding Count, DCNTL and DCNTU\n" in record


def test_a_bit_range_has_two_digits_on_each_side(model, tmp_path):
    text = header(generate(model, tmp_path / "out"), "nvm").read_text(encoding="utf-8")
    assert "\tuint8_t hnt; ///< Hint, CDW14 bits 07:00\n" in text


@pytest.mark.parametrize(
    "document, name",
    [
        ("Base Specification, Revision 2-4", "Base 2.4"),
        ("NVM Command Set Specification, Revision 1-3", "NVM Command Set 1.3"),
        (
            "Zoned Namespace Command Set Specification, Revision 1-5",
            "Zoned Namespace Command Set 1.5",
        ),
        (
            "Key Value Command Set Specification, Revision 1-4",
            "Key Value Command Set 1.4",
        ),
    ],
)
def test_a_command_set_specification_is_cited_by_its_full_name(document, name):
    where = spec_model.Source(
        document=f"NVM Express {document} Ratified 2026-07-31", figure_nr=2, section="1"
    )
    assert prep.cite(where) == f"{name}, 1, Figure 2"
