// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <errno.h>
#include <inttypes.h>
#include <libxnvme.h>

struct bits_case {
	uint64_t val;
	uint64_t mask;
	uint64_t get;  ///< xnvme_bits_get(val, mask)
	uint64_t bits; ///< The bits given to xnvme_bits_set()
	uint64_t set;  ///< xnvme_bits_set(val, mask, bits)
};

static const struct bits_case g_cases[] = {
	{0x5a, 0x0f, 0xa, 0x3, 0x53},
	{0x5a, 0x60, 0x2, 0x1, 0x3a},
	{0x5a, 0x10, 0x1, 0x0, 0x4a},
	{0x00, 0x10, 0x0, 0x1, 0x10},
	{0x5a, 0x0f, 0xa, 0x1f, 0x5f},
	{0xc0000000, 0xc0000000, 0x3, 0x1, 0x40000000},
	{UINT64_MAX, 1ULL << 63, 0x1, 0x0, UINT64_MAX >> 1},
	{0x1234, UINT64_MAX, 0x1234, 0x42, 0x42},
};

static int
sub_get_set(struct xnvme_cli *XNVME_UNUSED(cli))
{
	int err = 0;

	for (size_t i = 0; i < sizeof(g_cases) / sizeof(*g_cases); ++i) {
		const struct bits_case *c = &g_cases[i];
		uint64_t get = xnvme_bits_get(c->val, c->mask);
		uint64_t set = xnvme_bits_set(c->val, c->mask, c->bits);

		if (get != c->get) {
			xnvme_cli_pinf("FAILED: case %zu: get: %#" PRIx64 " != %#" PRIx64, i, get,
				       c->get);
			err = -EINVAL;
		}
		if (set != c->set) {
			xnvme_cli_pinf("FAILED: case %zu: set: %#" PRIx64 " != %#" PRIx64, i, set,
				       c->set);
			err = -EINVAL;
		}
	}

	return err;
}

static struct xnvme_cli_sub g_subs[] = {
	{
		"get-set",
		"Get and set fields of bits by their mask",
		"Get and set fields of bits by their mask",
		sub_get_set,
		{
			{XNVME_CLI_OPT_NONE, XNVME_CLI_LOPT},
		},
	},
};

static struct xnvme_cli g_cli = {
	.title = "Exercise the helpers of fields of bits",
	.descr_short = "Exercise the helpers of fields of bits",
	.descr_long = "",
	.subs = g_subs,
	.nsubs = sizeof g_subs / sizeof(*g_subs),
};

int
main(int argc, char **argv)
{
	return xnvme_cli_run(&g_cli, argc, argv, XNVME_CLI_INIT_NONE);
}
