static struct xnvme_be *xnvme_be_registry[] = {
	NULL
};

static int
xnvme_be_count = sizeof xnvme_be_registry / sizeof * xnvme_be_registry - 1;
