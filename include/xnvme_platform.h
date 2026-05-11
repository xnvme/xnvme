// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef __INTERNAL_XNVME_PLATFORM_H
#define __INTERNAL_XNVME_PLATFORM_H

#include <stdint.h>

struct xnvme_be_config;
struct xnvme_dev;
struct xnvme_opts;
struct xnvme_ident;

/**
 * Callback function for xnvme_scan()
 *
 * @param ident Device identifier with URI and backend binding
 * @param cb_args User-provided callback arguments
 *
 * @return Return 0 to continue scanning, non-zero to stop.
 */
typedef int (*xnvme_scan_cb)(const struct xnvme_ident *ident, void *cb_args);

/**
 * Callback function for xnvme_enumerate()
 *
 * @param dev Opened device handle (caller must close with xnvme_dev_close())
 * @param cb_args User-provided callback arguments
 *
 * @return XNVME_ENUMERATE_DEV_KEEP_OPEN to retain the device handle, or
 *         XNVME_ENUMERATE_DEV_CLOSE to close it after the callback returns.
 */
typedef int (*xnvme_enumerate_cb)(struct xnvme_dev *dev, void *cb_args);

/**
 * Platform abstraction for OS-specific device operations
 *
 * Each supported OS defines one xnvme_platform instance with function pointers
 * for device lifecycle operations. The global g_xnvme_platform pointer is set
 * at compile-time based on the target OS.
 */
struct xnvme_platform {
	const char *name; /**< Platform name (e.g., "linux", "freebsd") */
	const struct xnvme_be_config *const
		*backends; /**< NULL-terminated flat array of config pointers */

	uint32_t (*classify)(const char *uri); /**< Classify URI into xnvme_be_cap */

	int (*dev_open)(struct xnvme_dev *dev, struct xnvme_opts *opts);
	int (*scan)(const char *sys_uri, struct xnvme_opts *opts, xnvme_scan_cb cb_func,
		    void *cb_args);
	int (*enumerate)(const char *sys_uri, struct xnvme_opts *opts, xnvme_enumerate_cb cb_func,
			 void *cb_args);
};

/**
 * Shared dev_open implementation - iterates platform->backends configs, opens device
 *
 * All platforms use this unless they need custom backend selection logic.
 *
 * @param dev Device handle with ident.uri populated
 * @param opts Options for backend selection (may be NULL for defaults)
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned.
 */
int
xnvme_platform_dev_open(struct xnvme_dev *dev, struct xnvme_opts *opts);

/**
 * Shared enumerate implementation using scan + dev_open
 *
 * When sys_uri is NULL, uses platform scan to discover local devices, opens each,
 * and invokes the callback. When sys_uri is provided (fabrics), delegates to
 * enumerate_controller to iterate namespaces on the target controller.
 *
 * @param sys_uri NULL for local devices, or fabrics URI for discovery
 * @param opts Options for dev_open (may be NULL for defaults)
 * @param cb_func Callback invoked for each opened device
 * @param cb_args User-provided arguments passed to callback
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned.
 */
int
xnvme_platform_enumerate(const char *sys_uri, struct xnvme_opts *opts, xnvme_enumerate_cb cb_func,
			 void *cb_args);

extern struct xnvme_platform g_xnvme_platform_linux;   /**< Linux platform instance */
extern struct xnvme_platform g_xnvme_platform_freebsd; /**< FreeBSD platform instance */
extern struct xnvme_platform g_xnvme_platform_macos;   /**< macOS platform instance */
extern struct xnvme_platform g_xnvme_platform_windows; /**< Windows platform instance */

/**
 * Global platform pointer - compile-time selected based on target OS
 *
 * Points to one of g_xnvme_platform_{linux,freebsd,macos,windows} based on
 * which XNVME_BE_*_ENABLED macro is defined at compile time.
 */
extern struct xnvme_platform *g_xnvme_platform;

#endif /* __INTERNAL_XNVME_PLATFORM_H */
