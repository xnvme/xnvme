// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <errno.h>
#include <inttypes.h>
#include <stddef.h>
#include <string.h>
#include <libxnvme.h>

/**
 * The masks of each field of bits fit the field, which the structure must still
 * have: a field renamed or narrowed without its masks fails the build
 */
#define FITS(type, field, mask)                                                       \
	((uint64_t)(mask) / ((uint64_t)1 << (4 * XNVME_SIZEOF_MEMBER(type, field))) / \
		 ((uint64_t)1 << (4 * XNVME_SIZEOF_MEMBER(type, field))) ==           \
	 0)

XNVME_STATIC_ASSERT(FITS(struct xnvme_spec_fs_idfy_ctrlr, caps,
			 XNVME_SPEC_FS_IDFY_CTRLR_CAPS_DIRECT),
		    "caps of struct xnvme_spec_fs_idfy_ctrlr")
XNVME_STATIC_ASSERT(FITS(struct xnvme_spec_fs_idfy_ctrlr, properties,
			 XNVME_SPEC_FS_IDFY_CTRLR_PROPERTIES_PERMISSIONS_POSIX |
				 XNVME_SPEC_FS_IDFY_CTRLR_PROPERTIES_PERMISSIONS_ACL |
				 XNVME_SPEC_FS_IDFY_CTRLR_PROPERTIES_STAMP_CREATION |
				 XNVME_SPEC_FS_IDFY_CTRLR_PROPERTIES_STAMP_ACCESS |
				 XNVME_SPEC_FS_IDFY_CTRLR_PROPERTIES_STAMP_CHANGE |
				 XNVME_SPEC_FS_IDFY_CTRLR_PROPERTIES_STAMP_ARCHIVE |
				 XNVME_SPEC_FS_IDFY_CTRLR_PROPERTIES_HARDLINKS |
				 XNVME_SPEC_FS_IDFY_CTRLR_PROPERTIES_SYMLINKS |
				 XNVME_SPEC_FS_IDFY_CTRLR_PROPERTIES_CASE_SENSITIVE |
				 XNVME_SPEC_FS_IDFY_CTRLR_PROPERTIES_CASE_PRESERVING |
				 XNVME_SPEC_FS_IDFY_CTRLR_PROPERTIES_JOURNALING_BLOCK |
				 XNVME_SPEC_FS_IDFY_CTRLR_PROPERTIES_JOURNALING_META |
				 XNVME_SPEC_FS_IDFY_CTRLR_PROPERTIES_SNAPSHOTTING |
				 XNVME_SPEC_FS_IDFY_CTRLR_PROPERTIES_COMPRESSED |
				 XNVME_SPEC_FS_IDFY_CTRLR_PROPERTIES_ENCRYPTED),
		    "properties of struct xnvme_spec_fs_idfy_ctrlr")
XNVME_STATIC_ASSERT(offsetof(struct xnvme_spec_fs_idfy_ctrlr, caps) == 520,
		    "caps of struct xnvme_spec_fs_idfy_ctrlr")
XNVME_STATIC_ASSERT(offsetof(struct xnvme_spec_fs_idfy_ctrlr, limits) == 528,
		    "limits of struct xnvme_spec_fs_idfy_ctrlr")
XNVME_STATIC_ASSERT(offsetof(struct xnvme_spec_fs_idfy_ctrlr, properties) == 560,
		    "properties of struct xnvme_spec_fs_idfy_ctrlr")
XNVME_STATIC_ASSERT(offsetof(struct xnvme_spec_fs_idfy_ctrlr, iosizes) == 568,
		    "iosizes of struct xnvme_spec_fs_idfy_ctrlr")

struct offset_case {
	const char *name;
	size_t offset; ///< As the structure has it
	size_t spec;   ///< As the specification has it
};

#define OFFSET(type, field, spec) {#type "." #field, offsetof(type, field), spec}

static const struct offset_case g_offsets[] = {
	OFFSET(struct xnvme_spec_nvm_identify_namespace, nsfeat, 24),
	OFFSET(struct xnvme_spec_nvm_identify_namespace, flbas, 26),
	OFFSET(struct xnvme_spec_nvm_identify_namespace, dps, 29),
	OFFSET(struct xnvme_spec_nvm_identify_namespace, dlfeat, 33),
	OFFSET(struct xnvme_spec_nvm_identify_namespace, lbaf, 128),
	OFFSET(struct xnvme_spec_base_identify_controller, cmic, 76),
	OFFSET(struct xnvme_spec_base_identify_controller, ver, 80),
	OFFSET(struct xnvme_spec_base_identify_controller, oaes, 92),
	OFFSET(struct xnvme_spec_base_identify_controller, ctratt, 96),
	OFFSET(struct xnvme_spec_base_identify_controller, oacs, 256),
	OFFSET(struct xnvme_spec_base_identify_controller, lpa, 261),
	OFFSET(struct xnvme_spec_base_identify_controller, sanicap, 328),
	OFFSET(struct xnvme_spec_base_identify_controller, sqes, 512),
	OFFSET(struct xnvme_spec_base_identify_controller, oncs, 520),
	OFFSET(struct xnvme_spec_base_identify_controller, vwc, 525),
	OFFSET(struct xnvme_spec_base_identify_controller, sgls, 536),
	OFFSET(struct xnvme_spec_base_identify_controller, ioccsz, 1792),
	OFFSET(struct xnvme_spec_base_identify_controller, psd, 2048),
	OFFSET(struct xnvme_spec_base_power_state_descriptor, mxps_nops, 3),
	OFFSET(struct xnvme_spec_base_power_state_descriptor, apw_aps, 22),
};

static int
sub_idfy_offsets(struct xnvme_cli *XNVME_UNUSED(cli))
{
	int err = 0;

	for (size_t i = 0; i < sizeof(g_offsets) / sizeof(*g_offsets); ++i) {
		if (g_offsets[i].offset != g_offsets[i].spec) {
			xnvme_cli_pinf("FAILED: %s: offset %zu != %zu", g_offsets[i].name,
				       g_offsets[i].offset, g_offsets[i].spec);
			err = -EINVAL;
		}
	}

	return err;
}

static int
sub_idfy_masks(struct xnvme_cli *XNVME_UNUSED(cli))
{
	uint8_t raw[4096] = {0};
	struct xnvme_spec_nvm_identify_namespace ns;
	struct xnvme_spec_base_identify_controller ctrlr;
	int err = 0;

	raw[26] = 0x5a; // Identify Namespace FLBAS: format 0xa, extended, format MSB 0x2
	memcpy(&ns, raw, sizeof(ns));
	if (xnvme_bits_get(ns.flbas, XNVME_SPEC_NVM_IDENTIFY_NAMESPACE_FLBAS_FIDXL) != 0xa ||
	    !xnvme_bits_get(ns.flbas, XNVME_SPEC_NVM_IDENTIFY_NAMESPACE_FLBAS_MTELBA) ||
	    xnvme_bits_get(ns.flbas, XNVME_SPEC_NVM_IDENTIFY_NAMESPACE_FLBAS_FIDXU) != 0x2) {
		xnvme_cli_pinf("FAILED: flbas: %#" PRIx8, ns.flbas);
		err = -EINVAL;
	}

	memset(raw, 0, sizeof(raw));
	raw[521] = 0x01; // Identify Controller ONCS bit 8: copy
	raw[331] = 0xc0; // Identify Controller SANICAP bits 31:30: NODMMAS
	memcpy(&ctrlr, raw, sizeof(ctrlr));
	if (!xnvme_bits_get(ctrlr.oncs, XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_ONCS_NVMCPYS) ||
	    xnvme_bits_get(ctrlr.oncs, XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_ONCS_NVMCMPS)) {
		xnvme_cli_pinf("FAILED: oncs: %#" PRIx16, ctrlr.oncs);
		err = -EINVAL;
	}
	if (xnvme_bits_get(ctrlr.sanicap, XNVME_SPEC_BASE_IDENTIFY_CONTROLLER_SANICAP_NODMMAS) !=
	    0x3) {
		xnvme_cli_pinf("FAILED: sanicap: %#" PRIx32, ctrlr.sanicap);
		err = -EINVAL;
	}

	return err;
}

static int
sub_status_masks(struct xnvme_cli *XNVME_UNUSED(cli))
{
	uint8_t raw[sizeof(struct xnvme_spec_base_completion_queue_entry)] = {0};
	struct xnvme_spec_base_completion_queue_entry cpl;
	int err = 0;

	raw[14] = 0x05; // Completion Queue Entry DW3 bits 31:16: phase tag, status code 0x2,
	raw[15] = 0x82; // status code type 0x1, do not retry
	memcpy(&cpl, raw, sizeof(cpl));

	if (!xnvme_bits_get(cpl.status, XNVME_SPEC_BASE_COMPLETION_QUEUE_ENTRY_STATUS_P) ||
	    xnvme_bits_get(cpl.status, XNVME_SPEC_BASE_COMPLETION_QUEUE_ENTRY_STATUS_SC) != 0x2 ||
	    xnvme_bits_get(cpl.status, XNVME_SPEC_BASE_COMPLETION_QUEUE_ENTRY_STATUS_SCT) != 0x1 ||
	    xnvme_bits_get(cpl.status, XNVME_SPEC_BASE_COMPLETION_QUEUE_ENTRY_STATUS_CRD) ||
	    xnvme_bits_get(cpl.status, XNVME_SPEC_BASE_COMPLETION_QUEUE_ENTRY_STATUS_M) ||
	    !xnvme_bits_get(cpl.status, XNVME_SPEC_BASE_COMPLETION_QUEUE_ENTRY_STATUS_DNR)) {
		xnvme_cli_pinf("FAILED: status: %#" PRIx16, cpl.status);
		err = -EINVAL;
	}

	return err;
}

static int
sub_sqe_dwords(struct xnvme_cli *XNVME_UNUSED(cli))
{
	union xnvme_cmd_sqe sqe = {0};
	int err = 0;

	sqe.cdw[0] = 0x4102;     // Dword 0: opcode 0x02, FUSE 0x1, PSDT 0x1
	sqe.cdw[1] = 0x12345678; // Dword 1: namespace identifier
	sqe.cdw[6] = 0x1;        // Dwords 6 and 7: PRP entry 1
	sqe.cdw[10] = 0xa;       // Dword 10

	if (xnvme_bits_get(sqe.common.cdw0, XNVME_SPEC_BASE_COMMON_COMMAND_FORMAT_CDW0_OPC) !=
		    0x02 ||
	    xnvme_bits_get(sqe.common.cdw0, XNVME_SPEC_BASE_COMMON_COMMAND_FORMAT_CDW0_FUSE) !=
		    0x1 ||
	    xnvme_bits_get(sqe.common.cdw0, XNVME_SPEC_BASE_COMMON_COMMAND_FORMAT_CDW0_PSDT) !=
		    0x1 ||
	    sqe.common.nsid != 0x12345678 || sqe.common.dptr.prp1_prp2.prp1 != 0x1 ||
	    sqe.common.cdw10 != 0xa) {
		xnvme_cli_pinf("FAILED: cdw0: %#" PRIx32 ", nsid: %#" PRIx32 ", cdw10: %#" PRIx32,
			       sqe.common.cdw0, sqe.common.nsid, sqe.common.cdw10);
		err = -EINVAL;
	}

	return err;
}

static int
sub_fs_masks(struct xnvme_cli *XNVME_UNUSED(cli))
{
	uint8_t raw[sizeof(struct xnvme_spec_fs_idfy_ctrlr)] = {0};
	struct xnvme_spec_fs_idfy_ctrlr ctrlr;
	int err = 0;

	raw[520] = 0x01; // File-system Identify Controller caps: direct
	raw[561] = 0x40; // properties bit 14: encrypted
	memcpy(&ctrlr, raw, sizeof(ctrlr));
	if (!xnvme_bits_get(ctrlr.caps, XNVME_SPEC_FS_IDFY_CTRLR_CAPS_DIRECT) ||
	    xnvme_bits_get(ctrlr.properties,
			   XNVME_SPEC_FS_IDFY_CTRLR_PROPERTIES_PERMISSIONS_POSIX) ||
	    !xnvme_bits_get(ctrlr.properties, XNVME_SPEC_FS_IDFY_CTRLR_PROPERTIES_ENCRYPTED)) {
		xnvme_cli_pinf("FAILED: caps: %#" PRIx64 ", properties: %#" PRIx64, ctrlr.caps,
			       ctrlr.properties);
		err = -EINVAL;
	}

	return err;
}

static struct xnvme_cli_sub g_subs[] = {
	{
		"idfy-offsets",
		"Check the offsets of the identify fields of bits",
		"Check the offsets of the identify fields of bits",
		sub_idfy_offsets,
		{
			{XNVME_CLI_OPT_NONE, XNVME_CLI_LOPT},
		},
	},
	{
		"idfy-masks",
		"Read identify fields of bits through their masks",
		"Read identify fields of bits through their masks",
		sub_idfy_masks,
		{
			{XNVME_CLI_OPT_NONE, XNVME_CLI_LOPT},
		},
	},
	{
		"sqe-dwords",
		"Read the submission queue entry through its dwords and the Common Command Format",
		"Read the submission queue entry through its dwords and the Common Command Format",
		sub_sqe_dwords,
		{
			{XNVME_CLI_OPT_NONE, XNVME_CLI_LOPT},
		},
	},
	{
		"status-masks",
		"Read the completion status through its masks",
		"Read the completion status through its masks",
		sub_status_masks,
		{
			{XNVME_CLI_OPT_NONE, XNVME_CLI_LOPT},
		},
	},
	{
		"fs-masks",
		"Read the file-system Identify Controller through its masks",
		"Read the file-system Identify Controller through its masks",
		sub_fs_masks,
		{
			{XNVME_CLI_OPT_NONE, XNVME_CLI_LOPT},
		},
	},
};

static struct xnvme_cli g_cli = {
	.title = "Exercise the layout of the specification structures",
	.descr_short = "Exercise the layout of the specification structures",
	.descr_long = "",
	.subs = g_subs,
	.nsubs = sizeof g_subs / sizeof(*g_subs),
};

int
main(int argc, char **argv)
{
	return xnvme_cli_run(&g_cli, argc, argv, XNVME_CLI_INIT_NONE);
}
