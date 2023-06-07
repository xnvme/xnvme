/**
 * SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * @headerfile libxnvme_ver.h
 */

/**
 * Produces the "major" version of the library
 *
 * @return On success, the "major" version of the library is returned
 */
int
xnvme_ver_major(void);

/**
 * Produces the "minor" version of the library
 *
 * @return On success, the "minor" version of the library is returned
 */
int
xnvme_ver_minor(void);

/**
 * Produces the "patch" version of the library
 *
 * @return On success, the "patch" version of the library is returned
 */
int
xnvme_ver_patch(void);

/**
 * Prints the library version to the given 'stream'
 *
 * @param stream output stream used for printing
 * @param opts printer options, see ::xnvme_pr
 *
 * @return On success, the number of characters printed is returned.
 */
int
xnvme_ver_fpr(FILE *stream, int opts);

/**
 * Prints the library version to stdout
 *
 * @param opts printer options, see ::xnvme_pr
 *
 * @return On success, the number of characters printed is returned.
 */
int
xnvme_ver_pr(int opts);
