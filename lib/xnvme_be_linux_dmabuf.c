// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <xnvme_be.h>
#include <xnvme_be_nosys.h>
#ifdef XNVME_PLATFORM_LINUX_ENABLED
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <libxnvme.h>
#include <xnvme_dev.h>
#include <xnvme_be_linux.h>
#include <xnvme_be_linux_dmabuf.h>
#include <upcie/debug.h>
#include <upcie/dmabuf.h>
#include <upcie/experimental/dmabuf_import.h>
#include <upcie/experimental/iommu_map_pa.h>

struct xnvme_be_linux_dmabuf_entry {
	void *vaddr;
	size_t nbytes;
	uint64_t map_handle;
	struct dmabuf import; ///< Attachment backing 'map_handle'
	int fd;               ///< -1 when the entry is free
	uint32_t refs;
};

struct xnvme_be_linux_dmabuf {
	uint32_t generation;
	uint32_t nentries;
	struct xnvme_be_linux_dmabuf_entry *entries;
};

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

/**
 * Determine whether an IOMMU translates DMA addresses for the given device
 */
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

/**
 * The mappings live until unmapped or until this descriptor is closed, so it is
 * kept open
 */
static int
_iommu_fd(void)
{
	static int fd = -1;

	if (fd < 0) {
		fd = iommu_map_pa_open();
	}

	return fd;
}

/**
 * Map the physical pages of 'dmabuf_fd' into the IOMMU domain of the device
 *
 * The pages are mapped at IOVA == physical address, as the kernel hands the
 * physical addresses of the dma-buf to the device.
 *
 * On success, 'import' stays attached: the exporter may otherwise hand the
 * physical addresses to the next dma-buf. Detach it after unmapping.
 */
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

	// The IOVA is the physical address, so the pages must be one physical run
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
_entry_release(struct xnvme_be_linux_dmabuf_entry *entry)
{
	if (entry->map_handle) {
		_iommu_unmap(entry->map_handle);
		dmabuf_import_detach(&entry->import);
	}
	close(entry->fd);
	memset(entry, 0, sizeof(*entry));
	entry->fd = -1;
}

static struct xnvme_be_linux_dmabuf *
_registry(const struct xnvme_dev *dev, bool create)
{
	struct xnvme_be_linux_state *state = (void *)dev->be.state;
	struct xnvme_be_linux_dmabuf *registry;

	registry = state->dmabuf;
	if (registry || (!create)) {
		return registry;
	}

	registry = calloc(1, sizeof(*registry));
	if (!registry) {
		return NULL;
	}
	// A new queue is at generation 0, so start ahead of it
	registry->generation = 1;

	state->dmabuf = registry;

	return registry;
}

/**
 * Double the number of entries; the index of an entry never changes, as it is
 * the 'sqe->buf_index' of the rings
 */
static int
_registry_grow(struct xnvme_be_linux_dmabuf *registry)
{
	uint32_t nentries = registry->nentries ? registry->nentries * 2 : 4;
	struct xnvme_be_linux_dmabuf_entry *entries;

	// The index is a 'uint16_t'
	if (nentries > UINT16_MAX + 1) {
		return -ENOMEM;
	}

	entries = realloc(registry->entries, nentries * sizeof(*entries));
	if (!entries) {
		return -ENOMEM;
	}
	for (uint32_t i = registry->nentries; i < nentries; ++i) {
		memset(&entries[i], 0, sizeof(entries[i]));
		entries[i].fd = -1;
	}
	registry->entries = entries;
	registry->nentries = nentries;

	return 0;
}

int
xnvme_be_linux_dmabuf_add(const struct xnvme_dev *dev, void *vaddr, size_t nbytes, int fd)
{
	struct xnvme_be_linux_dmabuf_entry *entry;
	struct xnvme_be_linux_dmabuf *registry;
	uint32_t i;
	int err;

	if ((!vaddr) || (!nbytes) || (fd < 0)) {
		return -EINVAL;
	}

	registry = _registry(dev, true);
	if (!registry) {
		XNVME_DEBUG("FAILED: _registry()");
		return -ENOMEM;
	}

	for (i = 0; (i < registry->nentries) && (registry->entries[i].fd >= 0); ++i) {
	}
	if (i == registry->nentries) {
		err = _registry_grow(registry);
		if (err) {
			XNVME_DEBUG("FAILED: _registry_grow(), err: %d", err);
			return err;
		}
	}
	entry = &registry->entries[i];

	if (_iommu_translates(dev)) {
		err = _iommu_map(dev, fd, &entry->map_handle, &entry->import);
		if (err) {
			XNVME_DEBUG("FAILED: _iommu_map(), err: %d", err);
			return err;
		}
	}

	entry->vaddr = vaddr;
	entry->fd = fd;
	entry->nbytes = nbytes;
	entry->refs = 1;
	registry->generation += 1;

	return (int)i;
}

int
xnvme_be_linux_dmabuf_ref(const struct xnvme_dev *dev, uint16_t index)
{
	struct xnvme_be_linux_dmabuf *registry;

	registry = _registry(dev, false);
	if ((!registry) || (index >= registry->nentries) || (registry->entries[index].fd < 0)) {
		return -ENOENT;
	}

	registry->entries[index].refs += 1;

	return 0;
}

int
xnvme_be_linux_dmabuf_del(const struct xnvme_dev *dev, uint16_t index)
{
	struct xnvme_be_linux_dmabuf_entry *entry;
	struct xnvme_be_linux_dmabuf *registry;

	registry = _registry(dev, false);
	if ((!registry) || (index >= registry->nentries)) {
		return -ENOENT;
	}

	entry = &registry->entries[index];
	if (entry->fd < 0) {
		return -ENOENT;
	}

	entry->refs -= 1;
	if (entry->refs) {
		return 0;
	}

	_entry_release(entry);
	registry->generation += 1;

	return 0;
}

int
xnvme_be_linux_dmabuf_lookup(const struct xnvme_dev *dev, const void *buf, size_t nbytes,
			     uint16_t *index, uint64_t *offset)
{
	struct xnvme_be_linux_dmabuf *registry;

	registry = _registry(dev, false);
	if (!registry) {
		return -ENOENT;
	}

	for (uint32_t i = 0; i < registry->nentries; ++i) {
		struct xnvme_be_linux_dmabuf_entry *entry = &registry->entries[i];
		size_t entry_nbytes = entry->nbytes;
		uint64_t off;

		if (entry->fd < 0) {
			continue;
		}
		if ((buf < entry->vaddr) ||
		    (buf >= (void *)((uint8_t *)entry->vaddr + entry_nbytes))) {
			continue;
		}

		off = (uint64_t)((const uint8_t *)buf - (const uint8_t *)entry->vaddr);
		if ((off + nbytes) > entry_nbytes) {
			XNVME_DEBUG("FAILED: buf(%p, %zu) overruns its dma-buf mapping", buf,
				    nbytes);
			return -EINVAL;
		}

		*index = (uint16_t)i;
		*offset = off;

		return 0;
	}

	return -ENOENT;
}

uint32_t
xnvme_be_linux_dmabuf_generation(const struct xnvme_dev *dev, uint32_t *nentries)
{
	struct xnvme_be_linux_dmabuf *registry;

	registry = _registry(dev, false);
	if (!registry) {
		*nentries = 0;
		return 0;
	}

	*nentries = registry->nentries;

	return registry->generation;
}

int
xnvme_be_linux_dmabuf_fd(const struct xnvme_dev *dev, uint16_t index)
{
	struct xnvme_be_linux_dmabuf *registry;

	registry = _registry(dev, false);
	if ((!registry) || (index >= registry->nentries)) {
		return -ENOENT;
	}

	return (registry->entries[index].fd < 0) ? -ENOENT : registry->entries[index].fd;
}

void
xnvme_be_linux_dmabuf_term(const struct xnvme_dev *dev)
{
	struct xnvme_be_linux_state *state = (void *)dev->be.state;
	struct xnvme_be_linux_dmabuf *registry;

	registry = _registry(dev, false);
	if (!registry) {
		return;
	}

	for (uint32_t i = 0; i < registry->nentries; ++i) {
		if (registry->entries[i].fd >= 0) {
			_entry_release(&registry->entries[i]);
		}
	}
	free(registry->entries);
	free(registry);

	state->dmabuf = NULL;
}
#endif
