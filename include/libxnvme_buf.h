/**
 * SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * @headerfile libxnvme_buf.h
 */

/**
 * Allocate a buffer for IO with the given device
 *
 * The buffer will be aligned to device geometry and DMA allocated if required by the backend for
 * command payloads
 *
 * @note
 * nbytes must be greater than zero and a multiple of minimal granularity
 * @note
 * De-allocate the buffer using xnvme_buf_free()
 *
 * @param dev Device handle obtained with xnvme_dev_open()
 * @param nbytes The size of the allocated buffer in bytes
 *
 * @return On success, a pointer to the allocated memory is returned. On error, NULL is returned
 * and `errno` set to indicate the error.
 */
void *
xnvme_buf_alloc(const struct xnvme_dev *dev, size_t nbytes);

/**
 * Reallocate a buffer for IO with the given device
 *
 * The buffer will be aligned to device geometry and DMA allocated if required by the backend for
 * IO
 *
 * @note
 * nbytes must be greater than zero and a multiple of minimal granularity
 * @note
 * De-allocate the buffer using xnvme_buf_free()
 *
 * @param dev Device handle obtained with xnvme_dev_open()
 * @param buf The buffer to reallocate
 * @param nbytes The size of the allocated buffer in bytes
 *
 * @return On success, a pointer to the allocated memory is returned. On error, NULL is returned
 * and `errno` set to indicate the error.
 */
void *
xnvme_buf_realloc(const struct xnvme_dev *dev, void *buf, size_t nbytes);

/**
 * Free the given IO buffer allocated with xnvme_buf_alloc()
 *
 * @param dev Device handle obtained with xnvme_dev_open()
 * @param buf Pointer to a buffer allocated with xnvme_buf_alloc()
 */
void
xnvme_buf_free(const struct xnvme_dev *dev, void *buf);

/**
 * Allocate a buffer of physical memory, aligned for IO with the given device
 *
 * @note
 * nbytes must be greater than zero and a multiple of minimal granularity
 * @note
 * De-allocate the buffer using xnvme_buf_phys_free()
 *
 * @param dev Device handle obtained with xnvme_dev_open()
 * @param nbytes The size of the allocated buffer in bytes
 * @param phys A pointer to the variable to hold the physical address of the allocated buffer. If
 * NULL, the physical address is not returned.
 *
 * @return On success, a pointer to the allocated memory is returned. On error, NULL is returned
 * and `errno` set to indicate the error.
 */
void *
xnvme_buf_phys_alloc(const struct xnvme_dev *dev, size_t nbytes, uint64_t *phys);

/**
 * Free the given buffer of physical memory allocated with xnvme_buf_phys_alloc()
 *
 * @param dev Device handle obtained with xnvme_dev_open()
 * @param buf Pointer to a buffer allocated with xnvme_buf_phys_alloc()
 */
void
xnvme_buf_phys_free(const struct xnvme_dev *dev, void *buf);

/**
 * Re-allocate a buffer of physical memory, aligned for IO with the given device
 *
 * @note
 * nbytes must be greater than zero and a multiple of minimal granularity
 * @note
 * De-allocate the buffer using xnvme_buf_phys_free()
 *
 * @param dev Device handle obtained with xnvme_dev_open()
 * @param buf The buffer to reallocate
 * @param nbytes The size of the allocated buffer in bytes
 * @param phys A pointer to the variable to hold the physical address of the allocated buffer. If
 * NULL, the physical address is not returned.
 *
 * @return On success, a pointer to the allocated memory is returned. On error, NULL is returned
 * and `errno` set to indicate the error.
 */
void *
xnvme_buf_phys_realloc(const struct xnvme_dev *dev, void *buf, size_t nbytes, uint64_t *phys);

/**
 * Retrieve the physical address of the given buffer
 *
 * The intended use for this function is to provide the physical-address of a buffer-allocation
 * allocated with xnvme_buf_phys_alloc() or xnvme_buf_alloc(), where the 'phys' argument was either
 * not provided.
 *
 * @param dev Device handle obtained with xnvme_dev_open()
 * @param buf Pointer to a buffer allocated with xnvme_buf_alloc()
 * @param phys A pointer to the variable to hold the physical address of the given buffer.
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned.
 */
int
xnvme_buf_vtophys(const struct xnvme_dev *dev, void *buf, uint64_t *phys);

/**
 * Allocate a buffer of virtual memory of the given `alignment` and `nbytes`
 *
 * @note
 * You must use xnvme_buf_virt_free() to de-allocate the buffer
 *
 * @param alignment The alignment in bytes
 * @param nbytes The size of the buffer in bytes
 *
 * @return On success, a pointer to the allocated memory is return. On error, NULL is returned and
 * `errno` set to indicate the error
 */
void *
xnvme_buf_virt_alloc(size_t alignment, size_t nbytes);

/**
 * Free the given virtual memory buffer
 *
 * @param buf Pointer to a buffer allocated with xnvme_buf_virt_alloc()
 */
void
xnvme_buf_virt_free(void *buf);

/**
 * Fills `buf` with content `nbytes` of content
 *
 * @param buf Pointer to the buffer to fill
 * @param content Name of a file, or special "zero", "anum", "rand-k", "rand-t"
 * @param nbytes Amount of bytes to fill in buf
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned.
 */
int
xnvme_buf_fill(void *buf, size_t nbytes, const char *content);

/**
 * Write zeroes to the first 'nbytes' of 'buf'
 *
 * @param buf Pointer to the buffer to fill with zeroes
 * @param nbytes Amount of bytes to fill with zeroes in buf
 *
 * @return Returns the first argument.
 */
void *
xnvme_buf_clear(void *buf, size_t nbytes);

/**
 * Returns the number of bytes where expected is different from actual
 *
 * @param expected Pointer to buffer of "expected" content
 * @param actual Pointer to buffer to compare to "expected"
 * @param nbytes Amount of bytes to compare
 *
 * @return On success, returns number of bytes that differ
 */
size_t
xnvme_buf_diff(const void *expected, const void *actual, size_t nbytes);

/**
 * Prints the number and value of bytes where expected is different from actual
 *
 * @param expected Pointer to buffer of "expected" content
 * @param actual Pointer to buffer to compare to "expected"
 * @param nbytes Amount of bytes to compare
 * @param opts printer options, see ::xnvme_pr
 */
void
xnvme_buf_diff_pr(const void *expected, const void *actual, size_t nbytes, int opts);

/**
 * Write content of buffer into file
 *
 * - If file exists, then it is truncated / overwritten
 * - If file does NOT exist, then it is created
 * - When file is created, permissions are set to user WRITE + READ
 *
 * @param buf Pointer to the buffer
 * @param nbytes Size of buf
 * @param path Destination where buffer will be dumped to
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned.
 */
int
xnvme_buf_to_file(void *buf, size_t nbytes, const char *path);

/**
 * Read content of file into buffer
 *
 * @param buf Pointer to the buffer
 * @param nbytes Size of buf
 * @param path Source to read from
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned.
 */
int
xnvme_buf_from_file(void *buf, size_t nbytes, const char *path);
