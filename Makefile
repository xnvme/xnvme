PLATFORM_ID = $$( uname -s )
PLATFORM = $$( \
	case $(PLATFORM_ID) in \
		( Linux | FreeBSD | OpenBSD | NetBSD ) echo $(PLATFORM_ID) ;; \
		( * ) echo Unrecognized ;; \
	esac)

CTAGS = $$( \
	case $(PLATFORM_ID) in \
		( Linux ) echo "ctags" ;; \
		( FreeBSD | OpenBSD | NetBSD ) echo "exctags" ;; \
		( * ) echo Unrecognized ;; \
	esac)

MAKE = $$( \
	case $(PLATFORM_ID) in \
		( Linux ) echo "make" ;; \
		( FreeBSD | OpenBSD | NetBSD ) echo "gmake" ;; \
		( * ) echo Unrecognized ;; \
	esac)

NPROC = $$( \
	case $(PLATFORM_ID) in \
		( Linux ) nproc ;; \
		( FreeBSD | OpenBSD | NetBSD ) sysctl -n hw.ncpu ;; \
		( * ) echo Unrecognized ;; \
	esac)

PTARGET = $$( \
	case $(PLATFORM_ID) in \
		( Linux ) echo "linux" ;; \
		( FreeBSD | OpenBSD | NetBSD ) echo "freebsd" ;; \
		( * ) echo Unrecognized ;; \
	esac)

# TODO: fix this
BUILD_DIR?=build

#
# xnvme uses CMake, however, to provide the conventional practice of:
#
# ./configure
# make
# make install
#
# The initial targets of this Makefile supports the behavior, instrumenting
# CMake based on the options passed to "./configure"
#
# Additional targets come after these which modify CMAKE build-options and other
# common practices associated with libxnvme development.
#
.PHONY: default
default: info
	@echo "## xNVMe: make default"
	@$(MAKE) deps
	@if [ ! -d "$(BUILD_DIR)" ]; then $(MAKE) $(PTARGET); fi;
	$(MAKE) build

.PHONY: info
info:
	@echo "## xNVMe: make info"
	@echo "cc: $(CC)"
	@echo "platform: $(PLATFORM)"
	@echo "ptarget: $(PTARGET)"
	@echo "make: $(MAKE)"
	@echo "ctags: $(CTAGS)"
	@echo "nproc: $(NPROC)"

.PHONY: build
build: info
	@echo "## xNVMe: make build"
	@if [ ! -d "$(BUILD_DIR)" ]; then			\
		echo "Please run ./configure";			\
		echo "See ./configure --help for config options"\
		echo "";					\
		false;						\
	fi
	cd $(BUILD_DIR) && ${MAKE}
	@if [ -f "$(BUILD_DIR)/build_deb" ]; then	\
		cd $(BUILD_DIR) && ${MAKE} package;	\
	fi

.PHONY: install
install:
	@echo "## xNVMe: make install"
	cd $(BUILD_DIR) && ${MAKE} install

.PHONY: clean
clean:
	@echo "## xNVMe: make clean"
	rm -fr $(BUILD_DIR) || true
	rm -fr cmake-build-debug || true
	rm -f cscope.out || true
	rm -f tags || true

#
# THIRD-PARTY DEPENDENCIES
#

#
# SPDK
#
.PHONY: deps-spdk-clean
deps-spdk-clean:
	@echo "## xNVMe: make deps-spdk-clean"
	cd third-party/spdk && ${MAKE} clean || true

.PHONY: deps-spdk-build
deps-spdk-build:
	@echo "## xNVMe: make deps-spdk-build"
	@echo "# Installing SPDK dependencies"
	@echo "# Building SPDK"
	cd third-party/spdk && $(MAKE)

.PHONY: deps-spdk-configure
deps-spdk-configure:
	@echo "## xNVMe: make deps-spdk-configure"
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

.PHONY: deps-spdk-patch
deps-spdk-patch:
	@echo "## xNVMe: make deps-spdk-patch"
	@echo "# Patching DPDK to NOT use dynamic linking / plugins"
	@cd third-party/spdk/dpdk && patch -p1 --forward < ../../patches/dpdk-no-plugins.patch || true
	@cd third-party/spdk && patch -p1 --forward < ../patches/spdk-idfy-skip-nst-check.patch || true

.PHONY: deps-spdk-fetch
deps-spdk-fetch:
	@echo "## xNVMe: make deps-spdk-fetch"
	@echo "# deps-fetch: fetching SPDK/DPDK"
	@git submodule update --init --recursive
	@echo "# deps-fetch: checkout DPDK"
	@cd third-party/spdk/dpdk && git checkout .

# This target installs dependencies of dependencies onto the current system
.PHONY: deps-spdk-deps
deps-spdk-deps:
	echo "## xNVMe: make deps-spdk-deps"
	cd third-party/spdk && sudo ./scripts/pkgdep.sh

.PHONY: deps-clean
deps-clean:
	@echo "## xNVMe: make deps-clean"
	${MAKE} deps-spdk-clean || true
	${MAKE} deps-liburing-clean || true

.PHONY: deps-spdk
deps-spdk:
	@echo "## xNVMe: make deps-spdk"
	@echo "# Preparing deps (SPDK/DPDK)"
	@$(MAKE) deps-spdk-clean
	@if [ ! -d "third-party/spdk/dpdk" ]; then	\
		$(MAKE) deps-spdk-fetch;		\
	fi
	@$(MAKE) deps-spdk-patch
	@$(MAKE) deps-spdk-configure
	@$(MAKE) deps-spdk-build

#
# liburing
#
.PHONY: deps-liburing-clean
deps-liburing-clean:
	@echo "## xNVMe: make deps-liburing-clean"
	cd third-party/liburing && ${MAKE} clean || true

.PHONY: deps-liburing-build
deps-liburing-build:
	@echo "## xNVMe: make deps-liburing-build"
	@echo "# Building liburing"
	cd third-party/liburing && $(MAKE)

.PHONY: deps-liburing-fetch
deps-liburing-fetch:
	@echo "## xNVMe: make deps-liburing-fetch"
	@echo "# deps-fetch: fetching liburing"
	@git submodule update --init --recursive
	@echo "# deps-fetch: checkout liburing"
	@cd third-party/liburing && git checkout .

.PHONY: deps-liburing
deps-liburing:
	@echo "## xNVMe: make deps-liburing"
	@echo "# Preparing deps (liburing)"
	@$(MAKE) deps-liburing-clean
	@if [ ! -d "third-party/liburing" ]; then	\
		$(MAKE) deps-liburing-fetch;		\
	fi
	@$(MAKE) deps-liburing-build

.PHONY: deps-linux
deps-linux:
	@echo "## xNVMe: make deps-linux"
	@if [ ! -f third-party/liburing/src/liburing.a ]; then		\
		$(MAKE) deps-liburing;					\
	fi
	@if [ ! -f third-party/spdk/build/lib/libspdk_nvme.a ]; then 	\
		$(MAKE) deps-spdk;					\
	fi

.PHONY: deps-freebsd
deps-freebsd:
	@echo "## xNVMe: make deps-freebsd"
	@if [ ! -f third-party/spdk/build/lib/libspdk_nvme.a ]; then 	\
		$(MAKE) deps-spdk;					\
	fi

.PHONY: deps
deps:
	@echo "## xNVMe: make deps"
	@$(MAKE) deps-$(PTARGET)

#
# The targets below configure xNVMe, that is, they call "./configure"
#
# They are simply short-hands for configurations commonly used during
# development and when generating documentation (linux-nospdk)
#

.PHONY: linux
linux: clean
	@echo "## xNVMe: make linux"
	CC=$(CC) ./configure		\
		--disable-be-fioc	\
		--enable-be-lioc	\
		--enable-be-liou	\
		--enable-be-spdk	\
		--enable-tools		\
		--enable-examples	\
		--enable-tests		\
		--disable-debug		\
		--enable-debs

.PHONY: linux-dbg
linux-dbg: clean
	@echo "## xNVMe: make linux-dbg"
	CC=$(CC) ./configure		\
		--disable-be-fioc	\
		--enable-be-lioc	\
		--enable-be-liou	\
		--enable-be-spdk	\
		--enable-tools		\
		--enable-examples	\
		--enable-tests		\
		--enable-debug		\
		--enable-debs

.PHONY: freebsd
freebsd: clean
	@echo "## xNVMe: make freebsd"
	CC=$(CC) ./configure		\
		--enable-be-fioc	\
		--disable-be-lioc	\
		--disable-be-liou	\
		--enable-be-spdk	\
		--enable-tools		\
		--enable-examples	\
		--enable-tests		\
		--disable-debug		\
		--disable-debs

.PHONY: freebsd-dbg
freebsd-dbg: clean
	@echo "## xNVMe: make freebsd-dbg"
	CC=$(CC) ./configure		\
		--enable-be-fioc	\
		--disable-be-lioc	\
		--disable-be-liou	\
		--enable-be-spdk	\
		--enable-tools		\
		--enable-examples	\
		--enable-tests		\
		--enable-debug		\
		--disable-debs

#
# The binary DEB packages generated here are not meant to be used for anything
# but easy install/uninstall during test and development
#
# make install-deb
#
# Which will build a deb pkg and install it instead of copying it directly
# into the system paths. This is convenient as it is easier to purge it by
# running e.g.
#
# make uninstall-deb
#
.PHONY: install-deb
install-deb:
	@echo "## xNVMe: make install-deb"
	dpkg -i $(BUILD_DIR)/*.deb

.PHONY: uninstall-deb
uninstall-deb:
	@echo "## xNVMe: make uninstall-deb"
	apt-get --yes remove xnvme-* || true

#
# Helper-target to produce full-source archive
#
.PHONY: gen-src-archive
gen-src-archive:
	@echo "## xNVMe: make gen-src-archive"
	./scripts/xnvme_gen_src_archive.sh

#
# Helper-target to produce Bash-completions for tools (tools, examples, tests)
#
.PHONY: gen-bash-completions
gen-bash-completions:
	@echo "## xNVMe: make gen-bash-completions"
	python ./scripts/xnvmec_generator.py cpl	\
		--tools					\
			lblk				\
			nvmec				\
			xnvme				\
			zoned				\
			xnvme_hello			\
			xnvme_io_async			\
			zoned_io_async			\
			zoned_io_sync			\
			xnvme_tests_ident		\
			xnvme_tests_async_intf		\
			xnvme_tests_lblk		\
			xnvme_tests_znd_explicit_open	\
			xnvme_tests_znd_zrwa		\
		--output scripts/bash_completion.d

#
# Helper-target to produce man pages for tools (tools, examples, tests)
#
.PHONY: gen-man-pages
gen-man-pages:
	@echo "## xNVMe: make gen-man-pages"
	python ./scripts/xnvmec_generator.py man	\
		--tools					\
			lblk				\
			nvmec				\
			xnvme				\
			zoned				\
			xnvme_hello			\
			xnvme_io_async			\
			zoned_io_async			\
			zoned_io_sync			\
			xnvme_tests_ident		\
			xnvme_tests_async_intf		\
			xnvme_tests_lblk		\
			xnvme_tests_znd_explicit_open	\
			xnvme_tests_znd_zrwa		\
		--output man/

#
# Helper-target to produce tag and scope files for your editor
#
.PHONY: tags
tags:
	@echo "## xNVMe: make tags"
	@echo "Creating tags"
	@$(CTAGS) * --languages=C -h=".c.h" -R --exclude=build include src tools examples tests third-party/liburing third-party/spdk
	@#@echo "Creating cscope"
	@#@cscope -b `find include src tools examples tests third-party/liburing third-party/spdk -name '*.[ch]'`

.PHONY: tags-xnvme-public
tags-xnvme-public:
	@echo "## xNVMe: make tags for public interface"
	@echo "Creating tags"
	@$(CTAGS) --c-kinds=dfpgs -R include/lib*.h
