// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * I/O through a dma-buf that vfio-pci exports of another device's BAR
 *
 * The exporter is taken by its vfio cdev and its largest BAR exported; the dma-buf is registered
 * with xnvme_buf_register_dmabuf() at an address range the CPU cannot access, and the BAR's
 * content is read and written through the vfio region instead. Needs the exporter bound to
 * vfio-pci and a kernel whose iommufd maps the dma-bufs vfio-pci exports (Linux 6.19). It WRITES
 * the first bytes of the namespace.
 */
#include <limits.h>
#include <libxnvme.h>
#include <upcie/upcie.h>

#define NBYTES_MAX (1UL << 20)

static uint64_t
min_u64(uint64_t a, uint64_t b)
{
	return a < b ? a : b;
}

static uint64_t
max_u64(uint64_t a, uint64_t b)
{
	return a > b ? a : b;
}

struct exporter {
	struct iommufd iommufd;
	struct vfio_cdev cdev;
	uint32_t region;
	uint64_t region_offset; ///< Where the region is in the device fd
	uint64_t region_size;
};

static int
exporter_open(const char *bdf, struct exporter *exp)
{
	char path[PATH_MAX];
	unsigned int n;
	int err;

	for (n = 0; n < 4096; ++n) {
		snprintf(path, sizeof(path), "/sys/bus/pci/devices/%s/vfio-dev/vfio%u", bdf, n);
		if (!access(path, F_OK)) {
			break;
		}
	}
	if (n == 4096) {
		xnvme_cli_pinf("FAILED: %s has no vfio cdev; bind it to vfio-pci", bdf);
		return -ENODEV;
	}
	snprintf(path, sizeof(path), "/dev/vfio/devices/vfio%u", n);

	err = vfio_cdev_open(path, &exp->cdev);
	if (err) {
		xnvme_cli_perr("vfio_cdev_open()", err);
		return err;
	}
	err = iommufd_open(&exp->iommufd);
	if (err) {
		xnvme_cli_perr("iommufd_open()", err);
		return err;
	}
	err = iommufd_bind(&exp->iommufd, &exp->cdev);
	if (err) {
		xnvme_cli_perr("iommufd_bind()", err);
		return err;
	}

	exp->region_size = 0;
	for (uint32_t i = VFIO_PCI_BAR0_REGION_INDEX; i <= VFIO_PCI_BAR5_REGION_INDEX; ++i) {
		struct vfio_region_info info = {.index = i};

		if (vfio_device_get_region_info(exp->cdev.fd, &info)) {
			continue;
		}
		if ((info.flags & VFIO_REGION_INFO_FLAG_READ) &&
		    (info.flags & VFIO_REGION_INFO_FLAG_WRITE) && (info.size > exp->region_size)) {
			exp->region = i;
			exp->region_offset = info.offset;
			exp->region_size = info.size;
		}
	}
	if (exp->region_size < 4096) {
		xnvme_cli_pinf("FAILED: %s has no BAR of a page or more", bdf);
		return -ENODEV;
	}

	return 0;
}

static void
exporter_close(struct exporter *exp)
{
	if (exp->cdev.fd >= 0) {
		vfio_cdev_close(&exp->cdev);
	}
	if (exp->iommufd.fd >= 0) {
		iommufd_close(&exp->iommufd);
	}
}

static int
bar_rw(struct exporter *exp, int wr, void *buf, uint64_t nbytes)
{
	ssize_t n = wr ? pwrite(exp->cdev.fd, buf, nbytes, (off_t)exp->region_offset)
		       : pread(exp->cdev.fd, buf, nbytes, (off_t)exp->region_offset);

	if (n != (ssize_t)nbytes) {
		int err = n < 0 ? -errno : -EIO;

		xnvme_cli_perr(wr ? "pwrite(BAR)" : "pread(BAR)", err);
		return err;
	}

	return 0;
}

static int
nvm_rw(struct xnvme_dev *dev, int wr, char *buf, uint64_t nbytes)
{
	const struct xnvme_geo *geo = xnvme_dev_get_geo(dev);
	uint64_t step = geo->mdts_nbytes ? min_u64(geo->mdts_nbytes, 128 * 1024) : 4096;

	for (uint64_t off = 0; off < nbytes; off += step) {
		struct xnvme_cmd_ctx ctx = {0};
		uint64_t len = min_u64(step, nbytes - off);
		uint16_t nlb = (uint16_t)(len / geo->lba_nbytes - 1);
		uint64_t slba = off / geo->lba_nbytes;
		uint32_t nsid = xnvme_dev_get_nsid(dev);
		int err;

		xnvme_dev_get_cmd_ctx(dev, &ctx);
		if (wr) {
			struct xnvme_cmd_prep_nvm_write_args args = {
				.nsid = nsid, .slba = slba, .nlb = nlb};

			err = xnvme_cmd_prep_nvm_write(&ctx.cmd, &args);
		} else {
			struct xnvme_cmd_prep_nvm_read_args args = {
				.nsid = nsid, .slba = slba, .nlb = nlb};

			err = xnvme_cmd_prep_nvm_read(&ctx.cmd, &args);
		}
		if (!err) {
			err = xnvme_cmd_pass(&ctx, buf + off, len, NULL, 0);
		}
		if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
			xnvme_cli_pinf("FAILED: %s at offset %" PRIu64 "; err(%d)",
				       wr ? "write" : "read", off, err);
			xnvme_cmd_ctx_pr(&ctx, XNVME_PR_DEF);
			return err ? err : -EIO;
		}
	}

	return 0;
}

static int
mismatch(const char *what, const char *got, const char *want, uint64_t nbytes)
{
	for (uint64_t i = 0; i < nbytes; ++i) {
		if (got[i] != want[i]) {
			xnvme_cli_pinf("FAILED: %s differs first at byte %" PRIu64, what, i);
			return 1;
		}
	}

	return 0;
}

static int
test_rw(struct xnvme_cli *cli)
{
	struct xnvme_dev *dev = cli->args.dev;
	const struct xnvme_geo *geo = xnvme_dev_get_geo(dev);
	struct exporter exp = {.iommufd = {.fd = -1}, .cdev = {.fd = -1}};
	char *pattern = NULL, *check = NULL, *hbuf = NULL, *vaddr = MAP_FAILED;
	uint64_t nbytes;
	int fd = -1, registered = 0, err;

	if (!cli->args.alt_uri) {
		xnvme_cli_pinf("FAILED: give the exporter's BDF with --alt-uri");
		return -EINVAL;
	}
	err = exporter_open(cli->args.alt_uri, &exp);
	if (err) {
		goto exit;
	}
	nbytes = min_u64(exp.region_size, NBYTES_MAX);
	nbytes -= nbytes % max_u64(geo->lba_nbytes, 4096);
	xnvme_cli_pinf("exporter: %s, BAR%u of %" PRIu64 " bytes; using %" PRIu64,
		       cli->args.alt_uri, exp.region, exp.region_size, nbytes);

	fd = vfio_device_bar_export_dmabuf(exp.cdev.fd, exp.region, 0, nbytes);
	if (fd < 0) {
		err = fd;
		xnvme_cli_perr("vfio_device_bar_export_dmabuf()", err);
		goto exit;
	}

	// PROT_NONE, as the range of a caller whose device memory the CPU cannot access
	vaddr = mmap(NULL, nbytes, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if (vaddr == MAP_FAILED) {
		err = -errno;
		goto exit;
	}

	err = xnvme_buf_register_dmabuf(dev, fd, vaddr);
	if (err) {
		xnvme_cli_perr("xnvme_buf_register_dmabuf()", err);
		goto exit;
	}
	registered = 1;
	close(fd);
	fd = -1;

	pattern = malloc(nbytes);
	check = malloc(nbytes);
	hbuf = xnvme_buf_alloc(dev, nbytes);
	if (!pattern || !check || !hbuf) {
		err = -ENOMEM;
		goto exit;
	}

	for (uint64_t i = 0; i < nbytes; ++i) {
		pattern[i] = (char)((i * 131) ^ (i >> 9));
	}
	err = bar_rw(&exp, 1, pattern, nbytes);
	err = err ? err : bar_rw(&exp, 0, check, nbytes);
	if (err) {
		goto exit;
	}
	if (mismatch("the BAR, written and read by the CPU", check, pattern, nbytes)) {
		xnvme_cli_pinf("FAILED: the exporter's BAR is not memory; choose another device");
		err = -ENOTSUP;
		goto exit;
	}
	memcpy(hbuf, pattern, nbytes);
	memset(check, 0, nbytes); // so that a read which moves nothing fails
	err = bar_rw(&exp, 1, check, nbytes);
	err = err ? err : nvm_rw(dev, 1, hbuf, nbytes);
	err = err ? err : nvm_rw(dev, 0, vaddr, nbytes);
	err = err ? err : bar_rw(&exp, 0, check, nbytes);
	if (err) {
		goto exit;
	}
	if (mismatch("the BAR after reading the namespace into it", check, pattern, nbytes)) {
		err = -EIO;
		goto exit;
	}
	xnvme_cli_pinf("LGTM: read from the namespace into the dma-buf");

	for (uint64_t i = 0; i < nbytes; ++i) {
		pattern[i] = (char)~((i * 7) ^ (i >> 11));
	}
	memset(hbuf, 0, nbytes);
	err = bar_rw(&exp, 1, pattern, nbytes);
	err = err ? err : nvm_rw(dev, 1, hbuf, nbytes);
	err = err ? err : nvm_rw(dev, 1, vaddr, nbytes);
	err = err ? err : nvm_rw(dev, 0, hbuf, nbytes);
	if (err) {
		goto exit;
	}
	if (mismatch("the namespace after writing the dma-buf to it", hbuf, pattern, nbytes)) {
		err = -EIO;
		goto exit;
	}
	xnvme_cli_pinf("LGTM: wrote from the dma-buf onto the namespace");

	fd = vfio_device_bar_export_dmabuf(exp.cdev.fd, exp.region, 0, nbytes);
	if (fd < 0) {
		err = fd;
		goto exit;
	}
	err = xnvme_buf_register_dmabuf(dev, fd, vaddr);
	if (err) {
		xnvme_cli_perr("xnvme_buf_register_dmabuf(), again", err);
		goto exit;
	}
	err = xnvme_buf_unregister(dev, vaddr);
	err = err ? err : nvm_rw(dev, 0, vaddr, max_u64(geo->lba_nbytes, 4096));
	if (err) {
		xnvme_cli_pinf("FAILED: the dma-buf is unusable after one of two unregisters");
		goto exit;
	}
	err = xnvme_buf_unregister(dev, vaddr);
	registered = 0;
	if (err) {
		xnvme_cli_perr("xnvme_buf_unregister()", err);
		goto exit;
	}
	if (!xnvme_buf_unregister(dev, vaddr)) {
		xnvme_cli_pinf("FAILED: unregistered a dma-buf that was no longer registered");
		err = -EIO;
		goto exit;
	}

	if (!xnvme_buf_register_dmabuf(dev, fd, vaddr + 1)) {
		xnvme_cli_pinf("FAILED: registered a dma-buf at a misaligned address");
		xnvme_buf_unregister(dev, vaddr + 1);
		err = -EIO;
		goto exit;
	}
	xnvme_cli_pinf("LGTM: registrations are counted, and a misaligned one returns an error");
	err = 0;

exit:
	if (registered) {
		xnvme_buf_unregister(dev, vaddr);
	}
	if (fd >= 0) {
		close(fd);
	}
	if (vaddr != MAP_FAILED) {
		munmap(vaddr, nbytes);
	}
	xnvme_buf_free(dev, hbuf);
	free(check);
	free(pattern);
	exporter_close(&exp);

	return err;
}

/** Through uri, the BAR exported again is tried first: it registers where it is mapped at the same
 * addresses as the first export, and returns -EBUSY elsewhere */
static int
test_close(struct xnvme_cli *cli)
{
	struct xnvme_dev *dev = cli->args.dev, *dev2 = NULL;
	const struct xnvme_geo *geo = xnvme_dev_get_geo(dev);
	struct exporter exp = {.iommufd = {.fd = -1}, .cdev = {.fd = -1}};
	char *pattern = NULL, *check = NULL, *hbuf = NULL, *vaddr = MAP_FAILED;
	char *uris = NULL, *second = NULL;
	struct xnvme_opts opts;
	uint64_t nbytes = 0;
	int fd = -1, fd2, registered = 0, err;

	uris = cli->args.alt_uri ? strdup(cli->args.alt_uri) : NULL;
	second = uris ? strchr(uris, ',') : NULL;
	if (!second) {
		xnvme_cli_pinf("FAILED: give the exporter's BDF and a controller with --alt-uri");
		free(uris);
		return -EINVAL;
	}
	*second++ = '\0';

	err = exporter_open(uris, &exp);
	if (err) {
		goto exit;
	}
	nbytes = min_u64(exp.region_size, NBYTES_MAX);
	nbytes -= nbytes % max_u64(geo->lba_nbytes, 4096);

	fd = vfio_device_bar_export_dmabuf(exp.cdev.fd, exp.region, 0, nbytes);
	if (fd < 0) {
		err = fd;
		xnvme_cli_perr("vfio_device_bar_export_dmabuf()", err);
		goto exit;
	}
	vaddr = mmap(NULL, nbytes, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if (vaddr == MAP_FAILED) {
		err = -errno;
		goto exit;
	}

	xnvme_opts_set_defaults(&opts);
	err = xnvme_cli_to_opts(cli, &opts);
	err = err ? err : xnvme_dev_open_uri(second, &opts, &dev2);
	if (err) {
		xnvme_cli_perr("xnvme_dev_open_uri(--alt-uri)", err);
		goto exit;
	}
	err = xnvme_buf_register_dmabuf(dev2, fd, vaddr);
	if (err) {
		xnvme_cli_perr("xnvme_buf_register_dmabuf(--alt-uri)", err);
		goto exit;
	}
	fd2 = vfio_device_bar_export_dmabuf(exp.cdev.fd, exp.region, 0, nbytes);
	err = fd2 < 0 ? fd2 : xnvme_buf_register_dmabuf(dev, fd2, vaddr);
	if (fd2 >= 0) {
		close(fd2);
	}
	if (err == -EBUSY) {
		xnvme_cli_pinf(
			"the BAR exported again maps elsewhere; registering the first export");
		err = xnvme_buf_register_dmabuf(dev, fd, vaddr);
	}
	if (err) {
		xnvme_cli_perr("xnvme_buf_register_dmabuf()", err);
		xnvme_buf_unregister(dev2, vaddr);
		goto exit;
	}
	registered = 1;
	err = xnvme_buf_unregister(dev2, vaddr);
	xnvme_dev_close(dev2);
	dev2 = NULL;
	if (err) {
		xnvme_cli_perr("xnvme_buf_unregister(--alt-uri)", err);
		goto exit;
	}

	pattern = malloc(nbytes);
	check = malloc(nbytes);
	hbuf = xnvme_buf_alloc(dev, nbytes);
	if (!pattern || !check || !hbuf) {
		err = -ENOMEM;
		goto exit;
	}
	for (uint64_t i = 0; i < nbytes; ++i) {
		pattern[i] = (char)((i * 13) ^ (i >> 7));
	}
	memcpy(hbuf, pattern, nbytes);
	memset(check, 0, nbytes);
	err = bar_rw(&exp, 1, check, nbytes);
	err = err ? err : nvm_rw(dev, 1, hbuf, nbytes);
	err = err ? err : nvm_rw(dev, 0, vaddr, nbytes);
	err = err ? err : bar_rw(&exp, 0, check, nbytes);
	if (err) {
		goto exit;
	}
	if (mismatch("the BAR after reading the namespace into it", check, pattern, nbytes)) {
		err = -EIO;
		goto exit;
	}
	xnvme_cli_pinf("LGTM: read into the dma-buf after the controller registered first closed");

exit:
	if (registered) {
		xnvme_buf_unregister(dev, vaddr);
	}
	xnvme_dev_close(dev2);
	if (fd >= 0) {
		close(fd);
	}
	if (vaddr != MAP_FAILED) {
		munmap(vaddr, nbytes);
	}
	xnvme_buf_free(dev, hbuf);
	free(check);
	free(pattern);
	free(uris);
	exporter_close(&exp);

	return err;
}

//
// Command-Line Interface (CLI) definition
//
static struct xnvme_cli_sub g_subs[] = {
	{
		"rw",
		"Read into and write from a dma-buf of the BAR of the device at --alt-uri",
		"Read into and write from a dma-buf of the BAR of the device at --alt-uri",
		test_rw,
		{
			{XNVME_CLI_OPT_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_URI, XNVME_CLI_POSA},

			{XNVME_CLI_OPT_NON_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_ALT_URI, XNVME_CLI_LREQ},

			XNVME_CLI_SYNC_OPTS,
		},
	},
	{
		"close",
		"Read into a dma-buf after the other controller it was registered through closes",
		"Register a dma-buf of the BAR of the first device of --alt-uri through the "
		"second device of --alt-uri, then it or the BAR exported again through uri, "
		"close the second, and read into the dma-buf through uri",
		test_close,
		{
			{XNVME_CLI_OPT_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_URI, XNVME_CLI_POSA},

			{XNVME_CLI_OPT_NON_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_ALT_URI, XNVME_CLI_LREQ},

			XNVME_CLI_SYNC_OPTS,
		},
	},
};

static struct xnvme_cli g_cli = {
	.title = "Test I/O through a dma-buf vfio-pci exports",
	.descr_short = "Test I/O through a dma-buf vfio-pci exports",
	.subs = g_subs,
	.nsubs = sizeof g_subs / sizeof(*g_subs),
};

int
main(int argc, char **argv)
{
	return xnvme_cli_run(&g_cli, argc, argv, XNVME_CLI_INIT_DEV_OPEN);
}
