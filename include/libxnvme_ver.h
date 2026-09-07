/**
 * SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * @file libxnvme_ver.h
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
 * Produces the revision of the source tree the library was built from
 *
 * The value is what "git describe --tags --dirty=+ --always" reported at build
 * time, so a trailing '+' means the tree had uncommitted changes. A source
 * archive made with "meson dist" carries the revision it was cut from; a copy
 * of the sources with neither git metadata nor that stamp yields "unknown".
 *
 * @return A pointer to a string owned by the library; do not free it
 */
const char *
xnvme_ver_vcs(void);

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
