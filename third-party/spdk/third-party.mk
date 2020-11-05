#
# SPDK
#
XNVME_3P_SPDK_REPOS:=third-party/spdk/repos

.PHONY: third-party-spdk
third-party-spdk:
	@echo "## xNVMe: make third-party-spdk"
	@echo "# Preparing third-party (SPDK/DPDK)"
	@if [ ! -d "${XNVME_3P_SPDK_REPOS}/dpdk" ]; then	\
		$(MAKE) third-party-update;		\
	fi
	@$(MAKE) third-party-spdk-clean
	@$(MAKE) third-party-spdk-patch
	@$(MAKE) third-party-spdk-configure
	@$(MAKE) third-party-spdk-build

.PHONY: third-party-spdk-clean
third-party-spdk-clean:
	@echo "## xNVMe: make third-party-spdk-clean"
	cd ${XNVME_3P_SPDK_REPOS} && ${MAKE} clean || true

.PHONY: third-party-spdk-clobber
third-party-spdk-clobber: third-party-spdk-clean
	@echo "## xNVMe: make third-party-spdk-clobber"
	cd ${XNVME_3P_SPDK_REPOS}/dpdk && git checkout . || true
	cd ${XNVME_3P_SPDK_REPOS}/dpdk && git clean -dfx || true
	cd ${XNVME_3P_SPDK_REPOS}/dpdk && git clean -dfX || true
	cd ${XNVME_3P_SPDK_REPOS} && git clean -dfx || true
	cd ${XNVME_3P_SPDK_REPOS} && git clean -dfX || true
	cd ${XNVME_3P_SPDK_REPOS} && git checkout . || true

.PHONY: third-party-spdk-patch
third-party-spdk-patch:
	@echo "## xNVMe: make third-party-spdk-patch"
	@echo "# Patching DPDK to NOT use dynamic linking / plugins"
	@cd ${XNVME_3P_SPDK_REPOS}/dpdk && patch -p1 --forward < ../../patches/spdk-dpdk-no-plugins.patch || true
	@cd ${XNVME_3P_SPDK_REPOS} && find ../patches -type f -name '0*.patch' -print0 | sort -z | xargs -t -0 -n 1 patch -p1 --forward -i || true

.PHONY: third-party-spdk-configure
third-party-spdk-configure:
	@echo "## xNVMe: make third-party-spdk-configure"
	@echo "# Configuring SPDK"
	cd ${XNVME_3P_SPDK_REPOS} && ./configure	\
		--with-fio=../../fio/repos/		\
		--without-isal				\
		--without-iscsi-initiator		\
		--without-ocf				\
		--without-pmdk				\
		--without-rbd				\
		--without-reduce			\
		--without-shared			\
		--without-uring				\
		--without-vhost				\
		--without-virtio

.PHONY: third-party-spdk-build
third-party-spdk-build:
	@echo "## xNVMe: make third-party-spdk-build"
	@echo "# Installing SPDK dependencies"
	@echo "# Building SPDK"
	@[ ! -d "${XNVME_3P_SPDK_REPOS}/build" ] && mkdir -p "${XNVME_3P_SPDK_REPOS}/build/lib" || true
	cd ${XNVME_3P_SPDK_REPOS} && $(MAKE)
