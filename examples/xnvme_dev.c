/**
 * Usage Examples
 * --------------
 *
 * ## OS Managed Devices
 * # Linux
 * example_dev /dev/nvme0n1 1
 *
 * # FreeBSD
 * example_dev /dev/nvme0ns1 1
 *
 * # Windows
 * example_dev \\.\PhysicalDevice1 1
 *
 * # User-space driver
 * example_dev 0000:04:00.0 1
 *
 * # NVMe-over-Fabrics
 * example_dev 192.168.1.1:4472 1
 */
#include <libxnvme.h>
#include <libxnvme_pp.h>

int
main(int argc, char *argv[])
{
	struct xnvme_opts opts = xnvme_opts_default();
	struct xnvme_dev *dev;

	switch (argc) {
	case 3:
		opts.nsid = atoi(argv[2]);
		break;
	case 2:
		opts.nsid = 1;
		break;
	default:
		return 1;
	}

	dev = xnvme_dev_open(argv[1], &opts);
	xnvme_dev_pr(dev, XNVME_PR_DEF);
	xnvme_dev_close(dev);

	return 0;
}
