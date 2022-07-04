#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <sys/stat.h>
#include <libxnvme.h>
#include <libxnvme_pp.h>
#include <xnvme_dev.h>

int
xnvme_ident_from_uri(const char *uri, struct xnvme_ident *ident)
{
	if (strlen(uri) >= XNVME_IDENT_URI_LEN) {
		XNVME_DEBUG("FAILED: strlen(uri) too long");
		return -EINVAL;
	}

	memset(ident, 0, sizeof(*ident));
	strncpy(ident->uri, uri, XNVME_IDENT_URI_LEN - 1);
	ident->dtype = XNVME_DEV_TYPE_UNKNOWN;
	ident->nsid = 0xFFFFFFFF;
	ident->csi = 0xFF;

	return 0;
}

int
xnvme_ident_yaml(FILE *stream, const struct xnvme_ident *ident, int indent, const char *sep,
		 int head)
{
	int wrtn = 0;

	if (head) {
		wrtn += fprintf(stream, "%*sxnvme_ident:", indent, "");
		indent += 2;
	}

	if (!ident) {
		wrtn += fprintf(stream, " ~");
		return wrtn;
	}

	if (head) {
		wrtn += fprintf(stream, "\n");
	}

	wrtn += fprintf(stream, "%*suri: '%s'%s", indent, "", ident->uri, sep);
	wrtn += fprintf(stream, "%*sdtype: 0x%x%s", indent, "", ident->dtype, sep);
	wrtn += fprintf(stream, "%*snsid: 0x%x%s", indent, "", ident->nsid, sep);
	wrtn += fprintf(stream, "%*scsi: 0x%x%s", indent, "", ident->csi, sep);

	wrtn += fprintf(stream, "%*ssubnqn: '%s'", indent, "", ident->subnqn);

	return wrtn;
}

int
xnvme_ident_fpr(FILE *stream, const struct xnvme_ident *ident, int opts)
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

	wrtn += xnvme_ident_yaml(stream, ident, 0, "\n", 1);
	wrtn += fprintf(stream, "\n");

	return wrtn;
}

int
xnvme_ident_pr(const struct xnvme_ident *ident, int opts)
{
	return xnvme_ident_fpr(stdout, ident, opts);
}
