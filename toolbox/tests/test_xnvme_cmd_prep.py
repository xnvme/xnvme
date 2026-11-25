import importlib.util
import re
import shutil
import subprocess
import sys
from pathlib import Path

import pytest

spec_model = pytest.importorskip("senfd.documents.spec")

ROOT = Path(__file__).parents[2]
SCRIPT = ROOT / "toolbox" / "xnvme_cmd_prep.py"
module = importlib.util.spec_from_file_location("xnvme_cmd_prep", SCRIPT)
prep = importlib.util.module_from_spec(module)
module.loader.exec_module(prep)

FOO = "NVM Express Foo Command Set Specification, Revision 1-0 2025-01-01 Ratified"


def source(figure_nr=1, section="1"):
    return spec_model.Source(document=FOO, figure_nr=figure_nr, section=section)


def bits(range_, name, acronym=None, part=None):
    """A field of bits. 'part' is the name, the acronym and the shift of the
    value that the field is a part of."""

    upper, _, lower = range_.partition(":")
    upper, lower = int(upper), int(lower or upper)
    if part:
        part = spec_model.Part(name=part[0], acronym=part[1], shift=part[2])
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
        part=part,
    )


def dwords(
    lower, upper, fields, condition=None, figure_nr=2, section="1", refines=None
):
    return spec_model.Dwords(
        lower=lower,
        upper=upper,
        fields=fields,
        condition=condition,
        selected_when=spec_model.comparison_of(condition),
        refines=refines,
        source=source(figure_nr, section),
    )


def command(name, opcode, sqe, nsid=True, operations=(), delegated=()):
    """A command; without 'nsid', its opcode table says that the Namespace
    Identifier is not used"""

    typed = {}
    if not nsid:
        typed["namespace_identifier_used"] = spec_model.Typed(kind="flag", flag=False)
    return spec_model.Command(
        name=name,
        opcode=opcode,
        sqe=sqe,
        typed=typed,
        operations=list(operations),
        delegated=list(delegated),
        source=source(),
    )


def operation(name, sel, sqe):
    return spec_model.Operation(
        name=name, selectors={"SEL": sel}, sqe=sqe, source=source()
    )


def move(extra=(), operations=None):
    """An admin command whose operations describe CDW11 and CDW12"""

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
        False,
        operations,
    )


def ding(extra=()):
    """A command with a plain field, a reserved field, a field of 7 bits, a
    value split in halves, a dword with two descriptions under a condition,
    and a key split by its bit ranges over two pairs of dwords"""

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
                    bits("30:21", "Reserved"),
                    bits("19:16", "Extension Type", "CETYPE"),
                    *extra,
                    bits(
                        "15:00", "Ding Count Lower", "DCNTL", ("Ding Count", "DCNT", 0)
                    ),
                ],
            ),
            dwords(
                13,
                13,
                [
                    bits(
                        "15:00", "Ding Count Upper", "DCNTU", ("Ding Count", "DCNT", 16)
                    ),
                    bits("22:16", "Ding Level", "DLVL"),
                ],
            ),
            dwords(14, 14, [bits("07:00", "Hint", "HNT")], "CETYPE is cleared to 0h"),
            dwords(
                14, 14, [bits("15:00", "Extension Value", "CEV")], "CETYPE is non-zero"
            ),
            dwords(
                2, 3, [bits("63:00", "Ding Key[63:00]", None, ("Ding Key", None, 0))]
            ),
            dwords(
                4,
                5,
                [bits("63:00", "Ding Key[127:64]", None, ("Ding Key", None, 64))],
            ),
        ],
    )


def admin():
    """An admin command without a namespace, and one without fields"""

    return {
        "dong": command(
            "Dong",
            0x42,
            [dwords(10, 10, [bits("07:00", "Dong Select", "DSEL")])],
            False,
        ),
        "keep": command("Keep", 0x18, [], False),
    }


def write_model(path, commands, admin=()):
    document = spec_model.SpecDocument(
        command_sets={
            "nvm": spec_model.CommandSet(
                name="NVM", title="NVM Command Set", commands=commands
            ),
            "admin": spec_model.CommandSet(
                name="Admin", title="Admin Command Set", commands=dict(admin)
            ),
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


def test_a_record_is_the_fields_of_the_command(model, tmp_path):
    nvm = header(generate(model, tmp_path / "out"), "nvm")
    assert members(nvm, "xnvme_cmd_prep_nvm_ding_args") == [
        "uint32_t nsid;",
        "uint8_t rsvd4[4];",
        "uint64_t sding;",
        "uint8_t frc;",
        "uint8_t cetype;",
        "uint8_t dlvl;",
        "uint8_t hnt;",
        "uint16_t cev;",
        "uint8_t rsvd22[2];",
        "uint32_t dcnt;",
        "uint8_t ding_key[16];",
        "uint8_t rsvd44[4];",
    ]


def test_an_admin_command_without_a_namespace_has_no_nsid(model, tmp_path):
    adm = header(generate(model, tmp_path / "out"), "adm")
    assert members(adm, "xnvme_cmd_prep_adm_dong_args") == [
        "uint8_t dsel;",
        "uint8_t rsvd1[7];",
    ]


def test_a_command_without_fields_has_a_record_of_one_reserved_member(model, tmp_path):
    adm = header(generate(model, tmp_path / "out"), "adm")
    assert members(adm, "xnvme_cmd_prep_adm_keep_args") == ["uint8_t rsvd0[8];"]


def test_a_condition_without_a_comparison_stops_the_generator(tmp_path):
    odd = command(
        "Odd", 0x01, [dwords(10, 10, [bits("0", "Bit", "BIT")], "MOON is full")]
    )
    with pytest.raises(ValueError, match="MOON is full"):
        generate(write_model(tmp_path / "m.json", {"odd": odd}), tmp_path / "out")


def test_new_fields_follow_the_committed_ones(model, tmp_path):
    first = header(generate(model, tmp_path / "first"), "nvm")
    grown = write_model(
        tmp_path / "grown.json",
        {"ding": ding(extra=[bits("20", "Next Bit", "NXT")])},
        admin(),
    )
    nvm = header(generate(grown, tmp_path / "out", committed=first), "nvm")
    assert fields(nvm, "xnvme_cmd_prep_nvm_ding_args")[-1] == "uint8_t nxt;"


def test_a_record_ends_at_a_multiple_of_eight_bytes(model, tmp_path):
    nvm = header(generate(model, tmp_path / "out"), "nvm")
    assert members(nvm, "xnvme_cmd_prep_nvm_ding_args")[-1] == "uint8_t rsvd44[4];"


def test_a_new_field_replaces_the_reserved_bytes(model, tmp_path):
    first = header(generate(model, tmp_path / "first"), "nvm")
    before = members(first, "xnvme_cmd_prep_nvm_ding_args")
    grown = write_model(
        tmp_path / "grown.json",
        {"ding": ding(extra=[bits("20", "Next Bit", "NXT")])},
        admin(),
    )
    nvm = header(generate(grown, tmp_path / "out", committed=first), "nvm")
    assert members(nvm, "xnvme_cmd_prep_nvm_ding_args") == before[:-1] + [
        "uint8_t nxt;",
        "uint8_t rsvd45[3];",
    ]


def test_a_record_grows_by_eight_bytes_when_the_reserved_bytes_are_used_up(
    model, tmp_path
):
    first = header(generate(model, tmp_path / "first"), "adm")
    wide = admin()
    wide["dong"].sqe.append(dwords(11, 12, [bits("63:00", "Wide", "WIDE")]))
    grown = write_model(tmp_path / "grown.json", {"ding": ding()}, wide)
    adm = header(generate(grown, tmp_path / "out", committed=first), "adm")
    assert members(adm, "xnvme_cmd_prep_adm_dong_args") == [
        "uint8_t dsel;",
        "uint8_t rsvd1[7];",
        "uint64_t wide;",
    ]


def test_a_field_named_as_a_reserved_member_stops_the_generator(tmp_path):
    odd = command("Odd", 0x01, [dwords(10, 10, [bits("0", "Bit", "RSVD7")])])
    with pytest.raises(ValueError, match="'rsvd7' is the name of a reserved member"):
        generate(write_model(tmp_path / "m.json", {"odd": odd}), tmp_path / "out")


def test_the_dwords_of_the_operations_are_raw_members(tmp_path):
    model = write_model(tmp_path / "m.json", {}, {"move": move()})
    adm = header(generate(model, tmp_path / "out"), "adm")
    assert fields(adm, "xnvme_cmd_prep_adm_move_args") == [
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


def test_dwords_of_an_operation_that_refine_a_field_are_no_raw_members(
    tmp_path,
):
    events = operation(
        "Events",
        0,
        [dwords(10, 10, [bits("07:00", "Kind", "KIND")], figure_nr=3, refines="SEL")],
    )
    model = write_model(tmp_path / "m.json", {}, {"move": move(operations=[events])})
    adm = header(generate(model, tmp_path / "out"), "adm")
    assert fields(adm, "xnvme_cmd_prep_adm_move_args") == ["uint8_t sel;"]
    assert "of Events" not in adm.read_text(encoding="utf-8")


def test_the_dwords_that_a_command_delegates_are_raw_members(tmp_path):
    """The dwords whose use a field of a command selects, as FID selects the
    use of CDW11 to CDW15 of Set Features, are raw members, except a dword
    that the command describes, as CDW14 of Set Features"""

    tune = command(
        "Tune",
        0x09,
        [
            dwords(10, 10, [bits("07:00", "Feature Identifier", "FID")]),
            dwords(14, 14, [bits("06:00", "UUID Index", "UIDX")], figure_nr=3),
        ],
        False,
        delegated=[
            spec_model.Delegation(
                fields=["DPTR", "CDW11", "CDW14", "CDW15"], selectors=["FID"]
            )
        ],
    )
    model = write_model(tmp_path / "m.json", {}, {"tune": tune})
    adm = header(generate(model, tmp_path / "out"), "adm")
    assert fields(adm, "xnvme_cmd_prep_adm_tune_args") == [
        "uint8_t fid;",
        "uint8_t uidx;",
        "uint32_t cdw11;",
        "uint32_t cdw15;",
    ]
    assert "///< CDW11, as the feature or operation describes it" in adm.read_text()


def test_a_data_model_of_another_format_stops_the_generator(model, tmp_path):
    document = spec_model.SpecDocument.model_validate_json(model.read_text())
    document.meta.format = 3
    model.write_text(document.model_dump_json(), encoding="utf-8")
    with pytest.raises(ValueError, match="the data model is of format 3"):
        generate(model, tmp_path / "out")


def test_the_files_name_the_senfd_that_wrote_the_data_model(model, tmp_path):
    document = spec_model.SpecDocument.model_validate_json(model.read_text())
    document.meta.version = "9.8.7"
    model.write_text(document.model_dump_json(), encoding="utf-8")
    wrote = generate(model, tmp_path / "out")
    for path in [header(wrote, "nvm"), *wrote.sources]:
        assert "Written from the data model of senfd 9.8.7, by" in path.read_text()


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
            + "struct xnvme_cmd_prep_nvm_lost_args {\n\tuint32_t nsid;\n};\n",
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
#include <string.h>
#include <libxnvme.h>

int
main(void)
{
    union xnvme_cmd_sqe sqe;
    struct xnvme_cmd_prep_nvm_ding_args args = {
        .nsid = 7,
        .sding = 0x1122334455667788ULL,
        .frc = 1,
        .dcnt = 0x12345678,
        .dlvl = 0x7F,
        .hnt = 0xAB,
        .ding_key = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16},
    };
    struct xnvme_cmd_prep_adm_keep_args keep = {0};
    unsigned char big[sizeof(args) + 8] = {0};

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
    printf("%d\n", xnvme_cmd_prep_adm_keep_sized(&sqe, &keep, 0) == -EINVAL);
    printf("%d\n", xnvme_cmd_prep_adm_keep(NULL, &keep) == -EINVAL);
    printf("%d\n", xnvme_cmd_prep_adm_keep(&sqe, NULL) == -EINVAL);
    args.frc = 1;
    args.dlvl = 0x80;
    printf("%d\n", xnvme_cmd_prep_nvm_ding(&sqe, &args) == -EINVAL);
    args.dlvl = 0x7F;

    /* Under the other condition, CEV is written and the prep returns -EINVAL for HNT */
    args.frc = 1;
    args.cetype = 1;
    args.cev = 0xBEEF;
    printf("%d\n", xnvme_cmd_prep_nvm_ding(&sqe, &args) == -EINVAL);
    args.hnt = 0;
    printf("%d\n", xnvme_cmd_prep_nvm_ding(&sqe, &args) == 0 && sqe.cdw[14] == 0xBEEF);
    args.cetype = 0;
    args.cev = 0;
    args.hnt = 0xAB;

    /* The reserved bytes of a record, and a record without compiler padding */
    args.frc = 1;
    printf("%d\n", xnvme_cmd_prep_nvm_ding(&sqe, &args) == 0);
    args.rsvd4[0] = 1;
    printf("%d\n", xnvme_cmd_prep_nvm_ding(&sqe, &args) == -EINVAL);
    args.rsvd4[0] = 0;
    keep.rsvd0[0] = 1;
    printf("%d\n", xnvme_cmd_prep_adm_keep(&sqe, &keep) == -EINVAL);
    keep.rsvd0[0] = 0;
    ((unsigned char *)&args)[sizeof(args) - 1] = 1;
    printf("%d\n", xnvme_cmd_prep_nvm_ding(&sqe, &args) == -EINVAL);
    printf("%d\n", sizeof(args) == 48 && sizeof(keep) == 8);

    /* A record of an older header is smaller; the members it lacks are zero */
    ((unsigned char *)&args)[sizeof(args) - 1] = 0;
    printf("%d\n", xnvme_cmd_prep_nvm_ding_sized(&sqe, &args, 8) == 0 &&
                       sqe.cdw[1] == 7 && sqe.cdw[10] == 0);
    /* Every record is a multiple of 8 bytes; a size ending inside a member is no record */
    printf("%d\n", xnvme_cmd_prep_nvm_ding_sized(&sqe, &args, 10) == -EINVAL);
    /* A record of a newer header is larger; the bytes this one lacks are zero */
    memcpy(big, &args, sizeof(args));
    printf("%d\n", xnvme_cmd_prep_nvm_ding_sized(&sqe, (const void *)big, sizeof(big)) == 0);
    big[sizeof(args)] = 1;
    printf("%d\n", xnvme_cmd_prep_nvm_ding_sized(&sqe, (const void *)big, sizeof(big)) ==
                       -E2BIG);
#ifdef XNVME_CMD_PREP_NVM
    printf("1\n");
#endif

    return 0;
}
"""

STUB = """#include <stddef.h>
#include <stdint.h>
#define XNVME_STATIC_ASSERT(cond, msg) _Static_assert(cond, msg);
union xnvme_cmd_sqe {
    uint32_t cdw[16];
};
#include "libxnvme_cmd_prep_adm.h"
#include "libxnvme_cmd_prep_nvm.h"
#include "libxnvme_cmd_prep_nvm.h"
"""


@pytest.mark.parametrize("libxnvme", ["stub", "include"])
def test_the_functions_write_each_field_at_its_dword_and_bit(model, tmp_path, libxnvme):
    """Built with a stub of libxnvme.h, and with include/libxnvme.h of the
    repository and the headers of the preps"""

    cc = shutil.which("cc") or shutil.which("gcc")
    if cc is None:
        pytest.skip("no C compiler")
    wrote = generate(model, tmp_path / "out")
    include = wrote.sources[0].parent
    if libxnvme == "stub":
        (include / "libxnvme.h").write_text(STUB, encoding="utf-8")
        flags = ["-I", str(include)]
    else:
        # libxnvme.h includes the committed preps by quotes, from its own directory
        repo = tmp_path / "include"
        repo.mkdir()
        for header in (ROOT / "include").glob("*.h"):
            if header.name not in {h.name for h in wrote.headers}:
                shutil.copy(header, repo)
        flags = ["-D_GNU_SOURCE", "-I", str(repo), "-I", str(include)]
        for name in ("libxnvme.h", *(h.name for h in sorted(wrote.headers))):
            flags += ["-include", name]
    (tmp_path / "check.c").write_text(CHECK, encoding="utf-8")
    binary = tmp_path / "check"
    subprocess.run(
        [
            cc,
            "-std=c11",
            "-Wall",
            "-Wextra",
            "-Werror",
            *flags,
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
    assert cdw[4:6] == [0x0C0B0A09, 0x100F0E0D]
    assert cdw[10:12] == [0x55667788, 0x11223344]
    assert cdw[12] == (1 << 31) | 0x5678
    assert cdw[13] == (0x7F << 16) | 0x1234
    assert cdw[14] == 0xAB
    assert lines[16:] == ["1"] * 18


def test_each_command_set_has_its_header(model, tmp_path):
    wrote = generate(model, tmp_path / "out")
    assert sorted(h.name for h in wrote.headers) == [
        "libxnvme_cmd_prep_adm.h",
        "libxnvme_cmd_prep_nvm.h",
    ]
    adm = header(wrote, "adm").read_text(encoding="utf-8")
    assert "Admin Command Set" in adm and "_nvm_" not in adm


def test_the_io_commands_of_base_are_the_set_common_io(tmp_path):
    path = tmp_path / "m.json"
    document = spec_model.SpecDocument(
        command_sets={
            "common_io": spec_model.CommandSet(
                name="Common I/O", title="I/O Commands", commands={"ding": ding()}
            ),
        },
    )
    path.write_text(document.model_dump_json(), encoding="utf-8")
    wrote = generate(path, tmp_path / "out")
    assert [h.name for h in wrote.headers] == ["libxnvme_cmd_prep_io.h"]
    text = header(wrote, "io").read_text(encoding="utf-8")
    assert "I/O Commands" in text and "xnvme_cmd_prep_io_ding(" in text


def test_the_source_has_the_sets_in_order_each_by_opcode(model, tmp_path):
    (source,) = generate(model, tmp_path / "out").sources
    text = source.read_text(encoding="utf-8")
    order = [
        text.index(" * Admin Command Set"),
        text.index("xnvme_cmd_prep_adm_keep_sized("),
        text.index("xnvme_cmd_prep_adm_dong_sized("),
        text.index(" * NVM Command Set"),
        text.index("xnvme_cmd_prep_nvm_ding_sized("),
    ]
    assert order == sorted(order)


def test_the_committed_headers_of_a_directory_are_kept(model, tmp_path):
    first = generate(model, tmp_path / "first")
    grown = write_model(
        tmp_path / "grown.json",
        {"ding": ding(extra=[bits("20", "Next Bit", "NXT")])},
        admin(),
    )
    nvm = header(
        generate(grown, tmp_path / "out", committed=first.sources[0].parent), "nvm"
    )
    assert fields(nvm, "xnvme_cmd_prep_nvm_ding_args")[-1] == "uint8_t nxt;"


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


def test_a_prep_is_a_function_of_the_size_and_an_inline_function_of_the_record(
    model, tmp_path
):
    text = header(generate(model, tmp_path / "out"), "nvm").read_text(encoding="utf-8")
    assert (
        " * @recparam args The fields of the command, see"
        " ::xnvme_cmd_prep_nvm_ding_args\n * @param args_nbytes" in text
    )
    assert (
        " * @errno\n */\nint\nxnvme_cmd_prep_nvm_ding_sized(union xnvme_cmd_sqe *sqe,"
        " const struct xnvme_cmd_prep_nvm_ding_args *args, size_t args_nbytes);" in text
    )
    assert (
        "static inline int\nxnvme_cmd_prep_nvm_ding(union xnvme_cmd_sqe *sqe, const"
        " struct xnvme_cmd_prep_nvm_ding_args *args)\n{\n\treturn"
        " xnvme_cmd_prep_nvm_ding_sized(sqe, args, sizeof(*args));\n}" in text
    )
    # The errors that the function returns are those it checks for
    doc = text.replace("\n * ", " ")
    assert (
        "-EINVAL when 'sqe' or 'args' is NULL, 'args_nbytes' is zero or not a multiple"
        " of 8, a reserved byte is not zero, a value does not fit its field, or a field is not zero while"
        " its condition does not hold; -E2BIG when" in doc
    )


def generated_source(model, tmp_path):
    (found,) = generate(model, tmp_path / "out").sources
    return found.read_text(encoding="utf-8")


def ding_with(*sqe):
    """A command of a field CETYPE in CDW12 and the dwords 'sqe'"""

    return command(
        "Ding",
        0x81,
        [dwords(12, 12, [bits("19:16", "Extension Type", "CETYPE")]), *sqe],
    )


def test_the_prep_returns_einval_for_a_field_set_while_its_condition_does_not_hold(
    tmp_path,
):
    ding = ding_with(
        dwords(14, 14, [bits("07:00", "Hint", "HNT")], "CETYPE is set to 1h"),
        dwords(
            14, 14, [bits("15:00", "Extension Value", "CEV")], "CETYPE is set to 2h"
        ),
    )
    text = generated_source(write_model(tmp_path / "m.json", {"ding": ding}), tmp_path)
    assert "\tif (a.cetype != 1 && a.hnt) {" in text
    assert "\tif (a.cetype != 2 && a.cev) {" in text
    assert "\tif (a.cetype == 1) {\n\t\tprep_bits(sqe, 14, 0, 8, a.hnt);" in text


def test_a_field_of_both_descriptions_of_a_dword_is_written_whatever_the_condition(
    tmp_path,
):
    ding = ding_with(
        dwords(
            14,
            14,
            [bits("31:24", "Tag", "TAG"), bits("07:00", "Hint", "HNT")],
            "CETYPE is cleared to 0h",
        ),
        dwords(
            14,
            14,
            [bits("31:24", "Tag", "TAG"), bits("15:00", "Value", "CEV")],
            "CETYPE is non-zero",
        ),
    )
    text = generated_source(write_model(tmp_path / "m.json", {"ding": ding}), tmp_path)
    assert "\n\tprep_bits(sqe, 14, 24, 8, a.tag); ///< Tag\n" in text
    assert "a.tag) {" not in text


def test_a_field_in_descriptions_that_do_not_cover_every_value_stops_the_generator(
    tmp_path,
):
    ding = ding_with(
        dwords(14, 14, [bits("07:00", "Hint", "HNT")], "CETYPE is set to 1h"),
        dwords(14, 14, [bits("07:00", "Hint", "HNT")], "CETYPE is set to 2h"),
        dwords(14, 14, [bits("07:00", "Other", "OTH")], "CETYPE is cleared to 0h"),
    )
    model = write_model(tmp_path / "m.json", {"ding": ding})
    with pytest.raises(ValueError, match="hnt is in descriptions of CDW14"):
        generate(model, tmp_path / "out")


@pytest.mark.parametrize(
    "sqe, says",
    [
        ([dwords(12, 12, [bits("07:00", "Cap", "128CCS")])], "'128ccs' is no C"),
        ([dwords(12, 12, [bits("07:00", "Interrupt", "INT")])], "'int' is no C"),
        ([dwords(12, 12, [bits("07:00", "Class", "CLASS")])], "'class' is no C"),
        ([dwords(12, 12, [bits("07:00", "Error", "ERRNO")])], "'errno' is no C"),
        ([dwords(12, 12, [bits("07:00", "Linux", "LINUX")])], "'linux' is no C"),
        ([dwords(12, 12, [bits("07:00", "Near", "NEAR")])], "'near' is no C"),
        ([dwords(12, 12, [bits("31:00", "Other", "NSID")])], "two members named nsid"),
        (
            [
                dwords(12, 12, [bits("07:00", "One", "ONE")]),
                dwords(13, 13, [bits("07:00", "Two", "ONE")]),
            ],
            "two fields named one",
        ),
        (
            [dwords(14, 14, [bits("07:00", "Hint", "HNT")], "SEL is set to 1h")],
            "'SEL is set to 1h' of hnt is on no member",
        ),
        (
            [
                dwords(
                    12, 12, [bits("15:00", "Count Lower", "CNTL", ("Count", "CNT", 0))]
                ),
                dwords(
                    13, 13, [bits("15:00", "Count Upper", "CNTU", ("Count", "CNT", 32))]
                ),
            ],
            "the parts of 'Count' do not follow each other",
        ),
        (
            [
                dwords(
                    12, 12, [bits("15:00", "Count Lower", "CNTL", ("Count", "CNT", 0))]
                )
            ],
            "'Count' has one part only",
        ),
        (
            [dwords(12, 12, [bits("11:00", "Key[11:00]", None, ("Key", None, 0))])],
            "'Key' is not whole bytes",
        ),
        (
            [dwords(10, 13, [bits("95:00", "Wide", "WIDE")])],
            "wide is 96 bits, wider than a member of 64 bits",
        ),
        (
            [
                dwords(
                    10,
                    13,
                    [bits("127:00", "Wide Key[127:00]", None, ("Wide Key", None, 0))],
                ),
                dwords(
                    14,
                    15,
                    [bits("63:00", "Wide Key[191:128]", None, ("Wide Key", None, 128))],
                ),
            ],
            r"Wide Key\[127:00\] is 128 bits, wider than a member of 64 bits",
        ),
        (
            [
                dwords(10, 12, [bits("95:00", "Tag Lower", "TAGL", ("Tag", "TAG", 0))]),
                dwords(
                    13, 13, [bits("31:00", "Tag Upper", "TAGU", ("Tag", "TAG", 96))]
                ),
            ],
            "tagl is 96 bits, wider than a member of 64 bits",
        ),
        (
            [dwords(14, 14, [bits("39:32", "Far", "FAR")])],
            "Far has the bits 39:32, which are not bits of CDW14",
        ),
        (
            [dwords(14, 14, [bits("32:24", "Far", "FAR")])],
            "Far has the bits 32:24, which are not bits of CDW14",
        ),
        (
            [dwords(1, 1, [bits("15:00", "Group", "GRP")])],
            "grp and nsid take the same bits",
        ),
        (
            [dwords(0, 0, [bits("07:00", "Opcode", "OPC")])],
            "opc and the opcode take the same bits",
        ),
        (
            [dwords(0, 0, [bits("31:16", "Command Identifier", "CID")])],
            "cid and the command identifier take the same bits",
        ),
        (
            [
                dwords(
                    14,
                    14,
                    [bits("15:00", "Wide", "WIDE"), bits("07:00", "Hint", "HNT")],
                )
            ],
            "hnt and wide take the same bits",
        ),
        (
            [
                dwords(12, 12, [bits("07:00", "Select", "SEL")]),
                dwords(14, 14, [bits("15:00", "Wide", "WIDE")]),
                dwords(14, 14, [bits("07:00", "Hint", "HNT")], "SEL is set to 1h"),
            ],
            "hnt and wide take the same bits",
        ),
        (
            [
                dwords(12, 12, [bits("07:00", "Select", "SEL")]),
                dwords(
                    14,
                    14,
                    [bits("15:00", "Wide", "WIDE"), bits("07:00", "Hint", "HNT")],
                    "SEL is set to 1h",
                ),
            ],
            "hnt and wide take the same bits",
        ),
        (
            [
                dwords(12, 12, [bits("07:00", "Select", "SEL")]),
                dwords(14, 14, [bits("15:00", "Value", "VAL")], "SEL is non-zero"),
                dwords(14, 14, [bits("07:00", "Hint", "HNT")], "SEL is set to 1h"),
            ],
            "hnt and val take the same bits",
        ),
        (
            [
                dwords(
                    12,
                    12,
                    [bits("07:00", "Select", "SEL"), bits("15:08", "Type", "TYP")],
                ),
                dwords(14, 14, [bits("15:00", "Value", "VAL")], "SEL is set to 1h"),
                dwords(14, 14, [bits("07:00", "Hint", "HNT")], "TYP is set to 2h"),
            ],
            "hnt and val take the same bits",
        ),
        (
            [
                dwords(
                    12,
                    12,
                    [
                        bits("07:00", "Hint", "HNT"),
                        bits("15:00", "Count Lower", "CNTL", ("Count", "CNT", 0)),
                    ],
                ),
                dwords(
                    13, 13, [bits("15:00", "Count Upper", "CNTU", ("Count", "CNT", 16))]
                ),
            ],
            "Count Lower and hnt take the same bits",
        ),
        (
            [
                dwords(
                    12,
                    12,
                    [
                        bits("15:00", "Count Lower", "CNTL", ("Count", "CNT", 0)),
                        bits("07:00", "Hint", "HNT"),
                    ],
                ),
                dwords(
                    13, 13, [bits("15:00", "Count Upper", "CNTU", ("Count", "CNT", 16))]
                ),
            ],
            "hnt and Count Lower take the same bits",
        ),
        (
            [
                dwords(12, 12, [bits("07:00", "Select", "SEL")]),
                dwords(14, 14, [bits("31:24", "Tag", "TAG")], "SEL is cleared to 0h"),
                dwords(
                    14,
                    14,
                    [bits("31:24", "Tag", "TAG"), bits("31:16", "Value", "VAL")],
                    "SEL is non-zero",
                ),
            ],
            "val and tag take the same bits",
        ),
        (
            [
                dwords(12, 12, [bits("07:00", "Select", "SEL")]),
                dwords(14, 14, [bits("31:24", "Tag", "TAG")], "SEL is cleared to 0h"),
                dwords(
                    14,
                    14,
                    [bits("31:16", "Value", "VAL"), bits("31:24", "Tag", "TAG")],
                    "SEL is non-zero",
                ),
            ],
            "tag and val take the same bits",
        ),
        (
            [
                dwords(12, 12, [bits("07:00", "Select", "SEL")]),
                dwords(
                    13,
                    13,
                    [bits("15:00", "Count Lower", "CNTL", ("Count", "CNT", 0))],
                    "SEL is set to 1h",
                ),
                dwords(
                    14,
                    14,
                    [bits("15:00", "Count Upper", "CNTU", ("Count", "CNT", 16))],
                    "SEL is set to 1h",
                ),
            ],
            "Count Lower is a part of 'Count' in CDW13 if 'SEL is set to 1h'",
        ),
    ],
)
def test_a_field_without_a_member_in_c_stops_the_generator(tmp_path, sqe, says):
    model = write_model(tmp_path / "m.json", {"odd": command("Odd", 0x01, sqe)})
    with pytest.raises(ValueError, match=says):
        generate(model, tmp_path / "out")


@pytest.mark.parametrize(
    "operations, delegated, says",
    [
        (
            [operation("Go", 0, [dwords(1, 1, [bits("15:00", "Group", "GRP")])])],
            [],
            "cdw1 and nsid take the same bits",
        ),
        (
            [operation("Go", 0, [dwords(0, 0, [bits("15:08", "Flags", "FLG")])])],
            [],
            "cdw0 and the opcode take the same bits",
        ),
        (
            [],
            [spec_model.Delegation(fields=["CDW0"], selectors=["SEL"])],
            "cdw0 and the opcode take the same bits",
        ),
    ],
)
def test_a_raw_member_in_the_bits_the_function_writes_stops_the_generator(
    tmp_path, operations, delegated, says
):
    odd = command(
        "Odd",
        0x01,
        [dwords(10, 10, [bits("07:00", "Select", "SEL")])],
        True,
        operations,
        delegated,
    )
    model = write_model(tmp_path / "m.json", {"odd": odd})

    with pytest.raises(ValueError, match=says):
        generate(model, tmp_path / "out")


def test_a_value_wider_than_64_bits_is_a_member_for_each_part(tmp_path):
    tags = command(
        "Tags",
        0x01,
        [
            dwords(14, 14, [bits("31:00", "Tag Lower", "TAGL", ("Tag", "TAG", 0))]),
            dwords(15, 15, [bits("15:00", "Tag Middle", "TAGM", ("Tag", "TAG", 32))]),
            dwords(13, 13, [bits("31:00", "Tag Upper", "TAGU", ("Tag", "TAG", 48))]),
        ],
    )
    nvm = header(
        generate(write_model(tmp_path / "m.json", {"tags": tags}), tmp_path / "out"),
        "nvm",
    )
    assert fields(nvm, "xnvme_cmd_prep_nvm_tags_args") == [
        "uint32_t nsid;",
        "uint32_t tagl;",
        "uint16_t tagm;",
        "uint32_t tagu;",
    ]
    # Each member is written whole, at the bits of its part
    source = (tmp_path / "out" / "xnvme_cmd_prep" / "xnvme_cmd_prep.c").read_text()
    assert "\tprep_bits(sqe, 13, 0, 32, a.tagu); ///< Tag Upper\n" in source
    assert "\tprep_bits(sqe, 14, 0, 32, a.tagl); ///< Tag Lower\n" in source
    assert "\tprep_bits(sqe, 15, 0, 16, a.tagm); ///< Tag Middle\n" in source


def test_a_value_of_two_32_bit_parts_is_one_64_bit_member(tmp_path):
    offset = command(
        "Seek",
        0x01,
        [
            dwords(
                10, 10, [bits("31:00", "Offset Lower", "OFSL", ("Offset", "OFS", 0))]
            ),
            dwords(
                11, 11, [bits("31:00", "Offset Upper", "OFSU", ("Offset", "OFS", 32))]
            ),
        ],
    )
    wrote = generate(
        write_model(tmp_path / "m.json", {"seek": offset}), tmp_path / "out"
    )

    assert fields(header(wrote, "nvm"), "xnvme_cmd_prep_nvm_seek_args") == [
        "uint32_t nsid;",
        "uint64_t ofs;",
    ]
    source = wrote.sources[0].read_text(encoding="utf-8")
    assert "\tprep_bits(sqe, 10, 0, 32, a.ofs); ///< Offset Lower\n" in source
    assert "\tprep_bits(sqe, 11, 0, 32, a.ofs >> 32); ///< Offset Upper\n" in source


def test_mptr_and_dptr_are_no_members(tmp_path):
    ptrs = command(
        "Ptrs",
        0x01,
        [
            dwords(4, 5, [bits("63:00", "Metadata Pointer", "MPTR")]),
            dwords(6, 9, [bits("127:00", "Data Pointer", "DPTR")]),
            dwords(10, 10, [bits("07:00", "Count", "CNT")]),
        ],
    )
    nvm = header(
        generate(write_model(tmp_path / "m.json", {"ptrs": ptrs}), tmp_path / "out"),
        "nvm",
    )
    assert fields(nvm, "xnvme_cmd_prep_nvm_ptrs_args") == [
        "uint32_t nsid;",
        "uint8_t cnt;",
    ]


@pytest.mark.parametrize(
    "commands, records",
    [
        ("nvm", ["xnvme_cmd_prep_nvm_ding_args"]),
        ("admin:dong", ["xnvme_cmd_prep_adm_dong_args"]),
        (
            "nvm:ding,admin:keep",
            ["xnvme_cmd_prep_adm_keep_args", "xnvme_cmd_prep_nvm_ding_args"],
        ),
    ],
)
def test_the_option_commands_selects_sets_and_commands(
    model, tmp_path, commands, records
):
    options = {
        "spec": str(model),
        "committed": str(tmp_path / "none.h"),
        "commands": commands,
    }
    wrote = prep.CmdPrep(tmp_path / "out", options).generate()
    text = "".join(h.read_text(encoding="utf-8") for h in wrote.headers)
    assert sorted(set(re.findall(r"^struct (\w+) \{$", text, re.M))) == records


def test_each_record_has_the_size_of_its_layout_asserted(model, tmp_path):
    text = generated_source(model, tmp_path)
    assert (
        "XNVME_STATIC_ASSERT(sizeof(struct xnvme_cmd_prep_nvm_ding_args) == 48," in text
    )
    assert (
        "XNVME_STATIC_ASSERT(sizeof(struct xnvme_cmd_prep_adm_keep_args) == 8," in text
    )


def test_the_source_has_prep_le64_only_for_a_value_of_bytes(tmp_path):
    plain = write_model(tmp_path / "plain.json", {}, admin())
    assert "prep_le64" not in generated_source(plain, tmp_path / "plain")
    keyed = write_model(tmp_path / "keyed.json", {"ding": ding()}, admin())
    assert "\nprep_le64(const uint8_t *bytes" in generated_source(
        keyed, tmp_path / "keyed"
    )


def test_a_header_is_guarded_and_declares_c_linkage(model, tmp_path):
    text = header(generate(model, tmp_path / "out"), "nvm").read_text(encoding="utf-8")
    assert (
        "\n#ifndef __LIBXNVME_CMD_PREP_NVM_H\n#define __LIBXNVME_CMD_PREP_NVM_H\n"
        in text
    )
    assert '\n#ifdef __cplusplus\nextern "C" {\n#endif\n' in text
    assert text.endswith(
        "\n#ifdef __cplusplus\n}\n#endif\n\n#endif /* __LIBXNVME_CMD_PREP_NVM_H */\n"
    )


def test_doxygen_reads_the_recparam_of_a_prep_as_its_param(model, tmp_path):
    """Doxygen runs each header through the filter of docs/tooling/doxy.cfg,
    which must make @param of yace's @recparam, a command Doxygen lacks"""

    doxy = (Path(__file__).parents[2] / "docs" / "tooling" / "doxy.cfg").read_text()
    (line,) = [x for x in doxy.splitlines() if x.startswith("FILTER_PATTERNS")]
    command = line.split('*.h="', 1)[1].rsplit('"', 1)[0]
    nvm = header(generate(model, tmp_path / "out"), "nvm")
    filtered = subprocess.run(
        f"{command} {nvm}", shell=True, check=True, capture_output=True, text=True
    ).stdout
    assert "@recparam" not in filtered
    assert " * @param args The fields of the command, see" in filtered
    # Doxygen reads a bare #ifdef in a doc as a link to a symbol named ifdef
    docs = re.findall(r"/\*\*.*?\*/", filtered, re.S)
    assert not [d for d in docs if re.search(r"(?<!`)#ifdef", d)]


def test_a_record_has_nsid_unless_the_opcode_table_says_it_is_not_used(tmp_path):
    def odd(name, opcode, flag):
        found = command(
            name, opcode, [dwords(10, 10, [bits("07:00", "Select", "SEL")])]
        )
        found.typed["namespace_identifier_used"] = spec_model.Typed(
            kind="flag", flag=flag
        )
        return found

    model = write_model(
        tmp_path / "m.json",
        {"used": odd("Used", 0x01, True), "unused": odd("Unused", 0x02, False)},
    )
    nvm = header(generate(model, tmp_path / "out"), "nvm")
    assert fields(nvm, "xnvme_cmd_prep_nvm_used_args") == [
        "uint32_t nsid;",
        "uint8_t sel;",
    ]
    assert fields(nvm, "xnvme_cmd_prep_nvm_unused_args") == ["uint8_t sel;"]


@pytest.mark.parametrize(
    "acronym, name", [("CNT", "cnt"), ("C", "count"), (None, "count")]
)
def test_a_value_of_parts_is_named_by_an_acronym_of_more_than_one_letter(
    tmp_path, acronym, name
):
    odd = command(
        "Odd",
        0x01,
        [
            dwords(
                12, 12, [bits("15:00", "Count Lower", "CNTL", ("Count", acronym, 0))]
            ),
            dwords(
                13, 13, [bits("15:00", "Count Upper", "CNTU", ("Count", acronym, 16))]
            ),
        ],
    )
    nvm = header(
        generate(write_model(tmp_path / "m.json", {"odd": odd}), tmp_path / "out"),
        "nvm",
    )
    assert fields(nvm, "xnvme_cmd_prep_nvm_odd_args") == [
        "uint32_t nsid;",
        f"uint32_t {name};",
    ]


def einval_of(text, function):
    """What the doc of 'function' says makes it return -EINVAL"""

    end = text.index(f"\n{function}_sized(")
    doc = text[text.rindex("/**", 0, end) : end].replace("\n * ", " ")
    return doc[
        doc.index("-EINVAL when ") + len("-EINVAL when ") : doc.index("; -E2BIG")
    ]


@pytest.mark.parametrize(
    "function, says",
    [
        (
            "xnvme_cmd_prep_adm_keep",
            "'sqe' or 'args' is NULL, 'args_nbytes' is zero or not a multiple of 8,"
            " or a reserved byte is not zero",
        ),
        (
            "xnvme_cmd_prep_adm_dong",
            "'sqe' or 'args' is NULL, 'args_nbytes' is zero or not a multiple of 8,"
            " or a reserved byte is not zero",
        ),
        (
            "xnvme_cmd_prep_adm_wide",
            "'sqe' or 'args' is NULL, or 'args_nbytes' is zero or not a multiple of 8",
        ),
    ],
)
def test_the_doc_of_a_prep_names_only_the_errors_of_its_record(
    tmp_path, function, says
):
    commands = admin()
    commands["wide"] = command(
        "Wide", 0x05, [dwords(10, 11, [bits("63:00", "Wide", "WIDE")])], False
    )
    model = write_model(tmp_path / "m.json", {}, commands)
    adm = header(generate(model, tmp_path / "out"), "adm").read_text(encoding="utf-8")
    assert einval_of(adm, function) == says


def test_a_committed_header_that_clang_format_aligned_is_read(model, tmp_path):
    """pre-commit formats the committed headers, which aligns the comments of
    the members"""

    if shutil.which("clang-format") is None:
        pytest.skip("no clang-format")
    record = "xnvme_cmd_prep_nvm_ding_args"
    first = header(generate(model, tmp_path / "first"), "nvm")
    committed = tmp_path / "committed.h"
    shutil.copy(first, committed)
    subprocess.run(
        [
            sys.executable,
            str(ROOT / "toolbox" / "pcf_clang_format.py"),
            "--style-file",
            str(ROOT / "toolbox" / "clang-format-h"),
            str(committed),
        ],
        capture_output=True,
    )
    text = committed.read_text(encoding="utf-8")
    assert "\tuint8_t frc;     " in text

    grown = write_model(
        tmp_path / "grown.json",
        {"ding": ding(extra=[bits("20", "Next Bit", "NXT")])},
        admin(),
    )
    nvm = header(generate(grown, tmp_path / "out", committed=committed), "nvm")
    assert fields(nvm, record) == fields(first, record) + ["uint8_t nxt;"]

    committed.write_text(
        text.replace("\tuint8_t frc; ", "\tuint16_t frc;"), encoding="utf-8"
    )
    with pytest.raises(ValueError, match="'uint16_t frc;' is now"):
        generate(model, tmp_path / "again", committed=committed)


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
            f"{SCRIPT}:CmdPrep",
            "--generator-option",
            f"xnvme_cmd_prep.spec={model}",
            "--generator-option",
            f"xnvme_cmd_prep.committed={tmp_path / 'none.h'}",
            "--output",
            str(tmp_path / "out"),
        ],
        capture_output=True,
        text=True,
    )
    assert proc.returncode == 0, proc.stderr
    (status,) = (tmp_path / "out").glob("*.status")
    assert ", 0 diagnostics," in status.read_text()


def test_two_commands_of_one_name_in_a_set_stop_the_generator(tmp_path):
    model = write_model(tmp_path / "m.json", {"ding": ding(), "ding_2": ding()})

    with pytest.raises(ValueError, match=re.escape("give xnvme_cmd_prep_nvm_ding()")):
        generate(model, tmp_path / "out")


def test_a_field_in_cdw1_is_a_member_of_a_record_without_nsid(tmp_path):
    sqe = [dwords(1, 1, [bits("15:00", "Group", "GRP")])]
    model = write_model(
        tmp_path / "m.json", {"odd": command("Odd", 0x01, sqe, nsid=False)}
    )

    source = generated_source(model, tmp_path)

    assert "prep_bits(sqe, 1, 0, 16, a.grp);" in source
    assert "a.nsid" not in source


def test_the_doc_names_the_function_that_passes_a_command_of_the_set(model, tmp_path):
    wrote = generate(model, tmp_path / "out")

    adm = header(wrote, "adm").read_text(encoding="utf-8")
    nvm = header(wrote, "nvm").read_text(encoding="utf-8")
    left = " * The command identifier, MPTR and DPTR are left zero, for"
    assert f"{left} xnvme_cmd_pass_admin() to set.\n" in adm
    assert "xnvme_cmd_pass()" not in adm
    assert f"{left} xnvme_cmd_pass() or xnvme_cmd_pass_iov() to set.\n" in nvm
