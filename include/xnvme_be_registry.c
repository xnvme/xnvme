static struct xnvme_be *xnvme_be_registry[] = {
	&xnvme_be_spdk,
	&xnvme_be_liou,
	&xnvme_be_laio,
	&xnvme_be_lioc,
	&xnvme_be_fioc,
	NULL
};

static int
xnvme_be_count = sizeof xnvme_be_registry / sizeof * xnvme_be_registry - 1;
