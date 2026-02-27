/**
 * SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * @headerfile libxnvme_scan.h
 */

/**
 * Signature of callback function used with 'xnvme_scan' invoked for each discovered device
 *
 * The callback function receives an xnvme_ident describing the device without opening it.
 * Return 0 to continue scanning, non-zero to stop.
 */
typedef int (*xnvme_scan_cb)(const struct xnvme_ident *ident, void *cb_args);

/**
 * Scan for devices without opening them
 *
 * @param sys_uri URI of the system to scan, when NULL, localhost/PCIe
 * @param opts Options for instrumenting the runtime during scan
 * @param cb_func Callback function to invoke for each yielded device identity
 * @param cb_args Arguments passed to the callback function
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned.
 */
int
xnvme_scan(const char *sys_uri, struct xnvme_opts *opts, xnvme_scan_cb cb_func, void *cb_args);
