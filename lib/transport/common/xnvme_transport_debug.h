// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef __INTERNAL_XNVME_TRANSPORT_DEBUG_H
#define __INTERNAL_XNVME_TRANSPORT_DEBUG_H

#include <libxnvme.h>

enum {
	TRANSPORT_DEBUG_LOG_LEVEL_NONE,
	TRANSPORT_DEBUG_LOG_LEVEL_ERROR,
	TRANSPORT_DEBUG_LOG_LEVEL_WARN,
	TRANSPORT_DEBUG_LOG_LEVEL_INFO,
	TRANSPORT_DEBUG_LOG_LEVEL_DEBUG,
	TRANSPORT_DEBUG_LOG_LEVEL_TRACE,
	TRANSPORT_DEBUG_LOG_LEVEL_MAX,
};

enum transport_debug_categories {
	TRANSPORT_DEBUG_CATEGORY_ALL,
	TRANSPORT_DEBUG_CATEGORY_CORE,
	TRANSPORT_DEBUG_CATEGORY_CORE_CTRLR,
	TRANSPORT_DEBUG_CATEGORY_CORE_DEV,
	TRANSPORT_DEBUG_CATEGORY_CORE_NAMESPACE,
	TRANSPORT_DEBUG_CATEGORY_CORE_QPAIR,
	TRANSPORT_DEBUG_CATEGORY_CORE_SUBSYS,
	TRANSPORT_DEBUG_CATEGORY_DISCOVERY,
	TRANSPORT_DEBUG_CATEGORY_CMD,
	TRANSPORT_DEBUG_CATEGORY_CMD_ADMIN,
	TRANSPORT_DEBUG_CATEGORY_CMD_IO,
	TRANSPORT_DEBUG_CATEGORY_CMD_DUMP,
	TRANSPORT_DEBUG_CATEGORY_FABRICS,
	TRANSPORT_DEBUG_CATEGORY_MEM,
	TRANSPORT_DEBUG_CATEGORY_NVME,
	TRANSPORT_DEBUG_CATEGORY_RDMACM,
	TRANSPORT_DEBUG_CATEGORY_VERBS,
	TRANSPORT_DEBUG_CATEGORY_VERBS_CTRL,
	TRANSPORT_DEBUG_CATEGORY_VERBS_DATA,
	TRANSPORT_DEBUG_CATEGORY_MAX,
};

#ifdef XNVME_DEBUG_ENABLED
#define XNVME_DEBUG_UNUSED(x) x

extern int transport_debug_log_level;
extern int transport_debug_categories[TRANSPORT_DEBUG_CATEGORY_MAX];

void
xnvme_transport_debug_print_cpl_error_code(struct xnvme_spec_cpl *cpl);

static inline void
xnvme_transport_hexdump_range(int category, void *buf, size_t len)
{
	if (!transport_debug_categories[category])
		return;

	if (!(transport_debug_categories[TRANSPORT_DEBUG_CATEGORY_CMD_DUMP] &&
	      transport_debug_log_level >= TRANSPORT_DEBUG_LOG_LEVEL_DEBUG))
		return;

	for (size_t i = 0; i < len; ++i) {
		if (i % 16 == 0) {
			printf("\n%08zx: ", i);
		}
		printf("%02x ", ((unsigned char *)buf)[i]);
	}
	printf("\n");
}

static inline void
xnvme_transport_print_completion(struct xnvme_spec_cpl *cpl)
{
	if (transport_debug_categories[TRANSPORT_DEBUG_CATEGORY_NVME] &&
	    transport_debug_log_level >= TRANSPORT_DEBUG_LOG_LEVEL_DEBUG) {
		XNVME_DEBUG("INFO: NVMe Completion - cid: %u, sc: %u, sct: %u", cpl->cid,
			    cpl->status.sc, cpl->status.sct);
		if (cpl->status.sc || cpl->status.sct) {
			XNVME_DEBUG("INFO: NVMe Completion indicates an error");
			xnvme_transport_debug_print_cpl_error_code(cpl);
		}
	}
}

#define _TRANSPORT_DEBUG_PRINT(level, category, ...)                                              \
	do {                                                                                      \
		if (level <= transport_debug_log_level && transport_debug_categories[category]) { \
			XNVME_DEBUG(__VA_ARGS__);                                                 \
		}                                                                                 \
	} while (0)

#define _TRANSPORT_DEBUG(category, ...) \
	_TRANSPORT_DEBUG_PRINT(TRANSPORT_DEBUG_LOG_LEVEL_DEBUG, category, __VA_ARGS__)
#define _TRANSPORT_INFO(category, ...) \
	_TRANSPORT_DEBUG_PRINT(TRANSPORT_DEBUG_LOG_LEVEL_INFO, category, __VA_ARGS__)
#define _TRANSPORT_WARN(category, ...) \
	_TRANSPORT_DEBUG_PRINT(TRANSPORT_DEBUG_LOG_LEVEL_WARN, category, __VA_ARGS__)
#define _TRANSPORT_ERROR(category, ...) \
	_TRANSPORT_DEBUG_PRINT(TRANSPORT_DEBUG_LOG_LEVEL_ERROR, category, __VA_ARGS__)
#define _TRANSPORT_TRACE(category, ...) \
	_TRANSPORT_DEBUG_PRINT(TRANSPORT_DEBUG_LOG_LEVEL_TRACE, category, __VA_ARGS__)
#define _TRANSPORT_FATAL(category, ...)                    \
	do {                                               \
		_TRANSPORT_ERROR(category, ##__VA_ARGS__); \
		assert(0);                                 \
	} while (0)

#define TRANSPORT_INFO(fmt, ...)  _TRANSPORT_INFO(TRANSPORT_DEBUG_CATEGORY, fmt, ##__VA_ARGS__)
#define TRANSPORT_WARN(fmt, ...)  _TRANSPORT_WARN(TRANSPORT_DEBUG_CATEGORY, fmt, ##__VA_ARGS__)
#define TRANSPORT_TRACE(fmt, ...) _TRANSPORT_TRACE(TRANSPORT_DEBUG_CATEGORY, fmt, ##__VA_ARGS__)
#define TRANSPORT_ERROR(fmt, ...) _TRANSPORT_ERROR(TRANSPORT_DEBUG_CATEGORY, fmt, ##__VA_ARGS__)
#define TRANSPORT_DEBUG(fmt, ...) _TRANSPORT_DEBUG(TRANSPORT_DEBUG_CATEGORY, fmt, ##__VA_ARGS__)
#define TRANSPORT_FATAL(fmt, ...) _TRANSPORT_FATAL(TRANSPORT_DEBUG_CATEGORY, fmt, ##__VA_ARGS__)

#else
#define XNVME_DEBUG_UNUSED(x) XNVME_UNUSED(x)

static inline void
xnvme_transport_debug_print_cpl_error_code(struct xnvme_spec_cpl *XNVME_UNUSED(cpl))
{
	return;
}

static inline void
xnvme_transport_hexdump_range(int XNVME_UNUSED(category), void *XNVME_UNUSED(buf),
			      size_t XNVME_UNUSED(len))
{
	return;
}

static inline void
xnvme_transport_print_completion(struct xnvme_spec_cpl *XNVME_UNUSED(cpl))
{
	return;
}

#define _TRANSPORT_DEBUG_PRINT(level, category, ...)

#define _TRANSPORT_DEBUG(category, ...)
#define _TRANSPORT_INFO(category, ...)
#define _TRANSPORT_WARN(category, ...)
#define _TRANSPORT_ERROR(category, ...)
#define _TRANSPORT_TRACE(category, ...)
#define _TRANSPORT_FATAL(category, ...)

#define TRANSPORT_INFO(fmt, ...)
#define TRANSPORT_WARN(fmt, ...)
#define TRANSPORT_TRACE(fmt, ...)
#define TRANSPORT_ERROR(fmt, ...)
#define TRANSPORT_DEBUG(fmt, ...)
#define TRANSPORT_FATAL(fmt, ...)
#endif

#endif // __INTERNAL_XNVME_TRANSPORT_DEBUG_H