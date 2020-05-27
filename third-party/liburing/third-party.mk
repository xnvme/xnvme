#
# liburing
#
XNVME_3P_LIBURING_REPOS:=third-party/liburing/repos

.PHONY: third-party-liburing
third-party-liburing:
	@echo "## xNVMe: make third-party-liburing"
	@echo "# Preparing third-party (liburing)"
	@if [ ! -f "${XNVME_3P_LIBURING_REPOS}/README" ]; then	\
		$(MAKE) third-party-liburing-fetch;		\
	fi
	@$(MAKE) third-party-liburing-clean
	@$(MAKE) third-party-liburing-build

.PHONY: third-party-liburing-clean
third-party-liburing-clean:
	@echo "## xNVMe: make third-party-liburing-clean"
	cd ${XNVME_3P_LIBURING_REPOS} && ${MAKE} clean || true

.PHONY: third-party-liburing-clobber
third-party-liburing-clobber: third-party-liburing-clean
	@echo "## xNVMe: make third-party-liburing-clobber"
	cd ${XNVME_3P_LIBURING_REPOS} && git clean -dfx || true
	cd ${XNVME_3P_LIBURING_REPOS} && git clean -dfX || true
	cd ${XNVME_3P_LIBURING_REPOS} && git checkout . || true

.PHONY: third-party-liburing-build
third-party-liburing-build:
	@echo "## xNVMe: make third-party-liburing-build"
	cd ${XNVME_3P_LIBURING_REPOS} && ./configure
	cd ${XNVME_3P_LIBURING_REPOS} && $(MAKE) -C src CFLAGS="$(CFLAGS) -fPIC"
	cd ${XNVME_3P_LIBURING_REPOS}/src && ln -s liburing.so.1.0.6 liburing.so
