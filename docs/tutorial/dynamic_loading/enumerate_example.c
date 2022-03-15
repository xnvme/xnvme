#include <dlfcn.h>
#include <libxnvme.h>
#include <libxnvme_ident.h>
#include <libxnvme_pp.h>
#include <stdio.h>
#include <stdlib.h>

struct lib {
	void *handle;

	struct xnvme_opts (*opts_default)(void);
	struct xnvme_ident *(*get_ident)(struct xnvme_dev *);
	int (*ident_pr)(const struct xnvme_ident *, int);
	int (*enumerate)(char *, struct xnvme_opts *, xnvme_enumerate_cb, void *);
};

int
enum_cb(struct xnvme_dev *dev, void *cb_args)
{
	struct lib *lib = cb_args;

	lib->ident_pr(lib->get_ident(dev), XNVME_PR_DEF);

	return XNVME_ENUMERATE_DEV_CLOSE;
}

int
main(int argc, char *argv[])
{
	struct lib lib = {0};
	struct xnvme_opts opts;
	int err;

	if (argc < 2) {
		printf("usage: %s <library_path> [sys_uri]\n", argv[0]);
		return EXIT_FAILURE;
	}

	lib.handle = dlopen(argv[1], RTLD_LAZY);
	if (!lib.handle) {
		fprintf(stderr, "%s\n", dlerror());
		return EXIT_FAILURE;
	}

	lib.enumerate = dlsym(lib.handle, "xnvme_enumerate");
	if (!lib.enumerate) {
		fprintf(stderr, "%s\n", dlerror());
		err = EXIT_FAILURE;
		goto exit;
	}

	lib.opts_default = dlsym(lib.handle, "xnvme_opts_default");
	if (!lib.opts_default) {
		fprintf(stderr, "%s\n", dlerror());
		err = EXIT_FAILURE;
		goto exit;
	}

	lib.ident_pr = dlsym(lib.handle, "xnvme_ident_pr");
	if (!lib.ident_pr) {
		fprintf(stderr, "%s\n", dlerror());
		err = EXIT_FAILURE;
		goto exit;
	}

	lib.get_ident = dlsym(lib.handle, "xnvme_dev_get_ident");
	if (!lib.get_ident) {
		fprintf(stderr, "%s\n", dlerror());
		err = EXIT_FAILURE;
		goto exit;
	}

	opts = lib.opts_default();

	err = lib.enumerate(argc > 2 ? argv[2] : NULL, &opts, enum_cb, &lib);
	if (err) {
		printf("FAILED: enumerate(), err: %d", err);
		err = EXIT_FAILURE;
	}

exit:
	dlclose(lib.handle);

	return err;
}
