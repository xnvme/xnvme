#ifndef _INTERNAL_XNVME_BE_NVMF_DEBUG_H
#define _INTERNAL_XNVME_BE_NVMF_DEBUG_H

#include <libxnvme.h>

extern int nvmf_debug_log_level;

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
	NVMF_DEBUG_CATEGORY_CORE_QPAIR,
	NVMF_DEBUG_CATEGORY_CORE_DEV,
	NVMF_DEBUG_CATEGORY_RDMACM,
	NVMF_DEBUG_CATEGORY_VERBS,
	NVMF_DEBUG_CATEGORY_VERBS_CTRL,
	NVMF_DEBUG_CATEGORY_VERBS_DATA,
	NVMF_DEBUG_CATEGORY_CMD,
	NVMF_DEBUG_CATEGORY_CMD_ADMIN,
	NVMF_DEBUG_CATEGORY_CMD_IO,
	NVMF_DEBUG_CATEGORY_CMD_DUMP,
	NVMF_DEBUG_CATEGORY_NVME,
	NVMF_DEBUG_CATEGORY_FABRICS,
	NVMF_DEBUG_CATEGORY_MAX,
};

extern int nvmf_debug_categories[NVMF_DEBUG_CATEGORY_MAX];

int
_xnvme_print_error_code(struct xnvme_spec_cpl *cpl);

static inline void
_hexdump_range(int category, void *buf, size_t len)
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
_print_nvme_completion(struct xnvme_spec_cpl *cpl)
{
	if (!nvmf_debug_categories[NVMF_DEBUG_CATEGORY_NVME] && 
		nvmf_debug_log_level < NVMF_DEBUG_LOG_LEVEL_DEBUG) 
		return;

	XNVME_DEBUG("INFO: NVMe Completion - cid: %u, sc: %u, sct: %u",
		    cpl->cid, cpl->status.sc, cpl->status.sct);
	if (cpl->status.sc || cpl->status.sct) {
		XNVME_DEBUG("INFO: NVMe Completion indicates an error");
		_xnvme_print_error_code(cpl);
	}
}

#define _NVMF_DEBUG_PRINT(level, category,...)                                                                    \
	do {                                                                                 \
		if (level <= nvmf_debug_log_level && nvmf_debug_categories[category]) { \
			XNVME_DEBUG(__VA_ARGS__);                                           \
		}                                                                         \
	} while (0);

#define NVMF_DEBUG(category,...) \
	_NVMF_DEBUG_PRINT(NVMF_DEBUG_LOG_LEVEL_DEBUG, category, __VA_ARGS__)
#define NVMF_INFO(category,...) \
	_NVMF_DEBUG_PRINT(NVMF_DEBUG_LOG_LEVEL_INFO, category, __VA_ARGS__)
#define NVMF_WARN(category,...) \
	_NVMF_DEBUG_PRINT(NVMF_DEBUG_LOG_LEVEL_WARN, category, __VA_ARGS__)
#define NVMF_ERROR(category,...) \
	_NVMF_DEBUG_PRINT(NVMF_DEBUG_LOG_LEVEL_ERROR, category, __VA_ARGS__)
#define NVMF_TRACE(category,...) \
	_NVMF_DEBUG_PRINT(NVMF_DEBUG_LOG_LEVEL_TRACE, category, __VA_ARGS__)

#endif // _INTERNAL_XNVME_BE_NVMF_DEBUG_H