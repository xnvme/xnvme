// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <xnvme_be.h>
#include <xnvme_be_nosys.h>
#ifdef XNVME_PLATFORM_LINUX_ENABLED
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <libxnvme.h>
#include <xnvme_dev.h>
#include <xnvme_be_linux.h>
#include <xnvme_be_linux_dmabuf.h>
#include <xnvme_be_linux_dmabuf_iommu.h>

/** Advance the generation, stepping over the zero a ring starts on */
#define XNVME_BE_LINUX_DMABUF_GEN_NEXT(gen) ((gen) + 1 ? (gen) + 1 : 1)

/** One dma-buf made addressable for a device; 'fd' is -1 when the slot is free */
struct xnvme_be_linux_dmabuf_entry {
	void *vaddr;
	size_t nbytes;
	uint64_t map_handle; ///< IOMMU mapping to release with the entry; 0 when none
	int fd;
	uint32_t refs;
};

struct xnvme_be_linux_dmabuf {
	uint32_t generation;
	uint32_t nslots; ///< Highest index handed out, plus one
	struct xnvme_be_linux_dmabuf_entry entries[XNVME_BE_LINUX_DMABUF_MAX];
};

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
	for (int i = 0; i < XNVME_BE_LINUX_DMABUF_MAX; ++i) {
		registry->entries[i].fd = -1;
	}
	// A queue starts out at 0, so a fresh registry has to be past it
	registry->generation = 1;

	state->dmabuf = registry;

	return registry;
}

int
xnvme_be_linux_dmabuf_add(const struct xnvme_dev *dev, void *vaddr, size_t nbytes, int fd,
			  uint64_t map_handle)
{
	struct xnvme_be_linux_dmabuf *registry;

	if ((!vaddr) || (!nbytes) || (fd < 0)) {
		return -EINVAL;
	}

	registry = _registry(dev, true);
	if (!registry) {
		XNVME_DEBUG("FAILED: _registry()");
		return -ENOMEM;
	}

	for (int i = 0; i < XNVME_BE_LINUX_DMABUF_MAX; ++i) {
		struct xnvme_be_linux_dmabuf_entry *entry = &registry->entries[i];

		if (entry->fd >= 0) {
			continue;
		}

		entry->vaddr = vaddr;
		entry->fd = fd;
		entry->nbytes = nbytes;
		entry->map_handle = map_handle;
		entry->refs = 1;

		if ((uint32_t)i >= registry->nslots) {
			registry->nslots = (uint32_t)i + 1;
		}
		registry->generation = XNVME_BE_LINUX_DMABUF_GEN_NEXT(registry->generation);

		return i;
	}

	XNVME_DEBUG("FAILED: registry is full; XNVME_BE_LINUX_DMABUF_MAX: %d",
		    XNVME_BE_LINUX_DMABUF_MAX);

	return -ENOMEM;
}

int
xnvme_be_linux_dmabuf_ref(const struct xnvme_dev *dev, uint16_t index)
{
	struct xnvme_be_linux_dmabuf *registry;

	registry = _registry(dev, false);
	if ((!registry) || (index >= registry->nslots) || (registry->entries[index].fd < 0)) {
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
	if ((!registry) || (index >= registry->nslots)) {
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

	if (entry->map_handle) {
		xnvme_be_linux_dmabuf_iommu_unmap(entry->map_handle);
		entry->map_handle = 0;
	}
	close(entry->fd);
	entry->fd = -1;
	entry->vaddr = NULL;
	entry->nbytes = 0;
	registry->generation = XNVME_BE_LINUX_DMABUF_GEN_NEXT(registry->generation);

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

	for (uint32_t i = 0; i < registry->nslots; ++i) {
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
xnvme_be_linux_dmabuf_generation(const struct xnvme_dev *dev, uint32_t *nslots)
{
	struct xnvme_be_linux_dmabuf *registry;

	registry = _registry(dev, false);
	if (!registry) {
		*nslots = 0;
		return 0;
	}

	*nslots = registry->nslots;

	return registry->generation;
}

int
xnvme_be_linux_dmabuf_fd(const struct xnvme_dev *dev, uint16_t index)
{
	struct xnvme_be_linux_dmabuf *registry;

	if (index >= XNVME_BE_LINUX_DMABUF_MAX) {
		return -EINVAL;
	}

	registry = _registry(dev, false);
	if (!registry) {
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

	for (uint32_t i = 0; i < registry->nslots; ++i) {
		if (registry->entries[i].fd < 0) {
			continue;
		}
		if (registry->entries[i].map_handle) {
			xnvme_be_linux_dmabuf_iommu_unmap(registry->entries[i].map_handle);
		}
		close(registry->entries[i].fd);
	}
	free(registry);

	state->dmabuf = NULL;
}
#endif
