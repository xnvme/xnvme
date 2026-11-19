/**
 * SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * @file libxnvme_opts.h
 */

/**
 * Size in bytes of the name fields of ::xnvme_opts, their terminating NUL included
 */
#define XNVME_OPTS_NAME_LEN 32

/**
 * Size in bytes of xnvme_opts.core_mask, its terminating NUL included
 */
#define XNVME_OPTS_CORE_MASK_LEN 128

/**
 * Size in bytes of the NQN fields of ::xnvme_opts: an NQN is at most 223 bytes, and a NUL
 */
#define XNVME_OPTS_NQN_LEN 224

struct xnvme_opts_css {
	uint32_t value;
	uint32_t given;
};

/**
 * xNVMe options
 *
 * The struct holds the strings themselves, so a copy of the struct, such as the copy the device
 * keeps, has its own strings. An empty string means that the option is not given.
 *
 * @see xnvme_dev_open_uri()
 *
 * @struct xnvme_opts
 */
struct xnvme_opts {
	char be[XNVME_OPTS_NAME_LEN];    ///< Backend/system interface to use
	char dev[XNVME_OPTS_NAME_LEN];   ///< Device manager/enumerator
	char mem[XNVME_OPTS_NAME_LEN];   ///< Memory allocator to use for buffers
	char sync[XNVME_OPTS_NAME_LEN];  ///< Synchronous Command-interface
	char async[XNVME_OPTS_NAME_LEN]; ///< Asynchronous Command-interface
	char admin[XNVME_OPTS_NAME_LEN]; ///< Administrative Command-interface
	uint32_t nsid;                   ///< Namespace identifier
	uint8_t rdonly;                  ///< OS open; for read only
	uint8_t wronly;                  ///< OS open; for write only
	uint8_t rdwr;                    ///< OS open; for read and write
	uint8_t create;                  ///< OS open; create if it does not exist
	uint8_t truncate;                ///< OS open; truncate content
	uint8_t direct;                  ///< OS open; bypass OS-managed caching
	uint32_t create_mode;            ///< OS file creation-mode
	uint8_t poll_io;                 ///< io_uring: enable io-polling
	uint8_t poll_sq;                 ///< io_uring: enable sqthread-polling
	uint8_t register_files;          ///< io_uring: enable file-registrations
	uint8_t register_buffers;        ///< io_uring: enable buffer-registration
	struct xnvme_opts_css css;       ///< SPDK controller-setup: do command-set-selection
	uint32_t use_cmb_sqs; ///< SPDK controller-setup: use controller-memory-buffer for sq
	uint32_t shm_id;      ///< SPDK: DPDK shared-memory-id
	uint32_t main_core;   ///< SPDK: main-core
	char core_mask[XNVME_OPTS_CORE_MASK_LEN]; ///< SPDK: core-mask
	char iova_mode[XNVME_OPTS_NAME_LEN];      ///< SPDK env-init: DPDK IOVA mode ("va" or "pa")
	char adrfam[XNVME_OPTS_NAME_LEN];         ///< SPDK fabrics: address-family, IPv4/IPv6
	char subnqn[XNVME_OPTS_NQN_LEN];          ///< SPDK fabrics: Subsystem NQN
	char hostnqn[XNVME_OPTS_NQN_LEN];         ///< SPDK fabrics: Host NQN
	uint32_t admin_timeout;                   ///< SPDK fabrics: enable admin command timeout
	uint32_t command_timeout;                 ///< SPDK fabrics: enable io command timeout
	uint32_t spdk_fabrics;          ///< Is assigned a value by backend if SPDK uses fabrics
	uint32_t keep_alive_timeout_ms; ///< SPDK fabrics: set keep alive timeout
	size_t host_heap_size;          ///< upcie: host DMA heap size in bytes (0 = default 1 GiB)
	size_t device_heap_size; ///< upcie-cuda/upcie-hip: GPU device heap size in bytes (0 =
				 ///< default 1 GiB)
	uint32_t gpu_id;         ///< upcie-cuda/upcie-hip: GPU ordinal to use (default 0)

	/** upcie: the control-plane server to attach to; 0 opens the controller here */
	uint32_t homi_id;
};

/**
 * Set the defaults of the given 'opts'
 *
 * Fields without a default are left as they are; zero-initialize 'opts' first, as in
 * 'struct xnvme_opts opts = {0};'.
 *
 * @param opts The options to set the defaults of
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned: -EINVAL when
 * `opts` is NULL.
 *
 * @errno
 */
int
xnvme_opts_set_defaults(struct xnvme_opts *opts);
