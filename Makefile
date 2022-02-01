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

MESON = $$( \
	case $(PLATFORM_ID) in \
		( Linux | Windows) echo "meson" ;; \
		( FreeBSD | OpenBSD | NetBSD ) echo "meson" ;; \
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

PROJECT_VER = $(( python3 scripts/xnvme_ver.py --path meson.build ))
BUILD_DIR?=builddir

.PHONY: default
default: info tags git-setup
	@echo "## xNVMe: make default"
	@if [ ! -d "$(BUILD_DIR)" ]; then $(MAKE) config; fi;
	$(MAKE) build

.PHONY: config
config:
	@echo "## xNVMe: make config"
	CC=$(CC) CXX=$(CXX) $(MESON) setup $(BUILD_DIR)
	@echo "## xNVMe: make config [DONE]"

.PHONY: config-debug
config-debug:
	@echo "## xNVMe: make config-debug"
	CC=$(CC) CXX=$(CXX) $(MESON) setup $(BUILD_DIR) --buildtype=debug
	@echo "## xNVMe: make config-debug [DONE]"

.PHONY: config-uring
config-uring:
	@echo "## xNVMe: config-uring"
	CC=$(CC) CXX=$(CXX) $(MESON) setup $(BUILD_DIR) \
	   -Dwith-spdk=false \
	   -Dwith-fio=false \
	   -Dwith-liburing=true
	@echo "## xNVMe: config-uring [DONE]"

.PHONY: config-slim
config-slim:
	@echo "## xNVMe: make config-slim"
	CC=$(CC) CXX=$(CXX) $(MESON) setup $(BUILD_DIR) \
	   -Dwith-spdk=false \
	   -Dwith-fio=false \
	   -Dwith-liburing=false
	@echo "## xNVMe: make config-slim [DONE]"

.PHONY: git-setup
git-setup:
	@echo "## xNVMe:: git-setup"
	$(GIT) config core.hooksPath .githooks || true
	@echo "## xNVMe:: git-setup [DONE]"

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

.PHONY: build
build: info _require_builddir
	@echo "## xNVMe: make build"
	$(MESON) compile -C $(BUILD_DIR)
	@echo "## xNVMe: make build [DONE]"

.PHONY: install
install: _require_builddir
	@echo "## xNVMe: make install"
	$(MESON) install -C $(BUILD_DIR)
	@echo "## xNVMe: make install [DONE]"

.PHONY: uninstall
uninstall: 
	@echo "## xNVMe: make install"
	@if [ ! -d "$(BUILD_DIR)" ]; then		\
		echo "Missing builddir('$(BUILD_DIR))";	\
		false;					\
	fi
	cd $(BUILD_DIR) && $(MESON) --internal uninstall
	@echo "## xNVMe: make install [DONE]"

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
.PHONY: build-deb
build-deb: _require_builddir
	@echo "## xNVMe: make build-deb"
	rm -rf $(BUILD_DIR)/meson-dist/deb
	python3 scripts/meson_dist_deb_build.py \
		--deb-description "xNVMe binary package for test/development" \
		--deb-maintainer "See GitHUB and https://xnvme.io/" \
		--builddir $(BUILD_DIR) \
		--workdir $(BUILD_DIR)/meson-dist/deb \
		--output $(BUILD_DIR)/meson-dist/xnvme.deb
	@echo "## xNVMe: make-deb [DONE]; find it here:"
	@find $(BUILD_DIR)/meson-dist/ -name '*.deb'

.PHONY: install-deb
install-deb: _require_builddir
	@echo "## xNVMe: make install-deb"
	dpkg -i $(BUILD_DIR)/meson-dist/xnvme.deb
	@echo "## xNVMe: make install-deb [DONE]"

.PHONY: uninstall-deb
uninstall-deb:
	@echo "## xNVMe: make uninstall-deb"
	apt-get --yes remove xnvme* || true
	@echo "## xNVMe: make uninstall-deb [DONE]"

.PHONY: clean
clean:
	@echo "## xNVMe: make clean"
	rm -fr $(BUILD_DIR) || true
	@echo "## xNVMe: make clean [DONE]"

.PHONY: clean-subprojects
clean-subprojects:
	@echo "## xNVMe: make clean"
	@if [ -d "subprojects/spdk" ]; then cd subprojects/spdk &&$(MAKE) clean; fi;
	@if [ -d "subprojects/fio" ]; then cd subprojects/fio && $(MAKE) clean; fi;
	@if [ -d "subprojects/liburing" ]; then cd subprojects/liburing &&$(MAKE) clean; fi;
	rm -fr $(BUILD_DIR) || true
	@echo "## xNVMe: make clean [DONE]"

#
# Helper-target generating third-party strings
#
.PHONY: gen-3p-ver
gen-3p-ver:
	@echo "## xNVMe: make gen-3p-ver"
	./scripts/xnvme_3p.py --repos .
	@echo "## xNVMe: make gen-3p-ver [DONE]"


#
# Helper-target to produce full-source archive
#
.PHONY: gen-src-archive
gen-src-archive:
	@echo "## xNVMe: make gen-src-archive"
	$(MESON) setup $(BUILD_DIR) -Dbuild_subprojects=false
	$(MESON) dist -C $(BUILD_DIR) --include-subprojects --no-tests --formats gztar
	@echo "## xNVMe: make gen-src-archive [DONE]"

#
# Helper-target to produce Bash-completions for tools (tools, examples, tests)
#
# NOTE: This target requires a bunch of things: binaries must be built and
# residing in 'builddir/tools' etc. AND installed on the system. Also, the find
# command has only been tried with GNU find
#
.PHONY: gen-bash-completions
gen-bash-completions:
	@echo "## xNVMe: make gen-bash-completions"
	$(eval TOOLS := $(shell find builddir/tools builddir/examples builddir/tests -not -name "xnvme_single*" -not -name "xnvme_enum" -not -name "xnvme_dev" -not -name "*.so" -type f -executable -exec basename {} \;))
	python3 ./scripts/xnvmec_generator.py cpl --tools ${TOOLS} --output scripts/bash_completion.d
	@echo "## xNVMe: make gen-bash-completions [DONE]"

#
# Helper-target to produce man pages for tools (tools, examples, tests)
#
# NOTE: This target requires a bunch of things: binaries must be built and
# residing in 'builddir/tools' etc. AND installed on the system. Also, the find
# command has only been tried with GNU find
#
.PHONY: gen-man-pages
gen-man-pages:
	@echo "## xNVMe: make gen-man-pages"
	$(eval TOOLS := $(shell find builddir/tools builddir/examples builddir/tests -not -name "xnvme_single*" -not -name "xnvme_enum" -not -name "xnvme_dev" -not -name "*.so" -type f -executable -exec basename {} \;))
	python3 ./scripts/xnvmec_generator.py man --tools ${TOOLS} --output man/
	@echo "## xNVMe: make gen-man-pages [DONE]"

#
# Helper-target to produce tags
#
.PHONY: tags
tags:
	@echo "## xNVMe: make tags"
	@$(CTAGS) * --languages=C -h=".c.h" -R --exclude=builddir \
		include \
		lib \
		tools \
		examples \
		tests \
		subprojects/* \
		|| true
	@echo "## xNVMe: make tags [DONE]"

.PHONY: tags-xnvme-public
tags-xnvme-public:
	@echo "## xNVMe: make tags for public APIs"
	@$(CTAGS) --c-kinds=dfpgs -R include/lib*.h || true
	@echo "## xNVMe: make tags for public APIs [DONE]"

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

#
# clobber: clean third-party builds, drop any third-party changes and clean any
# untracked stuff lying around
#
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
