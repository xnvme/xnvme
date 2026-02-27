/**
 * SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * @headerfile libxnvme_ident.h
 */

#define XNVME_IDENT_URI_LEN 384

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

	char kernel_driver[32];
	uint8_t rsvd[23];
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_ident) == 704, "Incorrect size")

/**
 * Writes a YAML-representation of the given 'ident' to stream
 */
int
xnvme_ident_yaml(FILE *stream, const struct xnvme_ident *ident, int indent, const char *sep,
		 int head);

/**
 * Prints the given ::xnvme_ident to the given output stream
 *
 * @param stream output stream used for printing
 * @param ident pointer to structure to print
 * @param opts printer options, see ::xnvme_pr
 *
 * @return On success, the number of characters printed is returned.
 */
int
xnvme_ident_fpr(FILE *stream, const struct xnvme_ident *ident, int opts);

/**
 * Prints the given ::xnvme_ident to stdout
 *
 * @param ident pointer to structure to print
 * @param opts printer options, see ::xnvme_pr
 *
 * @return On success, the number of characters printed is returned.
 */
int
xnvme_ident_pr(const struct xnvme_ident *ident, int opts);

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
