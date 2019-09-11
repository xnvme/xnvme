.PHONY: third-party-spdk
third-party-spdk:
	@echo "## xNVMe: make third-party-spdk"
	@echo "# Preparing third-party (SPDK/DPDK)"
	@if [ ! -d "third-party/spdk/dpdk" ]; then	\
		$(MAKE) third-party-fetch;		\
	fi
	@$(MAKE) third-party-spdk-clean
	@$(MAKE) third-party-spdk-patch
	@$(MAKE) third-party-spdk-configure
	@$(MAKE) third-party-spdk-build

.PHONY: third-party-spdk-clean
third-party-spdk-clean:
	@echo "## xNVMe: make third-party-spdk-clean"
	cd third-party/spdk && ${MAKE} clean || true

.PHONY: third-party-spdk-clobber
third-party-spdk-clobber: third-party-spdk-clean
	@echo "## xNVMe: make third-party-spdk-clobber"
	cd third-party/spdk/dpdk && git checkout . || true
	cd third-party/spdk/dpdk && git clean -dfx || true
	cd third-party/spdk/dpdk && git clean -dfX || true
	cd third-party/spdk && git clean -dfx || true
	cd third-party/spdk && git clean -dfX || true
	cd third-party/spdk && git checkout . || true

.PHONY: third-party-spdk-patch
third-party-spdk-patch:
	@echo "## xNVMe: make third-party-spdk-patch"
	@echo "# Patching DPDK to NOT use dynamic linking / plugins"
	@cd third-party/spdk/dpdk && patch -p1 --forward < ../../patches/dpdk-no-plugins.patch || true

.PHONY: third-party-spdk-configure
third-party-spdk-configure:
	@echo "## xNVMe: make third-party-spdk-configure"
	@echo "# Configuring SPDK"
	cd third-party/spdk && ./configure	\
		--without-fio			\
		--without-isal			\
		--without-iscsi-initiator	\
		--without-ocf			\
		--without-pmdk			\
		--without-rbd			\
		--without-reduce		\
		--without-shared		\
		--without-uring			\
		--without-vhost			\
		--without-virtio		\
		--without-vpp

.PHONY: third-party-spdk-build
third-party-spdk-build:
	@echo "## xNVMe: make third-party-spdk-build"
	@echo "# Installing SPDK dependencies"
	@echo "# Building SPDK"
	@[ ! -d "third-party/spdk/build" ] && mkdir -p "third-party/spdk/build/lib" || true
	cd third-party/spdk && $(MAKE)
