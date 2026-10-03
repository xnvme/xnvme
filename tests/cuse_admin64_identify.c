// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <linux/nvme_ioctl.h>

#include <libxnvme.h>

/**
 * Exercises NVME_IOCTL_ADMIN64_CMD against a CUSE ioctl mimic
 * (lib/xnvme_cuse.c) directly, bypassing xNVMe's own command path:
 * xnvme_be_linux_nvme_cmd_admin() always issues the 32-bit
 * NVME_IOCTL_ADMIN_CMD, so nothing reachable through xnvme_cmd_pass_admin()
 * ever exercises the 64-bit ioctl; this is the only way to.
 */
static int
sub_identify64(struct xnvme_cli *cli)
{
	struct nvme_passthru_cmd64 cmd = {0};
	unsigned char data[4096] = {0};
	uint16_t vid;
	int fd, err;

	fd = open(cli->args.uri, O_RDWR);
	if (fd < 0) {
		err = -errno;
		xnvme_cli_perr("open()", err);
		return err;
	}

	cmd.opcode = 0x06; // Identify
	cmd.cdw10 = 1;     // CNS: Identify Controller
	cmd.addr = (uint64_t)(uintptr_t)data;
	cmd.data_len = sizeof(data);

	err = ioctl(fd, NVME_IOCTL_ADMIN64_CMD, &cmd);
	if (err < 0) {
		err = -errno;
		xnvme_cli_perr("ioctl(NVME_IOCTL_ADMIN64_CMD)", err);
		close(fd);
		return err;
	}
	close(fd);
	if (err != 0) {
		xnvme_cli_pinf("FAILED: NVMe status(0x%x)", err);
		return -EIO;
	}

	memcpy(&vid, data, sizeof(vid));
	xnvme_cli_pinf("LGTM: Identify Controller via NVME_IOCTL_ADMIN64_CMD, VID(0x%04x)", vid);

	return 0;
}

static struct xnvme_cli_sub g_subs[] = {
	{
		"identify64",
		"Issue an Identify Controller via the raw NVME_IOCTL_ADMIN64_CMD",
		"Issue an Identify Controller via the raw NVME_IOCTL_ADMIN64_CMD",
		sub_identify64,
		{
			{XNVME_CLI_OPT_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_URI, XNVME_CLI_POSA},
		},
	},
};

static struct xnvme_cli g_cli = {
	.title = "Exercise NVME_IOCTL_ADMIN64_CMD directly",
	.descr_short = "Exercise NVME_IOCTL_ADMIN64_CMD directly",
	.subs = g_subs,
	.nsubs = sizeof g_subs / sizeof(*g_subs),
};

int
main(int argc, char **argv)
{
	return xnvme_cli_run(&g_cli, argc, argv, XNVME_CLI_INIT_NONE);
}
