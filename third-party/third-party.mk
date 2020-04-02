#
# THIRD-PARTY Libraries
#

.PHONY: third-party-clean
third-party-clean:
	@echo "## xNVMe: make third-party-clean"

.PHONY: third-party-clobber
third-party-clobber: third-party-clean
	@echo "## xNVMe: make third-party-clobber"

.PHONY: third-party-update
third-party-update:
	@echo "## xNVMe: third-party-update"
	@git submodule update --init --recursive
