// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <xnvme_be.h>

#ifdef XNVME_BE_LINUX_IO_URING_DMABUF_ENABLED
#include <errno.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <xnvme_dev.h>
#include <xnvme_queue.h>
#include <xnvme_be_linux.h>
#include <xnvme_be_linux_liburing.h>
#include <xnvme_be_linux_io_uring_dmabuf.h>
// The uPCIe headers do not include their dependencies: dmabuf_import.h needs the
// three before it, and the system headers that iommu_map_pa.h includes
#include <upcie/debug.h>
#include <upcie/dmabuf.h>
#include <upcie/experimental/iommu_map_pa.h>
#include <upcie/experimental/dmabuf_import.h>

// A dma-buf registered with the rings of the device, at the 'sqe->buf_index' that
// is its index in the registry
struct xnvme_io_uring_dmabuf_entry {
	void *vaddr;
	size_t nbytes;
	uint64_t map_handle;
	struct dmabuf import; ///< Attachment backing 'map_handle'
	int fd;               ///< -1 when the entry is free
	uint32_t refs;
};

struct xnvme_io_uring_dmabuf {
	const struct xnvme_dev *dev;
	struct xnvme_io_uring_dmabuf *next;
	struct xnvme_queue_liburing **queues; ///< Rings holding the entries in their buffer-table
	uint32_t nqueues;
	uint32_t nentries; ///< One past the highest entry ever used
	struct xnvme_io_uring_dmabuf_entry entries[XNVME_IO_URING_DMABUF_NENTRIES];
};

// Guards the registries; IO only reads the entries of its own
static pthread_mutex_t g_registries_mutex = PTHREAD_MUTEX_INITIALIZER;
static struct xnvme_io_uring_dmabuf *g_registries;

static int
_sysfs_read(const struct xnvme_dev *dev, const char *attr, char *buf, int buf_len)
{
	int err;

	err = xnvme_be_linux_sysfs_dev_attr_to_buf((struct xnvme_dev *)dev, attr, buf, buf_len);
	if (err) {
		XNVME_DEBUG("FAILED: reading '%s' from sysfs, err: %d", attr, err);
		return err;
	}
	buf[strcspn(buf, "\n")] = '\0';

	return 0;
}

static int
_iommu_translates(const struct xnvme_dev *dev)
{
	char type[32];

	// No IOMMU group means nothing translates
	if (_sysfs_read(dev, "device/device/iommu_group/type", type, sizeof(type))) {
		return 0;
	}

	// 'DMA' and 'DMA-FQ' translate; 'identity' and 'unmanaged' do not
	return strncmp(type, "DMA", 3) ? 0 : 1;
}

static int
_iommu_fd(void)
{
	// The mappings live until unmapped or until this descriptor is closed
	static int fd = -1;

	if (fd < 0) {
		fd = iommu_map_pa_open();
	}

	return fd;
}

static int
_iommu_map(const struct xnvme_dev *dev, int dmabuf_fd, uint64_t *handle, struct dmabuf *import)
{
	struct dmabuf dmabuf = {0};
	char bdf[32];
	uint64_t *phys;
	int fd, import_fd, err;

	err = _sysfs_read(dev, "device/address", bdf, sizeof(bdf));
	if (err) {
		return err;
	}

	fd = _iommu_fd();
	if (fd < 0) {
		XNVME_DEBUG("FAILED: iommu_map_pa_open(), err: %d", fd);
		return fd;
	}

	// Detaching closes the descriptor, which the caller still owns
	import_fd = dup(dmabuf_fd);
	if (import_fd < 0) {
		return -errno;
	}
	err = dmabuf_import_attach(import_fd, &dmabuf);
	if (err) {
		XNVME_DEBUG("FAILED: dmabuf_import_attach(), err: %d", err);
		close(import_fd);
		return err;
	}

	phys = calloc(dmabuf.npages, sizeof(*phys));
	if (!phys) {
		err = -ENOMEM;
		goto exit;
	}

	// The kernel hands the physical addresses of the dma-buf to the device, so
	// they are mapped at IOVA == physical address and must be one physical run
	for (size_t i = 0; i < dmabuf.npages; ++i) {
		phys[i] = dmabuf.pages[i].addr;
		if (i && (phys[i] != phys[i - 1] + dmabuf.pages[i - 1].len)) {
			XNVME_DEBUG("FAILED: dma-buf is not physically contiguous");
			err = -ENOTSUP;
			goto exit;
		}
	}

	err = iommu_map_pa_add(fd, bdf, dmabuf_fd, phys[0], (uint32_t)dmabuf.pages[0].len,
			       (uint32_t)dmabuf.npages, phys,
			       IOMMU_MAP_PA_PROT_READ | IOMMU_MAP_PA_PROT_WRITE, handle);
	if (err) {
		XNVME_DEBUG("FAILED: iommu_map_pa_add(%s), err: %d", bdf, err);
	}

exit:
	free(phys);
	if (err) {
		dmabuf_import_detach(&dmabuf);
		return err;
	}
	// Stays attached until unmapped, as the exporter may otherwise hand the
	// physical addresses to the next dma-buf
	*import = dmabuf;

	return 0;
}

static void
_iommu_unmap(uint64_t handle)
{
	int err;

	err = iommu_map_pa_del(_iommu_fd(), handle);
	if (err) {
		XNVME_DEBUG("FAILED: iommu_map_pa_del(), err: %d", err);
	}
}

static void
_entry_release(struct xnvme_io_uring_dmabuf_entry *entry)
{
	if (entry->map_handle) {
		_iommu_unmap(entry->map_handle);
		dmabuf_import_detach(&entry->import);
	}
	close(entry->fd);
	memset(entry, 0, sizeof(*entry));
	entry->fd = -1;
}

static int
_registry(const struct xnvme_dev *dev, bool create, struct xnvme_io_uring_dmabuf **registry)
{
	struct xnvme_io_uring_dmabuf *reg;

	for (reg = g_registries; reg; reg = reg->next) {
		if (reg->dev == dev) {
			*registry = reg;
			return 0;
		}
	}
	if (!create) {
		return -ENOENT;
	}

	reg = calloc(1, sizeof(*reg));
	if (!reg) {
		return -ENOMEM;
	}
	for (uint32_t i = 0; i < XNVME_IO_URING_DMABUF_NENTRIES; ++i) {
		reg->entries[i].fd = -1;
	}

	reg->dev = dev;
	reg->next = g_registries;
	g_registries = reg;

	*registry = reg;

	return 0;
}

static int
_registry_add(struct xnvme_io_uring_dmabuf *registry, void *vaddr, size_t nbytes, int fd)
{
	struct xnvme_io_uring_dmabuf_entry *entry;
	uint32_t i;
	int err;

	for (i = 0; i < XNVME_IO_URING_DMABUF_NENTRIES; ++i) {
		if (registry->entries[i].fd < 0) {
			break;
		}
	}
	if (i == XNVME_IO_URING_DMABUF_NENTRIES) {
		XNVME_DEBUG("FAILED: all %d dma-buf entries are in use",
			    XNVME_IO_URING_DMABUF_NENTRIES);
		return -ENOMEM;
	}
	entry = &registry->entries[i];

	if (_iommu_translates(registry->dev)) {
		err = _iommu_map(registry->dev, fd, &entry->map_handle, &entry->import);
		if (err) {
			XNVME_DEBUG("FAILED: _iommu_map(), err: %d", err);
			return err;
		}
	}

	entry->vaddr = vaddr;
	entry->fd = fd;
	entry->nbytes = nbytes;
	entry->refs = 1;
	if (i >= registry->nentries) {
		registry->nentries = i + 1;
	}

	return i;
}

static int
_registry_lookup(struct xnvme_io_uring_dmabuf *registry, const void *buf, size_t nbytes,
		 struct xnvme_io_uring_dmabuf_entry **entry, uint64_t *offset)
{
	for (uint32_t i = 0; i < registry->nentries; ++i) {
		struct xnvme_io_uring_dmabuf_entry *ent = &registry->entries[i];
		uint64_t off;

		if (ent->fd < 0) {
			continue;
		}
		if ((buf < ent->vaddr) || (buf >= (void *)((uint8_t *)ent->vaddr + ent->nbytes))) {
			continue;
		}

		off = (uint64_t)((const uint8_t *)buf - (const uint8_t *)ent->vaddr);
		if ((off + nbytes) > ent->nbytes) {
			XNVME_DEBUG("FAILED: buf(%p, %zu) overruns its dma-buf", buf, nbytes);
			return -EINVAL;
		}

		*entry = ent;
		*offset = off;

		return 0;
	}

	return -ENOENT;
}

void
xnvme_io_uring_dmabuf_term(const struct xnvme_dev *dev)
{
	struct xnvme_io_uring_dmabuf **link, *registry;

	pthread_mutex_lock(&g_registries_mutex);

	for (link = &g_registries; *link; link = &(*link)->next) {
		if ((*link)->dev == dev) {
			break;
		}
	}
	registry = *link;
	if (registry) {
		*link = registry->next;
	}

	pthread_mutex_unlock(&g_registries_mutex);

	if (!registry) {
		return;
	}

	for (uint32_t i = 0; i < registry->nentries; ++i) {
		if (registry->entries[i].fd >= 0) {
			_entry_release(&registry->entries[i]);
		}
	}
	free(registry->queues);
	free(registry);
}

// Set the given slot of the buffer-table of the ring to the dma-buf 'fd', or
// empty it when 'fd' is negative
static int
_ring_update(struct xnvme_queue_liburing *queue, uint32_t index, int fd)
{
	struct xnvme_be_linux_state *state = (void *)queue->base.dev->be.state;
	struct io_uring_regbuf_desc desc = {0};
	struct io_uring_rsrc_update2 up = {0};
	int err;

	desc.type = (fd < 0) ? IO_REGBUF_TYPE_EMPTY : IO_REGBUF_TYPE_DMABUF;
	desc.dmabuf_fd = (fd < 0) ? 0 : fd;
	desc.target_fd = (fd < 0) ? 0 : state->fd;

	up.offset = index;
	up.nr = 1;
	up.data = (unsigned long)&desc;
	// NOTE: the kernel names this member 'flags'; liburing does not yet
	up.resv = IORING_RSRC_UPDATE_EXTENDED;

	err = io_uring_register(queue->ring.ring_fd, IORING_REGISTER_BUFFERS_UPDATE, &up,
				sizeof(up));
	if (err != 1) {
		XNVME_DEBUG("FAILED: io_uring_register(BUFFERS_UPDATE), err: %d", err);
		return (err < 0) ? err : -EIO;
	}

	return 0;
}

int
xnvme_io_uring_dmabuf_ref(const struct xnvme_dev *dev, const void *vaddr, size_t nbytes)
{
	struct xnvme_io_uring_dmabuf_entry *entry;
	struct xnvme_io_uring_dmabuf *registry;
	uint64_t offset;
	int err;

	pthread_mutex_lock(&g_registries_mutex);

	err = _registry(dev, false, &registry);
	if (err) {
		goto exit;
	}
	err = _registry_lookup(registry, vaddr, nbytes, &entry, &offset);
	if (err) {
		goto exit;
	}

	entry->refs += 1;

exit:
	pthread_mutex_unlock(&g_registries_mutex);

	return err;
}

int
xnvme_io_uring_dmabuf_add(const struct xnvme_dev *dev, void *vaddr, size_t nbytes, int fd)
{
	struct xnvme_io_uring_dmabuf_entry *entry;
	struct xnvme_io_uring_dmabuf *registry;
	uint64_t offset;
	int index, err;

	pthread_mutex_lock(&g_registries_mutex);

	err = _registry(dev, true, &registry);
	if (err) {
		XNVME_DEBUG("FAILED: _registry(), err: %d", err);
		goto exit;
	}

	// Another thread may have added the allocation since the caller looked
	err = _registry_lookup(registry, vaddr, nbytes, &entry, &offset);
	if (!err) {
		entry->refs += 1;
		err = -EEXIST;
		goto exit;
	}
	if (err != -ENOENT) {
		goto exit;
	}

	index = _registry_add(registry, vaddr, nbytes, fd);
	if (index < 0) {
		XNVME_DEBUG("FAILED: _registry_add(), err: %d", index);
		err = index;
		goto exit;
	}

	err = 0;
	for (uint32_t i = 0; i < registry->nqueues; ++i) {
		err = _ring_update(registry->queues[i], index, fd);
		if (err) {
			XNVME_DEBUG("FAILED: _ring_update(), err: %d", err);
			while (i--) {
				_ring_update(registry->queues[i], index, -1);
			}
			_entry_release(&registry->entries[index]);
			goto exit;
		}
	}

	XNVME_DEBUG("INFO: registered dma-buf(%p, %zu) at index: %d", vaddr, nbytes, index);

exit:
	pthread_mutex_unlock(&g_registries_mutex);

	return err;
}

int
xnvme_io_uring_dmabuf_unref(const struct xnvme_dev *dev, const void *vaddr)
{
	struct xnvme_io_uring_dmabuf_entry *entry;
	struct xnvme_io_uring_dmabuf *registry;
	uint64_t offset;
	int err;

	pthread_mutex_lock(&g_registries_mutex);

	err = _registry(dev, false, &registry);
	if (err) {
		goto exit;
	}
	err = _registry_lookup(registry, vaddr, 1, &entry, &offset);
	if (err) {
		goto exit;
	}

	entry->refs -= 1;
	if (entry->refs) {
		goto exit;
	}

	for (uint32_t i = 0; i < registry->nqueues; ++i) {
		_ring_update(registry->queues[i], (uint32_t)(entry - registry->entries), -1);
	}
	_entry_release(entry);

exit:
	pthread_mutex_unlock(&g_registries_mutex);

	return err;
}

int
xnvme_io_uring_dmabuf_lookup(struct xnvme_io_uring_dmabuf *registry, const void *buf,
			     size_t nbytes, uint16_t *index, uint64_t *offset)
{
	struct xnvme_io_uring_dmabuf_entry *entry;
	int err;

	err = _registry_lookup(registry, buf, nbytes, &entry, offset);
	if (err) {
		return err;
	}

	*index = (uint16_t)(entry - registry->entries);

	return 0;
}

int
xnvme_io_uring_dmabuf_attach(struct xnvme_queue_liburing *queue)
{
	struct xnvme_io_uring_dmabuf *registry;
	struct xnvme_queue_liburing **queues;
	int err;

	err = io_uring_register_buffers_sparse(&queue->ring, XNVME_IO_URING_DMABUF_NENTRIES);
	if (err) {
		XNVME_DEBUG("FAILED: io_uring_register_buffers_sparse(), err: %d", err);
		return err;
	}

	pthread_mutex_lock(&g_registries_mutex);

	err = _registry(queue->base.dev, true, &registry);
	if (err) {
		XNVME_DEBUG("FAILED: _registry(), err: %d", err);
		goto exit;
	}

	// Load the dma-bufs mapped before the queue existed
	for (uint32_t i = 0; i < registry->nentries; ++i) {
		if (registry->entries[i].fd < 0) {
			continue;
		}
		err = _ring_update(queue, i, registry->entries[i].fd);
		if (err) {
			XNVME_DEBUG("FAILED: _ring_update(), err: %d", err);
			goto exit;
		}
	}

	queues = realloc(registry->queues, (registry->nqueues + 1) * sizeof(*queues));
	if (!queues) {
		err = -ENOMEM;
		goto exit;
	}
	queues[registry->nqueues++] = queue;
	registry->queues = queues;

	queue->dmabuf = registry;

exit:
	pthread_mutex_unlock(&g_registries_mutex);

	return err;
}

void
xnvme_io_uring_dmabuf_detach(struct xnvme_queue_liburing *queue)
{
	struct xnvme_io_uring_dmabuf *registry = queue->dmabuf;

	if (!registry) {
		return;
	}

	pthread_mutex_lock(&g_registries_mutex);

	for (uint32_t i = 0; i < registry->nqueues; ++i) {
		if (registry->queues[i] == queue) {
			registry->queues[i] = registry->queues[--registry->nqueues];
			break;
		}
	}

	pthread_mutex_unlock(&g_registries_mutex);

	queue->dmabuf = NULL;
}
#endif
