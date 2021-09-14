#
# fio
#
XNVME_3P_FIO_REPOS:=third-party/fio/repos

.PHONY: third-party-fio
third-party-fio:
	@echo "## xNVMe: make third-party-fio"
	@echo "# Preparing third-party (fio)"
	@if [ ! -f "${XNVME_3P_FIO_REPOS}/fio.c" ]; then	\
		$(MAKE) third-party-update;			\
	fi
	@$(MAKE) third-party-fio-clean
	@$(MAKE) third-party-fio-patch
	@$(MAKE) third-party-fio-build

.PHONY: third-party-fio-clean
third-party-fio-clean:
	@echo "## xNVMe: make third-party-fio-clean"
	cd ${XNVME_3P_FIO_REPOS} && $(MAKE) clean || true

.PHONY: third-party-fio-clobber
third-party-fio-clobber: third-party-fio-clean
	@echo "## xNVMe: make third-party-fio-clobber"
	cd ${XNVME_3P_FIO_REPOS} && git clean -dfx || true
	cd ${XNVME_3P_FIO_REPOS} && git clean -dfX || true
	cd ${XNVME_3P_FIO_REPOS} && git checkout . || true

.PHONY: third-party-fio-patch
third-party-fio-patch:
	@echo "## xNVMe: make third-party-fio-patch"
	@cd ${XNVME_3P_FIO_REPOS} && find ../patches -type f -name '0*.patch' -print0 | sort -z | xargs -t -0 -n 1 patch -p1 --forward -i || true

.PHONY: third-party-fio-build
third-party-fio-build:
	@echo "## xNVMe: make third-party-fio-build"
	@if [ "$(OS)" == "Windows_NT" ]; then    \
		cd ${XNVME_3P_FIO_REPOS} && ./configure --cc=clang;    \
	fi
	cd ${XNVME_3P_FIO_REPOS} && $(MAKE)
