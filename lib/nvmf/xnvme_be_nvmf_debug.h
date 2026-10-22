// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef __INTERNAL_XNVME_BE_NVMF_DEBUG_H
#define __INTERNAL_XNVME_BE_NVMF_DEBUG_H

#include <libxnvme.h>

enum {
	NVMF_DEBUG_LOG_LEVEL_NONE,
	NVMF_DEBUG_LOG_LEVEL_ERROR,
	NVMF_DEBUG_LOG_LEVEL_WARN,
	NVMF_DEBUG_LOG_LEVEL_INFO,
	NVMF_DEBUG_LOG_LEVEL_DEBUG,
	NVMF_DEBUG_LOG_LEVEL_TRACE,
	NVMF_DEBUG_LOG_LEVEL_MAX,
};

enum nvmf_debug_categories {
	NVMF_DEBUG_CATEGORY_ALL,
	NVMF_DEBUG_CATEGORY_CORE,
	NVMF_DEBUG_CATEGORY_CORE_CTRLR,
	NVMF_DEBUG_CATEGORY_CORE_DEV,
	NVMF_DEBUG_CATEGORY_CORE_NAMESPACE,
	NVMF_DEBUG_CATEGORY_CORE_QPAIR,
	NVMF_DEBUG_CATEGORY_CORE_SUBSYS,
	NVMF_DEBUG_CATEGORY_DISCOVERY,
	NVMF_DEBUG_CATEGORY_CMD,
	NVMF_DEBUG_CATEGORY_CMD_ADMIN,
	NVMF_DEBUG_CATEGORY_CMD_IO,
	NVMF_DEBUG_CATEGORY_CMD_DUMP,
	NVMF_DEBUG_CATEGORY_FABRICS,
	NVMF_DEBUG_CATEGORY_MEM,
	NVMF_DEBUG_CATEGORY_NVME,
	NVMF_DEBUG_CATEGORY_RDMACM,
	NVMF_DEBUG_CATEGORY_VERBS,
	NVMF_DEBUG_CATEGORY_VERBS_CTRL,
	NVMF_DEBUG_CATEGORY_VERBS_DATA,
	NVMF_DEBUG_CATEGORY_MAX,
};

#ifdef XNVME_DEBUG_ENABLED
#define XNVME_DEBUG_UNUSED(x) x

extern int nvmf_debug_log_level;
extern int nvmf_debug_categories[NVMF_DEBUG_CATEGORY_MAX];

void
xnvme_be_nvmf_debug_print_cpl_error_code(struct xnvme_spec_cpl *cpl);

static inline void
xnvme_be_nvmf_hexdump_range(int category, void *buf, size_t len)
{
	if (!nvmf_debug_categories[category])
		return;

	if (!(nvmf_debug_categories[NVMF_DEBUG_CATEGORY_CMD_DUMP] &&
	      nvmf_debug_log_level >= NVMF_DEBUG_LOG_LEVEL_DEBUG))
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
xnvme_be_nvmf_print_completion(struct xnvme_spec_cpl *cpl)
{
	if (nvmf_debug_categories[NVMF_DEBUG_CATEGORY_NVME] &&
	    nvmf_debug_log_level >= NVMF_DEBUG_LOG_LEVEL_DEBUG) {
		XNVME_DEBUG("INFO: NVMe Completion - cid: %u, sc: %u, sct: %u", cpl->cid,
			    cpl->status.sc, cpl->status.sct);
		if (cpl->status.sc || cpl->status.sct) {
			XNVME_DEBUG("INFO: NVMe Completion indicates an error");
			xnvme_be_nvmf_debug_print_cpl_error_code(cpl);
		}
	}
}

#define _NVMF_DEBUG_PRINT(level, category, ...)                                         \
	do {                                                                            \
		if (level <= nvmf_debug_log_level && nvmf_debug_categories[category]) { \
			XNVME_DEBUG(__VA_ARGS__);                                       \
		}                                                                       \
	} while (0)

#define _NVMF_DEBUG(category, ...) \
	_NVMF_DEBUG_PRINT(NVMF_DEBUG_LOG_LEVEL_DEBUG, category, __VA_ARGS__)
#define _NVMF_INFO(category, ...) \
	_NVMF_DEBUG_PRINT(NVMF_DEBUG_LOG_LEVEL_INFO, category, __VA_ARGS__)
#define _NVMF_WARN(category, ...) \
	_NVMF_DEBUG_PRINT(NVMF_DEBUG_LOG_LEVEL_WARN, category, __VA_ARGS__)
#define _NVMF_ERROR(category, ...) \
	_NVMF_DEBUG_PRINT(NVMF_DEBUG_LOG_LEVEL_ERROR, category, __VA_ARGS__)
#define _NVMF_TRACE(category, ...) \
	_NVMF_DEBUG_PRINT(NVMF_DEBUG_LOG_LEVEL_TRACE, category, __VA_ARGS__)
#define _NVMF_FATAL(category, ...)                    \
	do {                                          \
		_NVMF_ERROR(category, ##__VA_ARGS__); \
		assert(0);                            \
	} while (0)

#define NVMF_INFO(fmt, ...)  _NVMF_INFO(NVMF_DEBUG_CATEGORY, fmt, ##__VA_ARGS__)
#define NVMF_WARN(fmt, ...)  _NVMF_WARN(NVMF_DEBUG_CATEGORY, fmt, ##__VA_ARGS__)
#define NVMF_TRACE(fmt, ...) _NVMF_TRACE(NVMF_DEBUG_CATEGORY, fmt, ##__VA_ARGS__)
#define NVMF_ERROR(fmt, ...) _NVMF_ERROR(NVMF_DEBUG_CATEGORY, fmt, ##__VA_ARGS__)
#define NVMF_DEBUG(fmt, ...) _NVMF_DEBUG(NVMF_DEBUG_CATEGORY, fmt, ##__VA_ARGS__)
#define NVMF_FATAL(fmt, ...) _NVMF_FATAL(NVMF_DEBUG_CATEGORY, fmt, ##__VA_ARGS__)

#else
#define XNVME_DEBUG_UNUSED(x) XNVME_UNUSED(x)

static inline void
xnvme_be_nvmf_debug_print_cpl_error_code(struct xnvme_spec_cpl *XNVME_UNUSED(cpl))
{
	return;
}

static inline void
xnvme_be_nvmf_hexdump_range(int XNVME_UNUSED(category), void *XNVME_UNUSED(buf),
			    size_t XNVME_UNUSED(len))
{
	return;
}

static inline void
xnvme_be_nvmf_print_completion(struct xnvme_spec_cpl *XNVME_UNUSED(cpl))
{
	return;
}

#define _NVMF_DEBUG_PRINT(level, category, ...)

#define _NVMF_DEBUG(category, ...)
#define _NVMF_INFO(category, ...)
#define _NVMF_WARN(category, ...)
#define _NVMF_ERROR(category, ...)
#define _NVMF_TRACE(category, ...)
#define _NVMF_FATAL(category, ...)

#define NVMF_INFO(fmt, ...)
#define NVMF_WARN(fmt, ...)
#define NVMF_TRACE(fmt, ...)
#define NVMF_ERROR(fmt, ...)
#define NVMF_DEBUG(fmt, ...)
#define NVMF_FATAL(fmt, ...)
#endif

#endif // __INTERNAL_XNVME_BE_NVMF_DEBUG_H