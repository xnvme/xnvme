// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <libxnvme.h>
#include <libxnvme_libconf.h>
#include <xnvme_be_linux.h>
#include <xnvme_be_windows.h>
#ifdef XNVME_PLATFORM_LINUX_ENABLED
#include <linux/nvme_ioctl.h>
#endif
#include "xnvme_libconf_entries.c"

struct libconf_part {
	const char *name;
	const char *macro;
	bool built;
	const char *brings;
	const char *without;
};

static const struct libconf_part g_libconf_parts[] = {
	{
		.name = "posix",
		.macro = "XNVME_BE_POSIX_ENABLED",
#ifdef XNVME_BE_POSIX_ENABLED
		.built = true,
#endif
		.brings =
			"posix aio, psync and the memory of the Linux, FreeBSD and macOS configs",
		.without = "the build refuses it on Linux, FreeBSD and macOS",
	},
	{
		.name = "emu",
		.macro = "XNVME_BE_EMU_ENABLED",
#ifdef XNVME_BE_EMU_ENABLED
		.built = true,
#endif
		.brings = "async emulated on sync: the emu, emu_bdev, emu_file and *_emu configs",
		.without = "they report -ENOSYS",
	},
	{
		.name = "thrpool",
		.macro = "XNVME_BE_THRPOOL_ENABLED",
#ifdef XNVME_BE_THRPOOL_ENABLED
		.built = true,
#endif
		.brings = "async on a thread pool: the thrpool, thrpool_bdev, thrpool_file and "
			  "*_thrpool configs",
		.without = "they report -ENOSYS",
	},
	{
		.name = "nil",
		.macro = "XNVME_BE_NIL_ENABLED",
#ifdef XNVME_BE_NIL_ENABLED
		.built = true,
#endif
		.brings = "async without I/O: the nil and ramdisk configs",
		.without = "they report -ENOSYS",
	},
	{
		.name = "shim",
		.macro = "XNVME_BE_SHIM_ENABLED",
#ifdef XNVME_BE_SHIM_ENABLED
		.built = true,
#endif
		.brings = "admin for regular files: the *_file configs of Linux and FreeBSD",
		.without = "they report -ENOSYS",
	},
	{
		.name = "ramdisk",
		.macro = "XNVME_BE_RAMDISK_ENABLED",
#ifdef XNVME_BE_RAMDISK_ENABLED
		.built = true,
#endif
		.brings = "a device in memory: the ramdisk* configs",
		.without = "left out",
	},
	{
		.name = "liburing",
		.macro = "XNVME_BE_LIBURING_ENABLED",
#ifdef XNVME_BE_LIBURING_ENABLED
		.built = true,
#endif
		.brings = "the io_uring, io_uring_cmd, io_uring_bdev and io_uring_file configs",
		.without = "left out",
	},
	{
		.name = "libaio",
		.macro = "XNVME_BE_LIBAIO_ENABLED",
#ifdef XNVME_BE_LIBAIO_ENABLED
		.built = true,
#endif
		.brings = "the libaio, libaio_bdev and libaio_file configs",
		.without = "left out",
	},
	{
		.name = "iocp",
		.macro = "XNVME_BE_IOCP_ENABLED",
#ifdef XNVME_BE_IOCP_ENABLED
		.built = true,
#endif
		.brings = "the iocp, iocp_th and iocp_file configs",
		.without = "left out",
	},
	{
		.name = "ioring",
		.macro = "XNVME_BE_IORING_ENABLED",
#ifdef XNVME_BE_IORING_ENABLED
		.built = true,
#endif
		.brings = "the ioring config",
		.without = "left out",
	},
	{
		.name = "spdk",
		.macro = "XNVME_BE_SPDK_ENABLED",
#ifdef XNVME_BE_SPDK_ENABLED
		.built = true,
#endif
		.brings = "the spdk config",
		.without = "left out",
	},
	{
		.name = "spdk-rdma",
		.macro = "XNVME_BE_SPDK_TRANSPORT_RDMA_ENABLED",
#ifdef XNVME_BE_SPDK_TRANSPORT_RDMA_ENABLED
		.built = true,
#endif
		.brings = "NVMe over RDMA in the spdk config",
		.without = "the spdk config refuses RDMA",
	},
	{
		.name = "libvfn",
		.macro = "XNVME_BE_LIBVFN_ENABLED",
#ifdef XNVME_BE_LIBVFN_ENABLED
		.built = true,
#endif
		.brings = "the libvfn config",
		.without = "left out",
	},
	{
		.name = "upcie",
		.macro = "XNVME_BE_UPCIE_ENABLED",
#ifdef XNVME_BE_UPCIE_ENABLED
		.built = true,
#endif
		.brings = "the upcie, upcie-cuda and upcie-hip configs and homi status",
		.without = "left out, and homi status refuses",
	},
	{
		.name = "driverkit",
		.macro = "XNVME_BE_DRIVERKIT_ENABLED",
#ifdef XNVME_BE_DRIVERKIT_ENABLED
		.built = true,
#endif
		.brings = "the driverkit and driverkit_emu configs",
		.without = "left out",
	},
	{
		.name = "cuda",
		.macro = "XNVME_CUDA_ENABLED",
#ifdef XNVME_CUDA_ENABLED
		.built = true,
#endif
		.brings = "CUDA device memory, for upcie-cuda and io_uring_dmabuf",
		.without = "CUDA buffers report -ENOTSUP",
	},
	{
		.name = "hip",
		.macro = "XNVME_HIP_ENABLED",
#ifdef XNVME_HIP_ENABLED
		.built = true,
#endif
		.brings = "HIP device memory, for upcie-hip and io_uring_dmabuf",
		.without = "HIP buffers report -ENOTSUP",
	},
	{
		.name = "cuse",
		.macro = "XNVME_CUSE_ENABLED",
#ifdef XNVME_CUSE_ENABLED
		.built = true,
#endif
		.brings = "the character devices homi serve exposes",
		.without = "homi serve exposes none",
	},
	{
		.name = "isal",
		.macro = "XNVME_LIBISAL_ENABLED",
#ifdef XNVME_LIBISAL_ENABLED
		.built = true,
#endif
		.brings = "CRC by ISA-L",
		.without = "CRC by a built-in table",
	},
};

const struct xnvme_libconf *
xnvme_libconf_get(void)
{
	return &g_libconf;
}

int
xnvme_libconf_fpr(FILE *stream, const struct xnvme_libconf *libconf, enum xnvme_pr opts)
{
	int wrtn = 0;

	switch (opts) {
	case XNVME_PR_TERSE:
		wrtn += fprintf(stream, "# ENOSYS: opts(%x)", opts);
		return wrtn;

	case XNVME_PR_DEF:
	case XNVME_PR_YAML:
		break;
	}

	wrtn += fprintf(stream, "xnvme_libconf:");

	wrtn += fprintf(stream, "\n");
	wrtn += fprintf(stream, "  entries:\n");
	for (int i = 0; libconf->entries[i]; ++i) {
		fprintf(stream, "  - '%s'\n", libconf->entries[i]);
	}

#ifdef XNVME_PLATFORM_WINDOWS_ENABLED
	wrtn += fprintf(stream, "  - '3p: ");
	wrtn += xnvme_be_windows_uapi_ver_fpr(stream, XNVME_PR_DEF);
	wrtn += fprintf(stream, "'\n");
#endif
#ifdef XNVME_PLATFORM_LINUX_ENABLED
	wrtn += fprintf(stream, "  - '3p: ");
	wrtn += xnvme_be_linux_uapi_ver_fpr(stream, XNVME_PR_DEF);
	wrtn += fprintf(stream, "'\n");
#endif

	wrtn += fprintf(stream, "  with:\n");
	for (size_t i = 0; i < sizeof(g_libconf_parts) / sizeof(*g_libconf_parts); ++i) {
		const struct libconf_part *part = &g_libconf_parts[i];

		if (part->built) {
			wrtn += fprintf(
				stream,
				"  - {name: '%s', macro: '%s', built: yes, brings: '%s'}\n",
				part->name, part->macro, part->brings);
		} else {
			wrtn += fprintf(
				stream,
				"  - {name: '%s', macro: '%s', built: no, without: '%s; %s'}\n",
				part->name, part->macro, part->brings, part->without);
		}
	}

	return wrtn;
}

int
xnvme_libconf_pr(const struct xnvme_libconf *libconf, enum xnvme_pr opts)
{
	return xnvme_libconf_fpr(stdout, libconf, opts);
}
