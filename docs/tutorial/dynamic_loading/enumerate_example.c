#include <dlfcn.h>
#include <libxnvme.h>
#include <libxnvme_ident.h>
#include <libxnvme_pp.h>
#include <stdio.h>
#include <stdlib.h>

int (*ident_pr)(const struct xnvme_ident *, int);
struct xnvme_ident *(*get_ident)(struct xnvme_dev *);

int
enum_cb(struct xnvme_dev *dev, void *XNVME_UNUSED(cb_args))
{
	ident_pr(get_ident(dev), XNVME_PR_DEF);

	return XNVME_ENUMERATE_DEV_CLOSE;
}

int
main(int argc, char *argv[])
{
	struct xnvme_opts (*opts_default)(void);
	struct xnvme_opts opts = opts_default();
	void *handle;
	int (*enumerate)(char *, struct xnvme_opts *, xnvme_enumerate_cb, void *);

	handle = dlopen("/usr/local/lib/libxnvme-shared.so", RTLD_LAZY);
	if (!handle) {
		fprintf(stderr, "%s\n", dlerror());
		exit(EXIT_FAILURE);
	}

	enumerate = (int (*)(char *, struct xnvme_opts *, xnvme_enumerate_cb, void *))dlsym(
		handle, "xnvme_enumerate");
	if (!enumerate) {
		fprintf(stderr, "%s\n", dlerror());
		exit(EXIT_FAILURE);
	}

	opts_default = (struct xnvme_opts(*)(void))dlsym(handle, "xnvme_opts_default");
	if (!opts_default) {
		fprintf(stderr, "%s\n", dlerror());
		exit(EXIT_FAILURE);
	}

	ident_pr = (int (*)(const struct xnvme_ident *, int))dlsym(handle, "xnvme_ident_pr");
	if (!ident_pr) {
		fprintf(stderr, "%s\n", dlerror());
		exit(EXIT_FAILURE);
	}

	get_ident = (struct xnvme_ident * (*)(struct xnvme_dev *))
		dlsym(handle, "xnvme_dev_get_ident");
	if (!get_ident) {
		fprintf(stderr, "%s\n", dlerror());
		exit(EXIT_FAILURE);
	}

	enumerate(argc > 1 ? argv[1] : NULL, &opts, enum_cb, NULL);

	dlclose(handle);
	return EXIT_SUCCESS;
}
