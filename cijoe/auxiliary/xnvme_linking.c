#include <libxnvme.h>

int
enum_cb(struct xnvme_dev *dev, void *cb_args)
{
	const struct xnvme_ident *ident = xnvme_dev_get_ident(dev);

	xnvme_ident_pr(ident, XNVME_PR_DEF);

	return XNVME_ENUMERATE_DEV_CLOSE;
}

int
main(int argc, const char *argv[])
{
	struct xnvme_opts opts = xnvme_opts_default();
	int err;

	err = xnvme_enumerate(argc > 2 ? argv[2] : NULL, &opts, enum_cb, NULL);
	if (err) {
		printf("FAILED: enumerate(), err: %d", err);
		err = EXIT_FAILURE;
	}

	return 0;
}
