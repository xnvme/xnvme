#ifndef __LIBXNVME_LIBCONF_H
#define __LIBXNVME_LIBCONF_H
#include <libxnvme.h>
#include <libxnvme_pp.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Array of strings related to the configuration of xNVMe
 * These include:
 * Version-strings for libraries bundled with xNVMe
 * Enabled flags at compilation
 *
 * @see libxnvme_libconf.h
 */
extern const char *xnvme_libconf[];

/**
 * Prints the given array of version-strings to the given output stream
 *
 * @param stream output stream used for printing
 * @param opts printer options, see ::xnvme_pr
 *
 * @return On success, the number of characters printed is returned.
 */
int
xnvme_libconf_fpr(FILE *stream, enum xnvme_pr opts);

/**
 * Prints the given array of version-strings to stdout
 *
 * @param opts printer options, see ::xnvme_pr
 *
 * @return On success, the number of characters printed is returned.
 */
int
xnvme_libconf_pr(enum xnvme_pr opts);

#ifdef __cplusplus
}
#endif

#endif /* __LIBXNVME_LIBCONF_H */
