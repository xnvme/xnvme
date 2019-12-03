#
# liburing
#
.PHONY: third-party-liburing
third-party-liburing:
	@echo "## xNVMe: make third-party-liburing"
	@echo "# Preparing third-party (liburing)"
	@if [ ! -f "third-party/liburing/README" ]; then	\
		$(MAKE) third-party-liburing-fetch;		\
	fi
	@$(MAKE) third-party-liburing-clean
	@$(MAKE) third-party-liburing-build

.PHONY: third-party-liburing-clean
third-party-liburing-clean:
	@echo "## xNVMe: make third-party-liburing-clean"
	cd third-party/liburing && ${MAKE} clean || true

.PHONY: third-party-liburing-clobber
third-party-liburing-clobber: third-party-liburing-clean
	@echo "## xNVMe: make third-party-liburing-clobber"
	cd third-party/liburing && git clean -dfx || true
	cd third-party/liburing && git clean -dfX || true
	cd third-party/liburing && git checkout . || true

.PHONY: third-party-liburing-build
third-party-liburing-build:
	@echo "## xNVMe: make third-party-liburing-build"
	cd third-party/liburing && ./configure
	cd third-party/liburing && $(MAKE) -C src CFLAGS="$(CFLAGS) -fPIC"
	cd third-party/liburing/src && ln -s liburing.so.1.0.6 liburing.so
