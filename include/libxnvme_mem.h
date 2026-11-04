/**
 * SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * @file libxnvme_mem.h
 */

/**
 * Map a buffer for IO with the given device
 *
 * The buffer will be aligned to device geometry and DMA allocated if required by the backend for
 * command payloads
 *
 * @note vaddr and nbytes must meet the requirements given by xnvme_mem_map_get_req(), and nbytes
 * be greater than zero
 * @note Unmap the buffer using xnvme_mem_unmap()
 * @note Control plane: serialised with the other control-plane calls, not with I/O on the
 * same device, queue or buffer; see Thread Safety in the API documentation
 *
 * @param dev Device handle obtained with xnvme_dev_open()
 * @param vaddr Pointer to start of virtual memory to use as mapped memory
 * @param nbytes The number of bytes to map, starting at vaddr
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned
 */
int
xnvme_mem_map(const struct xnvme_dev *dev, void *vaddr, size_t nbytes);

/**
 * Unmap the given IO buffer mapped with xnvme_mem_map()
 *
 * @note Control plane: serialised with the other control-plane calls, not with I/O on the
 * same device, queue or buffer; see Thread Safety in the API documentation
 *
 * @param dev Device handle obtained with xnvme_dev_open()
 * @param buf Pointer to a buffer allocated with xnvme_buf_alloc()
 *
 * @return On sucess, 0 is returned. On error, negative 'errno' is returned
 */
int
xnvme_mem_unmap(const struct xnvme_dev *dev, void *buf);

/**
 * Requirements on memory to map with xnvme_mem_map()
 *
 * @struct xnvme_mem_map_req
 */
struct xnvme_mem_map_req {
	uint64_t align; ///< The start and length must be multiples of this
	uint8_t rsvd[56];
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_mem_map_req) == 64, "Incorrect size")

/**
 * Retrieve the requirements on memory to map with the given device
 *
 * Query them before allocating memory that is to be mapped, to allocate it suitably.
 *
 * @param dev Device handle obtained with xnvme_dev_open()
 * @param req Filled on success
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned and `req` is left
 * untouched; `-ENOSYS` when the device does not support mapping memory
 */
int
xnvme_mem_map_get_req(const struct xnvme_dev *dev, struct xnvme_mem_map_req *req);
