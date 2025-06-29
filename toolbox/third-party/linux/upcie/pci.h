// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Simon Andreas Frimann Lund <os@safl.dk>

/**
 * Helpers for Linux PCI interface via sysfs
 * =========================================
 *
 * - Scan system for PCI devices / functions
 *   - Callback invocation on each discovered function
 *
 * - Retrieve "handles" to PCI devices via pci_func_{open,close} using PCI BDF
 *  - Handles provide PCI addresses, identifiers, and a container for BAR regions
 *
 * - Does BAR region mapping via /sys/bus/pci/devices/<PCI_ADDR>/resourceX
 *
 * - Provides MMIO accessor functions: pci_region_read32, pci_region_read64, pci_region_write32,
 *   and pci_region_write64
 *
 * @file pci.h
 * @version 0.3.2
 */
#define PCI_BDF_LEN 12
#define PCI_NBARS 6

enum pci_scan_action { PCI_SCAN_ACTION_CLAIM_FUNC = 0x1, PCI_SCAN_ACTION_RELEASE_FUNC = 0x2 };

struct pci_addr {
	uint32_t value;
};

static inline uint8_t
pci_addr_get_function(uint32_t bdf)
{
	return bdf & 0x7;
}

static inline uint8_t
pci_addr_get_device(uint32_t bdf)
{
	return (bdf >> 3) & 0x1f;
}

static inline uint8_t
pci_addr_get_bus(uint32_t bdf)
{
	return (bdf >> 8) & 0xff;
}

static inline uint16_t
pci_addr_get_domain(uint32_t bdf)
{
	return (bdf >> 16) & 0xffff;
}

/**
 * Representation of PCI identifiers
 */
struct pci_idents {
	uint16_t vendor_id; ///< Device vendor; e.g. Samsung, QEMU
	uint16_t device_id; ///< Device identifier;
	uint32_t classcode; ///< Base, sub, and programming-interface
};

/**
 * Encapsulation of a PCI BAR region mapping
 */
struct pci_func_bar {
	uint64_t size; ///< The size of the BAR region
	void *region;  ///< Pointer to mmap'ed BAR region
	uint8_t id;    ///< One of the six BARs; [0-5]
	int fd;        ///< Handle to file-representation
};

struct pci_func {
	struct pci_addr addr;                ///< The address of the PCI device function
	char bdf[PCI_BDF_LEN + 1];           ///< PCI address as a null-terminated full BDF string
	struct pci_idents ident;             ///< Describes who made it and what it is
	struct pci_func_bar bars[PCI_NBARS]; ///< The six BARs associated with a PCI Function
};

/**
 * Callback function definition for pci_scan(); must return a 'pci_scan_action'
 *
 * Return PCI_SCAN_ACTION_CLAIM_FUNC to take ownership of the struct pci_func,
 * or PCI_SCAN_ACTION_RELEASE_FUNC to have pci_scan() clean it up.
 */
typedef int (*pci_func_callback)(struct pci_func *func, void *callback_arg);

static inline int
pci_bar_pr(struct pci_func_bar *bar)
{
	int wrtn = 0;

	printf("pci_bar:\n");
	printf("  id: %" PRIu8 "\n", bar->id);
	printf("  fd: %d\n", bar->fd);
	printf("  size: %" PRIu64 "\n", bar->size);
	printf("  region: %p\n", bar->region);

	return wrtn;
}

static inline int
pci_func_pr(struct pci_func *func)
{
	int wrtn = 0;

	printf("pci_func:\n");
	printf("  addr: '%04" PRIx16 ":%02" PRIx8 ":%02" PRIx8 ".%01" PRIx8
	       "' # numerical representation printed as string\n",
	       pci_addr_get_domain(func->addr.value), pci_addr_get_bus(func->addr.value),
	       pci_addr_get_device(func->addr.value), pci_addr_get_function(func->addr.value));

	printf("  bdf: '%.*s'  # string representation printed as is\n", PCI_BDF_LEN, func->bdf);
	printf("  ident:\n");
	printf("    vendor_id: 0x%" PRIx16 "\n", func->ident.vendor_id);
	printf("    device_id: 0x%" PRIx16 "\n", func->ident.device_id);
	printf("    classcode: 0x%" PRIx32 "\n", func->ident.classcode);

	return wrtn;
}

/**
 * Fills the given 'addr' with parts found when scanning a text repr. on the form '0000:05:00.0'
 */
static inline int
pci_addr_from_text(const char *text, struct pci_addr *addr)
{
	int domain, bus, device, function;
	int fields;

	fields = sscanf(text, "%4x:%2x:%2x.%1x", &domain, &bus, &device, &function);
	if (fields != 4) {
		return -EINVAL;
	}

	if ((domain > 0xFFFF) || (bus > 0xFF) || (device > 0x1F) || (function > 0x07)) {
		return -EINVAL;
	}

	addr->value = (domain << 16) | (bus << 8) | (device << 3) | function;

	return 0;
}

/**
 * Populates the given char-array with a textual representation of the given 'addr'
 */
static inline int
pci_addr_to_text(struct pci_addr *addr, char *text)
{
	snprintf(text, PCI_BDF_LEN + 1, "%04" PRIx16 ":%02" PRIx8 ":%02" PRIx8 ".%01" PRIx8,
		 pci_addr_get_domain(addr->value), pci_addr_get_bus(addr->value),
		 pci_addr_get_device(addr->value), pci_addr_get_function(addr->value));

	return 0;
}

static inline int
pci_func_open(const char *bdf, struct pci_func *func)
{
	const char *sysfs_root = "/sys/bus/pci/devices";
	char path[256] = {0};
	char buf[16] = {0};
	ssize_t ret;
	int fd, err;

	err = pci_addr_from_text(bdf, &func->addr);
	if (err) {
		return err;
	}
	snprintf(func->bdf, sizeof(func->bdf), "%.*s", PCI_BDF_LEN, bdf);

	// vendor_id
	snprintf(path, sizeof(path), "%s/%s/vendor", sysfs_root, func->bdf);

	fd = open(path, O_RDONLY);
	if (fd < 0) {
		return -errno;
	}

	ret = read(fd, buf, sizeof(buf) - 1);
	if (ret < 0) {
		close(fd);
		return -errno;
	}
	buf[ret] = 0;

	func->ident.vendor_id = strtoul(buf, NULL, 16);
	close(fd);

	// device_id
	snprintf(path, sizeof(path), "%s/%s/device", sysfs_root, func->bdf);
	fd = open(path, O_RDONLY);
	if (fd < 0) {
		return -errno;
	}

	ret = read(fd, buf, sizeof(buf) - 1);
	if (ret < 0) {
		close(fd);
		return -errno;
	}
	buf[ret] = 0;

	func->ident.device_id = strtoul(buf, NULL, 16);
	close(fd);

	// classcode
	snprintf(path, sizeof(path), "%s/%s/class", sysfs_root, func->bdf);
	fd = open(path, O_RDONLY);
	if (fd < 0) {
		return -errno;
	}

	ret = read(fd, buf, sizeof(buf) - 1);
	if (ret < 0) {
		close(fd);
		return -errno;
	}
	buf[ret] = 0;

	func->ident.classcode = strtoul(buf, NULL, 16);
	close(fd);

	for (int id = 0; id < PCI_NBARS; ++id) {
		func->bars[id].id = id;
		func->bars[id].fd = -1;
	}

	return 0;
}

static inline int
pci_bar_unmap(struct pci_func_bar *bar)
{
	if (!bar) {
		return -EINVAL;
	}

	if (bar->region) {
		munmap(bar->region, bar->size);
		close(bar->fd);
	}

	bar->fd = -1;
	bar->region = NULL;

	return 0;
}

static inline int
pci_bar_map(const char *bdf, uint8_t id, struct pci_func_bar *bar)
{
	struct stat barstat = {0};
	char path[256] = {0};
	int err;

	snprintf(path, sizeof(path), "/sys/bus/pci/devices/%.*s/resource%" PRIu8, PCI_BDF_LEN, bdf,
		 id);

	err = stat(path, &barstat);
	if (err) {
		return -errno;
	}

	bar->fd = open(path, O_RDWR);
	if (bar->fd == -1) {
		return -errno;
	}

	bar->size = barstat.st_size;
	bar->id = id;
	bar->region = mmap(NULL, bar->size, PROT_READ | PROT_WRITE, MAP_SHARED, bar->fd, 0x0);
	if (MAP_FAILED == bar->region) {
		close(bar->fd);
		return -errno;
	}

	return 0;
}

static inline void
pci_func_close(struct pci_func *func)
{
	if (!func) {
		return;
	}

	for (int id = 0; id < PCI_NBARS; ++id) {
		pci_bar_unmap(&func->bars[id]);
	}
}

/**
 * Scans /sys/bus/pci/devices for PCI functions and calls the provided callback for each one
 */
static inline int
pci_scan(pci_func_callback callback, void *callback_arg)
{
	const char *sysfs_path = "/sys/bus/pci/devices";
	int err = 0;
	struct dirent *entry;
	DIR *dir;

	dir = opendir(sysfs_path);
	if (!dir) {
		return -errno;
	}

	while ((entry = readdir(dir))) {
		struct pci_func *func;
		int action;

		if (entry->d_name[0] == '.') {
			continue;
		}

		func = calloc(1, sizeof(*func));
		if (!func) {
			return -errno;
		}

		err = pci_func_open(entry->d_name, func);
		if (err) {
			free(func);
			continue;
		}

		action = callback(func, callback_arg);
		switch (action) {
		case PCI_SCAN_ACTION_CLAIM_FUNC:
			break;
		case PCI_SCAN_ACTION_RELEASE_FUNC:
			pci_func_close(func);
			free(func);
			break;
		default:
			pci_func_close(func);
			free(func);
			err = -EIO;
			goto exit;
		}
	}

exit:
	closedir(dir);
	return err;
}
