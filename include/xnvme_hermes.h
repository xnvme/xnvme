/*******************************************************************************
 *
 * Hermes eBPF-based PCIe Accelerator
 * Copyright(c) 2020 Eideticom, Inc.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * The full GNU General Public License is included in this distribution in
 * the file called "LICENSE".
 *
 * Martin Oliveira <martin.oliveira@eideticom.com>
 *
 ******************************************************************************/

#ifdef XNVME_BE_HERMES_ENABLED
#ifndef HERMES_H
#define HERMES_H

#include <linux/types.h>

struct hermes_download_prog_ioctl_argp {
	__u32 len;
	__u64 prog;
	__u32 flags;
};

#define HERMES_DOWNLOAD_PROG_IOCTL    _IOW('H', 0x50, \
		struct hermes_download_prog_ioctl_argp)

#endif // HERMES_H
#endif // XNVME_BE_HERMES_ENABLED
