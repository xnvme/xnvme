#
# THIRD-PARTY Libraries
#
include third-party/spdk.mk
include third-party/liburing.mk

.PHONY: third-party-clean
third-party-clean:
	@echo "## xNVMe: make third-party-clean"
	$(MAKE) third-party-spdk-clean
	$(MAKE) third-party-liburing-clean

.PHONY: third-party-clobber
third-party-clobber: third-party-clean
	@echo "## xNVMe: make third-party-clobber"
	$(MAKE) third-party-spdk-clobber
	$(MAKE) third-party-liburing-clobber

.PHONY: third-party-update
third-party-update:
	@echo "## xNVMe: third-party-update"
	@git submodule update --init --recursive
