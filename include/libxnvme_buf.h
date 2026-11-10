/**
 * SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * @file libxnvme_buf.h
 */

/**
 * Allocate a buffer for IO with the given device
 *
 * The buffer will be aligned to device geometry and DMA allocated if required by the backend for
 * command payloads
 *
 * @note nbytes must be greater than zero and a multiple of minimal granularity
 * @note De-allocate the buffer using xnvme_buf_free()
 * @note Control plane: serialised with the other control-plane calls, not with I/O on the
 * same device, queue or buffer; see Thread Safety in the API documentation
 *
 * @param dev Device handle obtained with xnvme_dev_open_uri()
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
 * @note nbytes must be greater than zero and a multiple of minimal granularity
 * @note De-allocate the buffer using xnvme_buf_free()
 * @note Control plane: serialised with the other control-plane calls, not with I/O on the
 * same device, queue or buffer; see Thread Safety in the API documentation
 *
 * @param dev Device handle obtained with xnvme_dev_open_uri()
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
 * @note Control plane: serialised with the other control-plane calls, not with I/O on the
 * same device, queue or buffer; see Thread Safety in the API documentation
 *
 * @param dev Device handle obtained with xnvme_dev_open_uri()
 * @param buf Pointer to a buffer allocated with xnvme_buf_alloc()
 */
void
xnvme_buf_free(const struct xnvme_dev *dev, void *buf);

/**
 * Allocate a buffer of physical memory, aligned for IO with the given device
 *
 * @note nbytes must be greater than zero and a multiple of minimal granularity
 * @note De-allocate the buffer using xnvme_buf_phys_free()
 * @note Control plane: serialised with the other control-plane calls, not with I/O on the
 * same device, queue or buffer; see Thread Safety in the API documentation
 *
 * @param dev Device handle obtained with xnvme_dev_open_uri()
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
 * @note Control plane: serialised with the other control-plane calls, not with I/O on the
 * same device, queue or buffer; see Thread Safety in the API documentation
 *
 * @param dev Device handle obtained with xnvme_dev_open_uri()
 * @param buf Pointer to a buffer allocated with xnvme_buf_phys_alloc()
 */
void
xnvme_buf_phys_free(const struct xnvme_dev *dev, void *buf);

/**
 * Re-allocate a buffer of physical memory, aligned for IO with the given device
 *
 * @note nbytes must be greater than zero and a multiple of minimal granularity
 * @note De-allocate the buffer using xnvme_buf_phys_free()
 * @note Control plane: serialised with the other control-plane calls, not with I/O on the
 * same device, queue or buffer; see Thread Safety in the API documentation
 *
 * @param dev Device handle obtained with xnvme_dev_open_uri()
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
 * @note Control plane: serialised with the other control-plane calls, not with I/O on the
 * same device, queue or buffer; see Thread Safety in the API documentation
 *
 * @param dev Device handle obtained with xnvme_dev_open_uri()
 * @param buf Pointer to a buffer allocated with xnvme_buf_alloc()
 * @param phys A pointer to the variable to hold the physical address of the given buffer.
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned.
 */
int
xnvme_buf_vtophys(const struct xnvme_dev *dev, void *buf, uint64_t *phys);

/**
 * Register memory of the caller's for use as command payload with the given device
 *
 * Registering makes memory the caller allocated itself, rather than with xnvme_buf_alloc(),
 * usable as the payload of commands to the device.
 * What it does differs per backend, from programming an IOMMU on the caller's behalf, to a
 * dma-buf export, to nothing at all. With SPDK, a registration is the process's, for every device.
 *
 * Each registration is given back by one call to xnvme_buf_unregister(). Registering a range
 * that is registered already, or a range within one, is counted on that registration rather than
 * registered again; a range that overlaps a registration otherwise is refused with `-EBUSY`.
 * Memory from xnvme_buf_alloc() is usable already, and registering and unregistering it does
 * nothing.
 *
 * @note Unregister the memory with xnvme_buf_unregister()
 * @note vaddr and nbytes must be multiples of the alignment given by xnvme_buf_get_alignment()
 * @note Control plane: serialised with the other control-plane calls, not with I/O on the
 * same device, queue or buffer; see Thread Safety in the API documentation
 *
 * @param dev Device handle obtained with xnvme_dev_open_uri()
 * @param vaddr The address the caller accesses the memory at
 * @param nbytes The number of bytes to register, starting at vaddr
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned.
 */
int
xnvme_buf_register(const struct xnvme_dev *dev, void *vaddr, size_t nbytes);

/**
 * Register a dma-buf for use as command payload with the given device
 *
 * Commands and xnvme_buf_unregister() name the dma-buf by vaddr, the address of its first byte in
 * the caller's address space; its size is that of the dma-buf. Neither registering nor a command
 * accesses the memory with the CPU, so vaddr may name a range the caller cannot access, such as
 * one mapped PROT_NONE or a GPU's memory. The library keeps a duplicate of fd while the dma-buf is
 * registered, so the caller may close fd once this returns. Registering the dma-buf again at the
 * same vaddr is counted, as xnvme_buf_register() counts memory. Through a further device, register
 * the same dma-buf, by fd or a duplicate of it: another dma-buf of the same memory may be mapped
 * at other addresses, and then returns `-EBUSY`.
 *
 * @note Unregister the dma-buf with xnvme_buf_unregister()
 * @note vaddr and the size of the dma-buf must be multiples of the alignment given by
 * xnvme_buf_get_alignment()
 * @note Control plane: serialised with the other control-plane calls, not with I/O on the
 * same device, queue or buffer; see Thread Safety in the API documentation
 *
 * @param dev Device handle obtained with xnvme_dev_open_uri()
 * @param fd File descriptor of the dma-buf
 * @param vaddr The address that names the first byte of the dma-buf
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned; `-ENOSYS` when the
 * device does not register dma-bufs, and `-EBUSY` when vaddr lies inside a registered range, the
 * dma-buf partly overlaps one, or a different dma-buf registered at vaddr through another device
 * is mapped at other addresses.
 *
 * @errno
 */
int
xnvme_buf_register_dmabuf(const struct xnvme_dev *dev, int fd, void *vaddr);

/**
 * Unregister memory registered with xnvme_buf_register() or xnvme_buf_register_dmabuf()
 *
 * @note Control plane: serialised with the other control-plane calls, not with I/O on the
 * same device, queue or buffer; see Thread Safety in the API documentation
 *
 * @param dev Device handle obtained with xnvme_dev_open_uri()
 * @param vaddr The address the memory was registered at
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned.
 */
int
xnvme_buf_unregister(const struct xnvme_dev *dev, void *vaddr);

/**
 * Retrieve the alignment of memory to register with the given device
 *
 * The alignment is a power of two, and at least `sizeof(void *)`. It is the larger of what the
 * device needs to register memory, of any kind it takes, and what its I/O path needs of command
 * payloads. Retrieve it before allocating memory that is to be registered, to allocate it
 * suitably.
 *
 * @param dev Device handle obtained with xnvme_dev_open_uri()
 * @param align Set to the alignment on success
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned and `align` is left
 * untouched; `-ENOSYS` when the device does not register memory.
 */
int
xnvme_buf_get_alignment(const struct xnvme_dev *dev, size_t *align);

/**
 * Whether a property of memory holds, as xnvme_buf_get_props() reports it
 *
 * @enum xnvme_buf_prop
 */
enum xnvme_buf_prop {
	XNVME_BUF_PROP_UNKNOWN = 0x0, ///< The platform does not report the property
	XNVME_BUF_PROP_NO      = 0x1, ///< The property does not hold
	XNVME_BUF_PROP_YES     = 0x2, ///< The property holds
};

/**
 * Properties of a range of memory, as xnvme_buf_get_props() reports them
 *
 * cpu_access is XNVME_BUF_PROP_YES when the process maps every byte of the range readable and
 * writable, and XNVME_BUF_PROP_NO when a load or store in the range faults, as in a range reserved
 * with PROT_NONE to name a dma-buf. page_nbytes is the smallest page size of the mappings that
 * cover the range, and 0 when the process does not map all of it. numa_node is the NUMA node of
 * the present pages of the range, and -1 when the CPU cannot access the range, no page is present,
 * the pages are on more than one node, or move_pages() fails, as under a seccomp filter.
 *
 * @struct xnvme_buf_props
 */
struct xnvme_buf_props {
	uint32_t cpu_access;  ///< See enum xnvme_buf_prop
	int32_t numa_node;    ///< -1 when not on one node, or not known
	uint64_t page_nbytes; ///< 0 when not mapped, or not known
	uint8_t _rsvd[48];    ///< Zero; for properties to come
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_buf_props) == 64, "Incorrect size")

/**
 * Retrieve the properties of a range of memory from how the process maps it
 *
 * The range need not be allocated by or registered with xNVMe. The call does not access the
 * memory and does not make its pages present. On platforms other than Linux, cpu_access is
 * XNVME_BUF_PROP_UNKNOWN, page_nbytes is 0 and numa_node is -1.
 *
 * @note The cost grows with the number of mappings of the process; do not call in the I/O path
 *
 * @param vaddr Start of the range
 * @param nbytes Length of the range
 * @param props Set to the properties of the range, on success
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned; `-EINVAL` when props
 * is NULL, nbytes is 0, or the range runs past the end of the address space.
 */
int
xnvme_buf_get_props(const void *vaddr, size_t nbytes, struct xnvme_buf_props *props);

/**
 * Allocate a buffer of virtual memory of the given `alignment` and `nbytes`
 *
 * @note You must use xnvme_buf_virt_free() to de-allocate the buffer
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
 * The behaviour of this function changes according to the value of content.
 *
 * If content is a single character, the buffer will be filled with nbytes
 * repetitions of this character.
 * NOTE: Make sure the argument uses string notation, e.g., "!". Giving a pointer
 * to a char won't work.
 *
 * If content is a file name, the buffer will be filled with nbytes of the
 * content of this file.
 *
 * If content == "zero", the buffer will be filled with 0s
 * If content == "anum", the buffer will be filled with ASCII characters in order
 * If content == "rand-k", the buffer will be filled with ASCII characters
 * randomly sampled with `srand(time(NULL))`
 * If content == "rand-t", the buffer will be filled with ASCII characters
 * randomly sampled with `srand(0)`
 *
 * @note CUDA or HIP memory that a backend allocated with xnvme_buf_alloc() or registered is
 * reached through its runtime; any other memory is reached by the CPU
 *
 * @param buf Pointer to the buffer to fill
 * @param content A single character, name of a file, or special "zero", "anum", "rand-k", "rand-t"
 * @param nbytes Amount of bytes to fill in buf
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned.
 */
int
xnvme_buf_fill(void *buf, size_t nbytes, const char *content);

/**
 * Write zeroes to the first 'nbytes' of 'buf'
 *
 * @note CUDA or HIP memory that a backend allocated with xnvme_buf_alloc() or registered is
 * reached through its runtime; any other memory is reached by the CPU
 *
 * @param buf Pointer to the buffer to fill with zeroes
 * @param nbytes Amount of bytes to fill with zeroes in buf
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned.
 */
int
xnvme_buf_clear(void *buf, size_t nbytes);

/**
 * Copy 'nbytes' from 'src' to 'dst'
 *
 * @note CUDA or HIP memory that a backend allocated with xnvme_buf_alloc() or registered is
 * reached through its runtime; any other memory is reached by the CPU
 *
 * @param dst Pointer to the destination buffer
 * @param src Pointer to the source buffer
 * @param nbytes Amount of bytes to copy
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned.
 */
int
xnvme_buf_memcpy(void *dst, const void *src, size_t nbytes);

/**
 * Compare two buffers and count the number of differing bytes.
 *
 * @note CUDA or HIP memory that a backend allocated with xnvme_buf_alloc() or registered is
 * reached through its runtime; any other memory is reached by the CPU
 *
 * @param expected Pointer to buffer containing the expected data
 * @param actual Pointer to buffer to compare against the expected data
 * @param nbytes Number of bytes to compare
 * @param diff Output pointer storing the number of differing bytes
 *
 * @return 0 on success, negative `errno` on error.
 */
int
xnvme_buf_diff(const void *expected, const void *actual, size_t nbytes, size_t *diff);

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
