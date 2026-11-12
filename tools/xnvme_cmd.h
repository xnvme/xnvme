// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef __XNVME_CMD_H
#define __XNVME_CMD_H

enum cli_cmd_kind {
	CLI_CMD_ADMIN,
	CLI_CMD_IO,
	CLI_CMD_IOV,
};

int
cli_cmd(enum cli_cmd_kind kind, int argc, char **argv);

#endif
