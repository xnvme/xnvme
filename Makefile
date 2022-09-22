#
# xNVMe uses meson, however, to provide the conventional practice of:
#
# ./configure
# make
# make install
#
# The initial targets of this Makefile supports the behavior, instrumenting
# meson based on the options passed to "./configure". However, to do proper configuration then
# actually use meson, this is just here to get one started, showing some default usage of meson and
# providing pointers to where to read about meson.
#
ifeq ($(PLATFORM_ID),Windows)
else
PLATFORM_ID = $$( uname -s )
endif
PLATFORM = $$( \
	case $(PLATFORM_ID) in \
		( Linux | FreeBSD | OpenBSD | NetBSD | Windows | Darwin ) echo $(PLATFORM_ID) ;; \
		( * ) echo Unrecognized ;; \
	esac)

CTAGS = $$( \
	case $(PLATFORM_ID) in \
		( Linux | Darwin ) echo "ctags" ;; \
		( FreeBSD | OpenBSD | NetBSD | Windows) echo "exctags" ;; \
		( * ) echo Unrecognized ;; \
	esac)

MAKE = $$( \
	case $(PLATFORM_ID) in \
		( Linux | Windows | Darwin ) echo "make" ;; \
		( FreeBSD | OpenBSD | NetBSD ) echo "gmake" ;; \
		( * ) echo Unrecognized ;; \
	esac)

MESON = $$( \
	case $(PLATFORM_ID) in \
		( Linux | Windows | Darwin ) echo "meson" ;; \
		( FreeBSD | OpenBSD | NetBSD ) echo "meson" ;; \
		( * ) echo Unrecognized ;; \
	esac)

NPROC = $$( \
	case $(PLATFORM_ID) in \
		( Linux | Windows ) nproc ;; \
		( FreeBSD | OpenBSD | NetBSD | Darwin ) sysctl -n hw.ncpu ;; \
		( * ) echo Unrecognized ;; \
	esac)

GIT = $$( \
	case $(PLATFORM_ID) in \
		( Linux | Windows | Darwin ) echo "git" ;; \
		( FreeBSD | OpenBSD | NetBSD ) echo "git" ;; \
		( * ) echo Unrecognized ;; \
	esac)

BUILD_DIR?=builddir
TOOLBOX_DIR?=toolbox
PROJECT_VER = $(( python3 $(TOOLBOX_DIR)/xnvme_ver.py --path meson.build ))

define default-help
# invoke: 'make info', 'make tags', 'make git-setup', 'make config' and 'make build'
#
# The default behavior when invoking 'make', that is, dump out info relevant to the system,
# generate ctags, setup git-hoooks, configure the meson-build and start compiling
endef
.PHONY: default
default: info tags git-setup
	@echo "## xNVMe: make default"
	@if [ ! -d "$(BUILD_DIR)" ]; then $(MAKE) config; fi;
	$(MAKE) build

define config-help
# Configure Meson
endef
.PHONY: config
config:
	@echo "## xNVMe: make config"
	CC=$(CC) CXX=$(CXX) $(MESON) setup $(BUILD_DIR)
	@echo "## xNVMe: make config [DONE]"

define config-debug-help
# Configure Meson to compile xNVMe with '--buildtype=debug'
endef
.PHONY: config-debug
config-debug:
	@echo "## xNVMe: make config-debug"
	CC=$(CC) CXX=$(CXX) $(MESON) setup $(BUILD_DIR) --buildtype=debug
	@echo "## xNVMe: make config-debug [DONE]"

define config-uring-help
# Configure Meson to compile xNVMe with liburing as the only backend
endef
.PHONY: config-uring
config-uring:
	@echo "## xNVMe: config-uring"
	CC=$(CC) CXX=$(CXX) $(MESON) setup $(BUILD_DIR) \
	   -Dwith-spdk=false \
	   -Dwith-fio=false \
	   -Dwith-liburing=true
	@echo "## xNVMe: config-uring [DONE]"

define config-slim-help
# Configure Meson to compile xNVMe without subprojects (SPDK, fio, liburing)
endef
.PHONY: config-slim
config-slim:
	@echo "## xNVMe: make config-slim"
	CC=$(CC) CXX=$(CXX) $(MESON) setup $(BUILD_DIR) \
	   -Dwith-spdk=false \
	   -Dwith-fio=false \
	   -Dwith-liburing=false
	@echo "## xNVMe: make config-slim [DONE]"

define git-setup-help
# Do git config for: 'core.hooksPath' and 'blame.ignoreRevsFile'
endef
.PHONY: git-setup
git-setup:
	@echo "## xNVMe: git-setup"
	@./$(TOOLBOX_DIR)/pre-commit-check.sh;
	$(GIT) config blame.ignoreRevsFile .git-blame-ignore-revs || true
	@echo "## xNVMe: git-setup [DONE]"

define format-help
# run code format (style, code-conventions and language-integrity) on staged changes
endef
.PHONY: format
format:
	@echo "## xNVMe: format"
	@pre-commit run
	@echo "## xNVME: format [DONE]"

define format-all-help
# run code format (style, code-conventions and language-integrity) on staged and committed changes
endef
.PHONY: format-all
format-all:
	@echo "## xNVMe: format-all"
	@pre-commit run --all-files
	@echo "## xNVME: format-all [DONE]"

define info-help
# Print information relevant to xNVMe
#
# Such as:
# - OS
# - Project version
# - Versions of tools
endef
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
	@echo "PROJECT_VER: $(PROTECT_VER)"
	@echo "## xNVMe: make info [DONE]"

define build-help
# Build xNVMe with Meson
endef
.PHONY: build
build: info _require_builddir
	@echo "## xNVMe: make build"
	$(MESON) compile -C $(BUILD_DIR)
	@echo "## xNVMe: make build [DONE]"

define install-help
# Install xNVMe with Meson
endef
.PHONY: install
install: _require_builddir
	@echo "## xNVMe: make install"
	$(MESON) install -C $(BUILD_DIR)
	@echo "## xNVMe: make install [DONE]"

define uninstall-help
# Uninstall xNVMe with Meson
endef
.PHONY: uninstall
uninstall:
	@echo "## xNVMe: make uninstall"
	@if [ ! -d "$(BUILD_DIR)" ]; then		\
		echo "Missing builddir('$(BUILD_DIR))";	\
		false;					\
	fi
	cd $(BUILD_DIR) && $(MESON) --internal uninstall
	@echo "## xNVMe: make uninstall [DONE]"

define build-deb-help
# Build a binary DEB package
#
# NOTE: The binary DEB packages generated here are not meant to be used for anything
# but easy install/uninstall during test and development
endef
.PHONY: build-deb
build-deb: _require_builddir
	@echo "## xNVMe: make build-deb"
	rm -rf $(BUILD_DIR)/meson-dist/deb
	python3 $(TOOLBOX_DIR)/meson_dist_deb_build.py \
		--deb-description "xNVMe binary package for test/development" \
		--deb-maintainer "See GitHUB and https://xnvme.io/" \
		--builddir $(BUILD_DIR) \
		--workdir $(BUILD_DIR)/meson-dist/deb \
		--output $(BUILD_DIR)/meson-dist/xnvme.deb
	@echo "## xNVMe: make-deb [DONE]; find it here:"
	@find $(BUILD_DIR)/meson-dist/ -name '*.deb'

define install-deb-help
# Install the binary DEB package created with: make build-deb
#
# This will install it instead of copying it directly
# into the system paths. This is convenient as it is easier to purge it by
# running eg.
#   make uninstall-deb
endef
.PHONY: install-deb
install-deb: _require_builddir
	@echo "## xNVMe: make install-deb"
	dpkg -i $(BUILD_DIR)/meson-dist/xnvme.deb
	@echo "## xNVMe: make install-deb [DONE]"

define uninstall-deb-help
# Uninstall xNVMe with apt-get
endef
.PHONY: uninstall-deb
uninstall-deb:
	@echo "## xNVMe: make uninstall-deb"
	apt-get --yes remove xnvme* || true
	@echo "## xNVMe: make uninstall-deb [DONE]"

define clean-help
# Remove Meson builddir
endef
.PHONY: clean
clean:
	@echo "## xNVMe: make clean"
	rm -fr $(BUILD_DIR) || true
	@echo "## xNVMe: make clean [DONE]"

define clean-subprojects-help
# Remove Meson builddir as well as running 'make clean' in all subprojects 
endef
.PHONY: clean-subprojects
clean-subprojects:
	@echo "## xNVMe: make clean-subprojects"
	@if [ -d "subprojects/spdk" ] && [ ${PLATFORM_ID} != 'Darwin' ]; then cd subprojects/spdk &&$(MAKE) clean; fi;
	@if [ -d "subprojects/fio" ]; then cd subprojects/fio && $(MAKE) clean; fi;
	@if [ -d "subprojects/liburing" ]; then cd subprojects/liburing &&$(MAKE) clean; fi;
	rm -fr $(BUILD_DIR) || true
	@echo "## xNVMe: make clean-subprojects [DONE]"

define gen-libconf-help
# Helper-target generating third-party/subproject/os/library-configuration string
endef
.PHONY: gen-libconf
gen-libconf:
	@echo "## xNVMe: make gen-libconf"
	./$(TOOLBOX_DIR)/xnvme_libconf.py --repos .
	@echo "## xNVMe: make gen-libconf [DONE]"

define gen-src-archive-help
# Helper-target to produce full-source archive
endef
.PHONY: gen-src-archive
gen-src-archive:
	@echo "## xNVMe: make gen-src-archive"
	$(MESON) setup $(BUILD_DIR) -Dbuild_subprojects=false -Dwith-liburing=false
	$(MESON) dist -C $(BUILD_DIR) --include-subprojects --no-tests --formats gztar
	@echo "## xNVMe: make gen-src-archive [DONE]"

define gen-bash-completions-help
# Helper-target to produce Bash-completions for tools (tools, examples, tests)
#
# NOTE: This target requires a bunch of things: binaries must be built and
# residing in '$(BUILD_DIR)/tools' etc. AND installed on the system. Also, the find
# command has only been tried with GNU find
endef
.PHONY: gen-bash-completions
gen-bash-completions:
	@echo "## xNVMe: make gen-bash-completions"
	$(eval TOOLS := $(shell find $(BUILD_DIR)/tools $(BUILD_DIR)/examples $(BUILD_DIR)/tests -not -name "xnvme_single*" -not -name "xnvme_enum" -not -name "xnvme_dev" -not -name "*.so" -type f -executable -exec basename {} \;))
	python3 ./$(TOOLBOX_DIR)/xnvmec_generator.py cpl --tools ${TOOLS} --output $(TOOLBOX_DIR)/bash_completion.d
	@echo "## xNVMe: make gen-bash-completions [DONE]"

define gen-man-pages-help
# Helper-target to produce man pages for tools (tools, examples, tests)
#
# NOTE: This target requires a bunch of things: binaries must be built and
# residing in '$(BUILD_DIR)/tools' etc. AND installed on the system. Also, the find
# command has only been tried with GNU find
endef
.PHONY: gen-man-pages
gen-man-pages:
	@echo "## xNVMe: make gen-man-pages"
	$(eval TOOLS := $(shell find $(BUILD_DIR)/tools $(BUILD_DIR)/examples $(BUILD_DIR)/tests -not -name "xnvme_single*" -not -name "xnvme_enum" -not -name "xnvme_dev" -not -name "*.so" -type f -executable -exec basename {} \;))
	python3 ./$(TOOLBOX_DIR)/xnvmec_generator.py man --tools ${TOOLS} --output man/
	@echo "## xNVMe: make gen-man-pages [DONE]"

define tags-help
# Helper-target to produce tags
endef
.PHONY: tags
tags:
	@echo "## xNVMe: make tags"
	@$(CTAGS) * --languages=C -h=".c.h" -R --exclude=$(BUILD_DIR) \
		include \
		lib \
		tools \
		examples \
		tests \
		subprojects/* \
		|| true
	@echo "## xNVMe: make tags [DONE]"

define tags-xnvme-public-help
# Make tags for public APIs
endef
.PHONY: tags-xnvme-public
tags-xnvme-public:
	@echo "## xNVMe: make tags-xnvme-public"
	@$(CTAGS) --c-kinds=dfpgs -R include/lib*.h || true
	@echo "## xNVMe: make tags-xnvme-public [DONE]"

.PHONY: _require_builddir
_require_builddir:
	@if [ ! -d "$(BUILD_DIR)" ]; then				\
		echo "";						\
		echo "+========================================+";	\
		echo "                                          ";	\
		echo " ERR: missing builddir: '$(BUILD_DIR)'    ";	\
		echo "                                          ";	\
		echo " Configure it first by running:           ";	\
		echo "                                          ";	\
		echo " 'meson setup $(BUILD_DIR)'               ";	\
		echo "                                          ";	\
		echo "+========================================+";	\
		echo "";						\
		false;							\
	fi

define clobber-help
# invoke 'make clean' and: remove subproject builds, git-clean and git-checkout .
#
# This is intended as a way to clearing out the repository for any old "build-debris",
# take care that you have stashed/commit any of your local changes as anything unstaged
# will be removed.
endef
.PHONY: clobber
clobber: clean
	@echo "## xNVMe: clobber"
	@git clean -dfx
	@git clean -dfX
	@git checkout .
	@rm -rf subprojects/fio || true
	@rm -rf subprojects/spdk || true
	@rm -rf subprojects/liburing || true
	@echo "## xNVMe: clobber [DONE]"


define help-help
# Print the description of every target
endef
.PHONY: help
help:
	@./$(TOOLBOX_DIR)/print_help.py --repos .

define help-verbose-help
# Print the verbose description of every target
endef
.PHONY: help-verbose
help-verbose:
	@./$(TOOLBOX_DIR)/print_help.py --verbose --repos .
