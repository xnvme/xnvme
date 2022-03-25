#include <stdio.h>
#include <libxnvme.h>
#include <libxnvme_pp.h>

int
main(int argc, char **argv)
{
	struct xnvme_opts opts = xnvme_opts_default();
	struct xnvme_dev *dev;

	dev = xnvme_dev_open("/dev/nvme0n1", &opts);
	if (!dev) {
		perror("xnvme_dev_open()");
		return 1;
	}

	xnvme_dev_pr(dev, XNVME_PR_DEF);
	xnvme_dev_close(dev);

	return 0;
}
