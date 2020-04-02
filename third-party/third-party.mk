#
# THIRD-PARTY Libraries
#
include third-party/spdk.mk
include third-party/liburing.mk
include third-party/fio.mk
include third-party/libnvme.mk

.PHONY: third-party-clean
third-party-clean:
	@echo "## xNVMe: make third-party-clean"
	$(MAKE) third-party-spdk-clean
	$(MAKE) third-party-liburing-clean
	$(MAKE) third-party-fio-clean
	$(MAKE) third-party-libnvme-clean

.PHONY: third-party-clobber
third-party-clobber: third-party-clean
	@echo "## xNVMe: make third-party-clobber"
	$(MAKE) third-party-spdk-clobber
	$(MAKE) third-party-liburing-clobber
	$(MAKE) third-party-fio-clobber
	$(MAKE) third-party-libnvme-clobber

.PHONY: third-party-update
third-party-update:
	@echo "## xNVMe: third-party-update"
	@git submodule update --init --recursive
