/**
 * SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * @headerfile libxnvme_ident.h
 */

#define XNVME_IDENT_URI_LEN 384

#define XNVME_IDENT_SCHM_LEN 5
#define XNVME_IDENT_TRGT_LEN 155
#define XNVME_IDENT_OPTS_LEN 160
#define XNVME_IDENT_OPTS_SEP '?'
#define XNVME_IDENT_OPT_MAX  20

#define XNVME_IDENT_URI_LEN_MIN (XNVME_IDENT_SCHM_LEN + 1)

/**
 * Representation of device identifiers once decoded from text-representation
 *
 * @struct xnvme_ident
 */
struct xnvme_ident {
	char subnqn[256];

	char uri[XNVME_IDENT_URI_LEN];
	uint32_t dtype;

	uint32_t nsid;
	uint8_t csi;

	uint8_t rsvd[55];
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_ident) == 704, "Incorrect size")

/**
 * Parse the given 'uri' into ::xnvme_ident
 *
 * @param uri
 * @param ident Pointer to ident to fill with values parsed from 'uri'
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned.
 */
int
xnvme_ident_from_uri(const char *uri, struct xnvme_ident *ident);
