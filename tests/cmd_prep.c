// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <errno.h>
#include <inttypes.h>
#include <stddef.h>
#include <libxnvme.h>

#if !defined(XNVME_CMD_PREP_ADM) || !defined(XNVME_CMD_PREP_IO) ||      \
	!defined(XNVME_CMD_PREP_NVM) || !defined(XNVME_CMD_PREP_ZNS) || \
	!defined(XNVME_CMD_PREP_KV)
#error "a header of the command preps does not define its XNVME_CMD_PREP_<SET>"
#endif

// A record without padding from the compiler: its size is the end of its last member
XNVME_STATIC_ASSERT(sizeof(struct xnvme_cmd_prep_adm_identify_args) ==
			    offsetof(struct xnvme_cmd_prep_adm_identify_args, rsvd15) + 1,
		    "The record has padding")
XNVME_STATIC_ASSERT(sizeof(struct xnvme_cmd_prep_adm_identify_args) % 8 == 0,
		    "The record is not a multiple of 8 bytes")

#define CHECK(cond)                                                             \
	do {                                                                    \
		if (!(cond)) {                                                  \
			xnvme_cli_pinf("FAILED: line %d: %s", __LINE__, #cond); \
			err = -EINVAL;                                          \
		}                                                               \
	} while (0)

static int
sub_fields(struct xnvme_cli *XNVME_UNUSED(cli))
{
	union xnvme_spec_sqe sqe;
	int err = 0;

	struct xnvme_cmd_prep_nvm_read_args rd = {
		.args_nbytes = sizeof(rd),
		.nsid = 1,
		.slba = 0x1122334455667788ULL,
		.nlb = 7,
		.fua = 1,
		.dsm = 0x81,
	};
	CHECK(!xnvme_cmd_prep_nvm_read(&sqe, &rd));
	CHECK(sqe.cdw[0] == 0x02 && sqe.cdw[1] == 1);
	CHECK(sqe.cdw[10] == 0x55667788 && sqe.cdw[11] == 0x11223344);
	CHECK(sqe.cdw[12] == (7u | 1u << 30));
	CHECK(sqe.cdw[13] == 0x81);
	for (int i = 2; i < 10; ++i) {
		CHECK(sqe.cdw[i] == 0);
	}

	// NUMD and LPO are each split in two fields; an offset above 28 bits
	struct xnvme_cmd_prep_adm_get_log_page_args lp = {
		.args_nbytes = sizeof(lp),
		.nsid = 0xFFFFFFFF,
		.lid = 0x02,
		.numd = 0x12345,
		.lpo = 0x00000001F0000000ULL,
		.csi = 2,
		.lsi = 0xBEEF,
	};
	CHECK(!xnvme_cmd_prep_adm_get_log_page(&sqe, &lp));
	CHECK(sqe.cdw[0] == 0x02 && sqe.cdw[1] == 0xFFFFFFFF);
	CHECK(sqe.cdw[10] == (0x02u | 0x2345u << 16));
	CHECK(sqe.cdw[11] == (0x1u | 0xBEEFu << 16));
	CHECK(sqe.cdw[12] == 0xF0000000 && sqe.cdw[13] == 0x1);
	CHECK(sqe.cdw[14] == 2u << 24);

	// The key is in CDW2-3 and CDW14-15, its first byte in the lowest bits
	struct xnvme_cmd_prep_kv_store_args st = {
		.args_nbytes = sizeof(st),
		.nsid = 1,
		.kv_key = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16},
		.kl = 16,
		.vs = 4096,
	};
	CHECK(!xnvme_cmd_prep_kv_store(&sqe, &st));
	CHECK(sqe.cdw[0] == 0x01 && sqe.cdw[10] == 4096 && (sqe.cdw[11] & 0xFF) == 16);
	CHECK(sqe.cdw[2] == 0x04030201 && sqe.cdw[3] == 0x08070605);
	CHECK(sqe.cdw[14] == 0x0C0B0A09 && sqe.cdw[15] == 0x100F0E0D);

	struct xnvme_cmd_prep_adm_set_features_args sf = {
		.args_nbytes = sizeof(sf),
		.fid = XNVME_SPEC_FEAT_NQUEUES,
		.sv = 1,
		.cdw11 = 0x00030003,
	};
	CHECK(!xnvme_cmd_prep_adm_set_features(&sqe, &sf));
	CHECK(sqe.cdw[0] == 0x09 && sqe.cdw[10] == (0x07u | 1u << 31));
	CHECK(sqe.cdw[11] == 0x00030003);

	// A dword that the operation selected by SEL describes is a raw member
	struct xnvme_cmd_prep_adm_migration_send_args ms = {
		.args_nbytes = sizeof(ms),
		.sel = 0x2,
		.uidx = 3,
		.cdw11 = 0x01020007,
		.cdw12 = 0x1000,
		.cdw13 = 0x1,
		.cdw15 = 0x3FF,
	};
	CHECK(!xnvme_cmd_prep_adm_migration_send(&sqe, &ms));
	CHECK(sqe.cdw[0] == 0x41 && sqe.cdw[10] == 0x2 && sqe.cdw[14] == 3);
	CHECK(sqe.cdw[11] == 0x01020007 && sqe.cdw[12] == 0x1000);
	CHECK(sqe.cdw[13] == 0x1 && sqe.cdw[15] == 0x3FF);

	// Create Controller Data Queue: CQS and PC in CDW11, CDQSIZE in CDW12, no CDW13
	struct xnvme_cmd_prep_adm_controller_data_queue_args cdq = {
		.args_nbytes = sizeof(cdq),
		.sel = 0x0,
		.mos = 0x1,
		.cdw11 = 0x00020001,
		.cdw12 = 0x400,
	};
	CHECK(!xnvme_cmd_prep_adm_controller_data_queue(&sqe, &cdq));
	CHECK(sqe.cdw[0] == 0x45 && sqe.cdw[10] == 0x00010000);
	CHECK(sqe.cdw[11] == 0x00020001 && sqe.cdw[12] == 0x400 && sqe.cdw[13] == 0);

	struct xnvme_cmd_prep_adm_keep_alive_args ka = {.args_nbytes = sizeof(ka)};
	CHECK(!xnvme_cmd_prep_adm_keep_alive(&sqe, &ka) && sqe.cdw[0] == 0x18);

	return err;
}

static int
sub_conditions(struct xnvme_cli *XNVME_UNUSED(cli))
{
	struct xnvme_cmd_prep_nvm_read_args rd = {.args_nbytes = sizeof(rd), .nsid = 1};
	union xnvme_spec_sqe sqe;
	int err = 0;

	// CDW13 is DSM when CETYPE is zero, and CEV otherwise
	rd.cev = 5;
	CHECK(xnvme_cmd_prep_nvm_read(&sqe, &rd) == -EINVAL);
	rd.cetype = 1;
	CHECK(!xnvme_cmd_prep_nvm_read(&sqe, &rd));
	CHECK(sqe.cdw[13] == 5 && ((sqe.cdw[12] >> 16) & 0xF) == 1);
	rd.dsm = 1;
	CHECK(xnvme_cmd_prep_nvm_read(&sqe, &rd) == -EINVAL);

	return err;
}

static int
sub_ranges(struct xnvme_cli *XNVME_UNUSED(cli))
{
	union xnvme_spec_sqe sqe;
	int err = 0;

	struct xnvme_cmd_prep_nvm_read_args rd = {.args_nbytes = sizeof(rd), .prinfo = 0x10};
	CHECK(xnvme_cmd_prep_nvm_read(&sqe, &rd) == -EINVAL);

	struct xnvme_cmd_prep_adm_get_log_page_args lp = {.args_nbytes = sizeof(lp), .lsp = 0x80};
	CHECK(xnvme_cmd_prep_adm_get_log_page(&sqe, &lp) == -EINVAL);
	lp.lsp = 0x7F;
	CHECK(!xnvme_cmd_prep_adm_get_log_page(&sqe, &lp));

	return err;
}

static int
sub_sizes(struct xnvme_cli *XNVME_UNUSED(cli))
{
	struct xnvme_cmd_prep_adm_get_log_page_args lp = {
		.args_nbytes = sizeof(lp),
		.nsid = 0xFFFFFFFF,
		.lid = 0x02,
		.lpo = 0x1000,
	};
	struct {
		struct xnvme_cmd_prep_adm_get_log_page_args args;
		uint64_t more;
	} newer = {lp, 0};
	struct xnvme_cmd_prep_adm_get_log_page_args older = lp;
	struct xnvme_cmd_prep_adm_get_log_page_args nosize = lp;
	union xnvme_spec_sqe sqe;
	int err = 0;

	CHECK(xnvme_cmd_prep_adm_get_log_page(NULL, &lp) == -EINVAL);
	CHECK(xnvme_cmd_prep_adm_get_log_page(&sqe, NULL) == -EINVAL);

	nosize.args_nbytes = 0;
	CHECK(xnvme_cmd_prep_adm_get_log_page(&sqe, &nosize) == -EINVAL);

	// The record of an older caller: the fields it does not have are zero
	older.args_nbytes = offsetof(struct xnvme_cmd_prep_adm_get_log_page_args, rae);
	CHECK(!xnvme_cmd_prep_adm_get_log_page(&sqe, &older));
	CHECK(sqe.cdw[1] == 0xFFFFFFFF && sqe.cdw[10] == 0 && sqe.cdw[12] == 0);

	// The record of a newer caller: refused unless what is beyond is zero
	newer.args.args_nbytes = sizeof(newer);
	CHECK(!xnvme_cmd_prep_adm_get_log_page(&sqe, &newer.args));
	CHECK(sqe.cdw[12] == 0x1000);
	newer.more = 1;
	CHECK(xnvme_cmd_prep_adm_get_log_page(&sqe, &newer.args) == -E2BIG);

	// A field that a newer caller has where this record is reserved: refused, not dropped
	struct xnvme_cmd_prep_adm_identify_args id = {
		.args_nbytes = sizeof(id),
		.cns = XNVME_SPEC_IDFY_CTRLR,
	};
	CHECK(!xnvme_cmd_prep_adm_identify(&sqe, &id));
	id.rsvd15[0] = 0x7;
	CHECK(xnvme_cmd_prep_adm_identify(&sqe, &id) == -EINVAL);

	// The reserved bytes between two members as well
	struct xnvme_cmd_prep_nvm_write_args wr = {.args_nbytes = sizeof(wr), .nsid = 1};
	CHECK(!xnvme_cmd_prep_nvm_write(&sqe, &wr));
	wr.rsvd27[0] = 1;
	CHECK(xnvme_cmd_prep_nvm_write(&sqe, &wr) == -EINVAL);

	return err;
}

static struct xnvme_cli_sub g_subs[] = {
	{
		"fields",
		"Prepare commands and check each field's bits",
		"Prepare commands and check each field's bits",
		sub_fields,
		{
			{XNVME_CLI_OPT_NONE, XNVME_CLI_LOPT},
		},
	},
	{
		"conditions",
		"Check the fields of a dword described by a condition",
		"Check the fields of a dword described by a condition",
		sub_conditions,
		{
			{XNVME_CLI_OPT_NONE, XNVME_CLI_LOPT},
		},
	},
	{
		"ranges",
		"Check that a value wider than its field is refused",
		"Check that a value wider than its field is refused",
		sub_ranges,
		{
			{XNVME_CLI_OPT_NONE, XNVME_CLI_LOPT},
		},
	},
	{
		"sizes",
		"Check records of no, a smaller and a larger size",
		"Check records of no, a smaller and a larger size",
		sub_sizes,
		{
			{XNVME_CLI_OPT_NONE, XNVME_CLI_LOPT},
		},
	},
};

static struct xnvme_cli g_cli = {
	.title = "Exercise the command preps",
	.descr_short = "Exercise the command preps",
	.descr_long = "",
	.subs = g_subs,
	.nsubs = sizeof g_subs / sizeof(*g_subs),
};

int
main(int argc, char **argv)
{
	return xnvme_cli_run(&g_cli, argc, argv, XNVME_CLI_INIT_NONE);
}
