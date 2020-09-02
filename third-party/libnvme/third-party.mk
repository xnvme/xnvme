#
# libnvme
#
XNVME_3P_LIBNVME_REPOS:=third-party/libnvme/repos

.PHONY: third-party-libnvme
third-party-libnvme:
	@echo "## xNVMe: make third-party-libnvme"
	@echo "# Preparing third-party (libnvme)"
	@if [ ! -f "${XNVME_3P_LIBNVME_REPOS}/README" ]; then	\
		$(MAKE) third-party-update;			\
	fi
	@$(MAKE) third-party-libnvme-clean
	@$(MAKE) third-party-libnvme-build

.PHONY: third-party-libnvme-clean
third-party-libnvme-clean:
	@echo "## xNVMe: make third-party-libnvme-clean"
	cd ${XNVME_3P_LIBNVME_REPOS} && ${MAKE} clean || true

.PHONY: third-party-libnvme-clobber
third-party-libnvme-clobber: third-party-libnvme-clean
	@echo "## xNVMe: make third-party-libnvme-clobber"
	cd ${XNVME_3P_LIBNVME_REPOS} && git clean -dfx || true
	cd ${XNVME_3P_LIBNVME_REPOS} && git clean -dfX || true
	cd ${XNVME_3P_LIBNVME_REPOS} && git checkout . || true

.PHONY: third-party-libnvme-build
third-party-libnvme-build:
	@echo "## xNVMe: make third-party-libnvme-build"
	cd ${XNVME_3P_LIBNVME_REPOS} && $(MAKE)
