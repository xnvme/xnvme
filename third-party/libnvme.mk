#
# libnvme
#
.PHONY: third-party-libnvme
third-party-libnvme:
	@echo "## xNVMe: make third-party-libnvme"
	@echo "# Preparing third-party (libnvme)"
	@if [ ! -f "third-party/libnvme/README" ]; then	\
		$(MAKE) third-party-update;		\
	fi
	@$(MAKE) third-party-libnvme-clean
	@$(MAKE) third-party-libnvme-build

.PHONY: third-party-libnvme-clean
third-party-libnvme-clean:
	@echo "## xNVMe: make third-party-libnvme-clean"
	cd third-party/libnvme && ${MAKE} clean || true

.PHONY: third-party-libnvme-clobber
third-party-libnvme-clobber: third-party-libnvme-clean
	@echo "## xNVMe: make third-party-libnvme-clobber"
	cd third-party/libnvme && git clean -dfx || true
	cd third-party/libnvme && git clean -dfX || true
	cd third-party/libnvme && git checkout . || true

.PHONY: third-party-libnvme-build
third-party-libnvme-build:
	@echo "## xNVMe: make third-party-libnvme-build"
	cd third-party/libnvme && $(MAKE)
