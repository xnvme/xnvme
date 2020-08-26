#
# THIRD-PARTY Libraries
#
include third-party/fio/third-party.mk
include third-party/libnvme/third-party.mk
include third-party/liburing/third-party.mk
include third-party/spdk/third-party.mk

.PHONY: third-party-clean
third-party-clean:
	@echo "## xNVMe: make third-party-clean"
	$(MAKE) third-party-fio-clean
	$(MAKE) third-party-libnvme-clean
	$(MAKE) third-party-liburing-clean
	$(MAKE) third-party-spdk-clean

.PHONY: third-party-clobber
third-party-clobber: third-party-clean
	@echo "## xNVMe: make third-party-clobber"
	$(MAKE) third-party-fio-clobber
	$(MAKE) third-party-libnvme-clobber
	$(MAKE) third-party-liburing-clobber
	$(MAKE) third-party-spdk-clobber

.PHONY: third-party-update
third-party-update:
	@echo "## xNVMe: third-party-update"
	@git submodule update --init --recursive
