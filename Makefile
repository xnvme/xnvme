#
# xNVMe uses CMake, however, to provide the conventional practice of:
#
# ./configure
# make
# make install
#
# The initial targets of this Makefile supports the behavior, instrumenting
# CMake based on the options passed to "./configure"
#
# Additional targets come after these which modify CMAKE build-options and other
# common practices associated with xNVMe development
#
ifeq ($(PLATFORM_ID),Windows)
else
PLATFORM_ID = $$( uname -s )
endif
PLATFORM = $$( \
	case $(PLATFORM_ID) in \
		( Linux | FreeBSD | OpenBSD | NetBSD | Windows) echo $(PLATFORM_ID) ;; \
		( * ) echo Unrecognized ;; \
	esac)

CTAGS = $$( \
	case $(PLATFORM_ID) in \
		( Linux ) echo "ctags" ;; \
		( FreeBSD | OpenBSD | NetBSD | Windows) echo "exctags" ;; \
		( * ) echo Unrecognized ;; \
	esac)

MAKE = $$( \
	case $(PLATFORM_ID) in \
		( Linux | Windows) echo "make" ;; \
		( FreeBSD | OpenBSD | NetBSD ) echo "gmake" ;; \
		( * ) echo Unrecognized ;; \
	esac)

NPROC = $$( \
	case $(PLATFORM_ID) in \
		( Linux | Windows) nproc ;; \
		( FreeBSD | OpenBSD | NetBSD ) sysctl -n hw.ncpu ;; \
		( * ) echo Unrecognized ;; \
	esac)

GIT = $$( \
	case $(PLATFORM_ID) in \
		( Linux | Windows) echo "git" ;; \
		( FreeBSD | OpenBSD | NetBSD ) echo "git" ;; \
		( * ) echo Unrecognized ;; \
	esac)

# TODO: fix this
BUILD_DIR?=build

.PHONY: default
default: info tags git-setup
	@echo "## xNVMe: make default"
	@if [ ! -d "$(BUILD_DIR)" ]; then $(MAKE) config; fi;
	$(MAKE) build

.PHONY: config
config:
	@echo "## xNVMe: make configure"
	CC=$(CC) ./configure

.PHONY: config-debug
config-debug:
	@echo "## xNVMe: make configure"
	CC=$(CC) ./configure --enable-debug

.PHONY: git-setup
git-setup:
	@echo "## xNVMe:: git-setup"
	$(GIT) config core.hooksPath .githooks || true

.PHONY: git-update
git-update:
	@echo "## xNVMe:: git-setup"
	$(GIT) submodule update --init --recursive

.PHONY: info
info:
	@echo "## xNVMe: make info"
	@echo "OSTYPE: $(OSTYPE)"
	@echo "PLATFORM: $(PLATFORM)"
	@echo "CC: $(CC)"
	@echo "CXX: $(CXX)"
	@echo "MAKE: $(MAKE)"
	@echo "CTAGS: $(CTAGS)"
	@echo "NPROC: $(NPROC)"

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

.PHONY: clean
clean:
	@echo "## xNVMe: make clean"
	rm -fr $(BUILD_DIR) || true
	rm -fr cmake-build-debug || true

#
# Helper-target generating third-party strings
#
.PHONY: gen-3p-ver
gen-3p-ver:
	@echo "## xNVMe: make gen-3p-ver"
	./scripts/xnvme_3p.py --repos .

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
# NOTE: This target requires a bunch of things: binaries must be built and
# residing in 'build/tools' etc. AND installed on the system. Also, the find
# command has only been tried with GNU find
#
.PHONY: gen-bash-completions
gen-bash-completions:
	@echo "## xNVMe: make gen-bash-completions"
	$(eval TOOLS := $(shell find build/tools build/examples build/tests -not -name "xnvme_single*" -not -name "*.so" -type f -executable -exec basename {} \;))
	python3 ./scripts/xnvmec_generator.py cpl --tools ${TOOLS} --output scripts/bash_completion.d

#
# Helper-target to produce man pages for tools (tools, examples, tests)
#
# NOTE: This target requires a bunch of things: binaries must be built and
# residing in 'build/tools' etc. AND installed on the system. Also, the find
# command has only been tried with GNU find
#
.PHONY: gen-man-pages
gen-man-pages:
	@echo "## xNVMe: make gen-man-pages"
	$(eval TOOLS := $(shell find build/tools build/examples build/tests -not -name "xnvme_single*" -not -name "*.so" -type f -executable -exec basename {} \;))
	python3 ./scripts/xnvmec_generator.py man --tools ${TOOLS} --output man/

#
# Helper-target to produce tags
#
.PHONY: tags
tags:
	@echo "## xNVMe: make tags"
	@$(CTAGS) * --languages=C -h=".c.h" -R --exclude=build \
		include \
		src \
		tools \
		examples \
		tests \
		third-party/* \
		|| true

.PHONY: tags-xnvme-public
tags-xnvme-public:
	@echo "## xNVMe: make tags for public APIs"
	@$(CTAGS) --c-kinds=dfpgs -R include/lib*.h || true

#
# clobber: clean third-party builds, drop any third-party changes and clean any
# untracked stuff lying around
#
.PHONY: clobber
clobber: third-party-clobber clean
	@git clean -dfx
	@git clean -dfX
	@git checkout .

include third-party/third-party.mk
