#ifndef __LIBXNVME_3P_H
#define __LIBXNVME_3P_H
#include <libxnvme.h>
#include <libxnvme_pp.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Array of version-strings for libraries bundled with xNVMe
 *
 * @see lixnvme_3p.h
 */
extern const char *xnvme_3p_ver[];

/**
 * Prints the given array of version-strings to the given output stream
 *
 * @param stream output stream used for printing
 * @param ver Pointer to the array of strings to print
 * @param opts printer options, see ::xnvme_pr
 *
 * @return On success, the number of characters printed is returned.
 */
int
xnvme_3p_ver_fpr(FILE *stream, const char *ver[], enum xnvme_pr opts);

/**
 * Prints the given array of version-strings to stdout
 *
 * @param ver Pointer to the array of strings to print
 * @param opts printer options, see ::xnvme_pr
 *
 * @return On success, the number of characters printed is returned.
 */
int
xnvme_3p_ver_pr(const char *ver[], enum xnvme_pr opts);

#ifdef __cplusplus
}
#endif

#endif /* __LIBXNVME_3P_H */
