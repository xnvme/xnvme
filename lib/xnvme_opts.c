#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <sys/stat.h>
#include <libxnvme.h>
#include <libxnvme_pp.h>

struct xnvme_opts
xnvme_opts_default(void)
{
	struct xnvme_opts opts = {0};

	opts.rdwr = 1;

	// Value is only applicable if the user also sets opts.create = 1
	opts.create_mode = S_IRUSR | S_IWUSR;

	return opts;
}
