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
# SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
#
# SPDX-License-Identifier: BSD-3-Clause

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

# This is done to avoid appleframework deprecation warnings
export MACOSX_DEPLOYMENT_TARGET=11.0

BUILD_DIR?=builddir
TOOLBOX_DIR?=toolbox
CIJOE_GUEST_FILE := cijoe/current.guest
PROJECT_VER = $$( python3 $(TOOLBOX_DIR)/xnvme_ver.py --path meson.build )

TRANSPORT ?= pcie

ALLOW_DIRTY ?= 0

define default-help
# Invoke: 'make help'
endef
.PHONY: default
default: help

define common-help
# Invokes: make info git-setup clean config build tags
#
# The common behavior when invoking 'make', that is, dump out info relevant to
# the system, generate ctags, setup git-hoooks, configure the meson-build and
# start compiling
endef
.PHONY: common
common: info tags git-setup clean config build

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
	CC=$(CC) CXX=$(CXX) $(MESON) setup $(BUILD_DIR) \
		 --buildtype=debug
	@echo "## xNVMe: make config-debug [DONE]"

define config-slim-help
# Configure Meson to compile xNVMe without any third-party libraries
endef
.PHONY: config-slim
config-slim:
	@echo "## xNVMe: make config-slim"
	CC=$(CC) CXX=$(CXX) $(MESON) setup $(BUILD_DIR) \
	   -Dwith-spdk=disabled \
		 -Dwith-liburing=disabled \
		 -Dwith-libvfn=disabled \
		 -Dwith-libaio=disabled \
		 -Dwith-isal=disabled
	@echo "## xNVMe: make config-slim [DONE]"

define docs-help
# Build docs using the new tooling (ctags, doxygen, apigen, toolchain, sphinx)
endef
.PHONY: docs
docs: tags-xnvme-public
	@echo "## xNVMe: make docs"
	xnvme-docs-clean
	mkdir -p docs/builddir/doxy
	cd docs/tooling && doxygen doxy.cfg
	xnvme-docs-apigen --tags tags --output docs/api
	xnvme-docs-toolchain
	xnvme-docs-build-html
	@echo "## xNVMe: make docs [DONE]"

define docker-help
# Drop into a privileged docker instance with the repository bind-mounted at /tmp/xnvme
endef
.PHONY: docker
docker:
	@echo "## xNVMe: docker"
	mkdir -p /tmp/artifacts
	docker run -it --privileged -w /tmp/xnvme --mount type=bind,source="$(shell pwd)",target=/tmp/xnvme --mount type=bind,source=/tmp/artifacts,target=/tmp/artifacts ghcr.io/safl/nosi/ubuntu-2604-docker:latest bash
	@echo "## xNVME: docker [DONE]"

define cijoe-install-help
# Install CIJOE and dependencies via pipx (used by CI and guest-env)
endef
.PHONY: cijoe-install
cijoe-install:
	@echo "## xNVMe: cijoe-install"
	pipx install cijoe/ --include-deps --force
	@echo "## xNVMe: cijoe-install [DONE]"

define guest-env-help
# Setup guest environment: install CIJOE, check QEMU, select guest
endef
.PHONY: guest-env
guest-env: cijoe-install
	@echo "## xNVMe: guest-env"
	@python3 $(TOOLBOX_DIR)/dev_setup_check.py
	@python3 $(TOOLBOX_DIR)/guest_select.py cijoe/
	@echo "## xNVMe: guest-env [DONE]"

define guest-select-help
# Interactive menu to select the active QEMU guest (or pipe: echo debian-trixie | make guest-select)
endef
.PHONY: guest-select
guest-select:
	@python3 $(TOOLBOX_DIR)/guest_select.py cijoe/

define guest-start-help
# Kill, initialize, and start the QEMU guest (reads from cijoe/current.guest)
endef
.PHONY: guest-start
guest-start:
	@if [ ! -f "$(CIJOE_GUEST_FILE)" ]; then \
		echo ""; \
		echo "+=============================================+"; \
		echo "                                               "; \
		echo " ERR: No guest selected                        "; \
		echo "                                               "; \
		echo " Run: make guest-select                        "; \
		echo "                                               "; \
		echo "+=============================================+"; \
		echo ""; \
		false; \
	fi
	$(eval GUEST := $(shell cat $(CIJOE_GUEST_FILE)))
	@echo "## xNVMe: guest-start ($(GUEST))"
	cd cijoe && cijoe "workflows/provision-using-tgz.yaml" \
		--monitor \
		--config "configs/$(GUEST).toml" \
		--config "configs/fio.toml" \
		--config "configs/xnvme.toml" \
		--config "configs/system_imaging.toml" \
		--output "$(if $(CIJOE_OUTPUT),$(CIJOE_OUTPUT)-guest-start,cijoe-output-guest-start)" \
		guest_kill \
		stage_nosi_guest \
		guest_initialize \
		guest_start \
		root_unlock \
		guest_check
	@echo "## xNVMe: guest-start [DONE]"

define guest-provision-help
# Sync source, build, and install xNVMe inside the guest (reads from cijoe/current.guest)
#
# Requires: /tmp/artifacts/xnvme-src.tar.gz (run: make gen-artifacts ALLOW_DIRTY=1)
endef
.PHONY: guest-provision
guest-provision:
	@if [ ! -f "$(CIJOE_GUEST_FILE)" ]; then \
		echo ""; \
		echo "+=============================================+"; \
		echo "                                               "; \
		echo " ERR: No guest selected                        "; \
		echo "                                               "; \
		echo " Run: make guest-select                        "; \
		echo "                                               "; \
		echo "+=============================================+"; \
		echo ""; \
		false; \
	fi
	$(eval GUEST := $(shell cat $(CIJOE_GUEST_FILE)))
	@if [ ! -f /tmp/artifacts/xnvme-src.tar.gz ]; then \
		echo ""; \
		echo "+=============================================+"; \
		echo "                                               "; \
		echo " ERR: /tmp/artifacts/xnvme-src.tar.gz missing  "; \
		echo "                                               "; \
		echo " Run: make gen-artifacts ALLOW_DIRTY=1         "; \
		echo "                                               "; \
		echo "+=============================================+"; \
		echo ""; \
		false; \
	fi
	@echo "## xNVMe: guest-provision ($(GUEST))"
	cd cijoe && cijoe "workflows/provision-using-tgz.yaml" \
		--monitor \
		--config "configs/$(GUEST).toml" \
		--config "configs/fio.toml" \
		--config "configs/xnvme.toml" \
		--output "$(if $(CIJOE_OUTPUT),$(CIJOE_OUTPUT)-guest-provision,cijoe-output-guest-provision)" \
		-l \
		xnvme_source_sync \
		xnvme_build_prep \
		xnvme_build \
		xnvme_install \
		ldconfig \
		stabilize_nvme_naming \
		xnvme_bindings_py_install_tgz \
		fio_prep
	@echo "## xNVMe: guest-provision [DONE]"

define guest-test-help
# Run the CIJOE test workflow for the selected guest (reads from cijoe/current.guest)
#
# Optional: CIJOE_OUTPUT=<prefix> for CI artifact naming
endef
.PHONY: guest-test
guest-test:
	@if [ ! -f "$(CIJOE_GUEST_FILE)" ]; then \
		echo ""; \
		echo "+=============================================+"; \
		echo "                                               "; \
		echo " ERR: No guest selected                        "; \
		echo "                                               "; \
		echo " Run: make guest-select                        "; \
		echo "                                               "; \
		echo "+=============================================+"; \
		echo ""; \
		false; \
	fi
	$(eval GUEST := $(shell cat $(CIJOE_GUEST_FILE)))
	@echo "## xNVMe: guest-test ($(GUEST), transport=$(TRANSPORT))"
	cd cijoe && cijoe "workflows/test-$(GUEST)-$(TRANSPORT).yaml" \
		--monitor \
		--config "configs/$(GUEST).toml" \
		--config "configs/fio.toml" \
		--config "configs/xnvme.toml" \
		--output "$(if $(CIJOE_OUTPUT),$(CIJOE_OUTPUT)-test-results,cijoe-output-guest-test)"
	@echo "## xNVMe: guest-test [DONE]"

define guest-stop-help
# Stop (kill) the QEMU guest (reads from cijoe/current.guest)
endef
.PHONY: guest-stop
guest-stop:
	@if [ ! -f "$(CIJOE_GUEST_FILE)" ]; then \
		echo ""; \
		echo "+=============================================+"; \
		echo "                                               "; \
		echo " ERR: No guest selected                        "; \
		echo "                                               "; \
		echo " Run: make guest-select                        "; \
		echo "                                               "; \
		echo "+=============================================+"; \
		echo ""; \
		false; \
	fi
	$(eval GUEST := $(shell cat $(CIJOE_GUEST_FILE)))
	@echo "## xNVMe: guest-stop ($(GUEST))"
	cd cijoe && cijoe "workflows/provision-using-tgz.yaml" \
		--monitor \
		--config "configs/$(GUEST).toml" \
		--config "configs/fio.toml" \
		--config "configs/xnvme.toml" \
		--config "configs/system_imaging.toml" \
		--output "$(if $(CIJOE_OUTPUT),$(CIJOE_OUTPUT)-guest-stop,cijoe-output-guest-stop)" \
		guest_kill
	@echo "## xNVMe: guest-stop [DONE]"

define git-setup-help
# Do git config for: 'core.hooksPath' and 'blame.ignoreRevsFile'
endef
.PHONY: git-setup
git-setup:
	@echo "## xNVMe: git-setup"
	./$(TOOLBOX_DIR)/pre-commit-check.sh;
	$(GIT) config blame.ignoreRevsFile .git-blame-ignore-revs || true
	@echo "## xNVMe: git-setup [DONE]"

define format-help
# Run code format (style, code-conventions and language-integrity) on staged changes
endef
.PHONY: format
format:
	@echo "## xNVMe: format"
	pre-commit run
	@echo "## xNVME: format [DONE]"

define format-all-help
# Run code format (style, code-conventions and language-integrity) on staged and committed changes
endef
.PHONY: format-all
format-all:
	@echo "## xNVMe: format-all"
	pre-commit run --all-files
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
	@echo "PLATFORM: $(PLATFORM)"
	@echo "CC: $(CC)"
	@echo "CXX: $(CXX)"
	@echo "MAKE: $(MAKE)"
	@echo "CTAGS: $(CTAGS)"
	@echo "NPROC: $(NPROC)"
	@echo "PROJECT_VER: $(PROJECT_VER)"
	@echo "## xNVMe: make info [DONE]"

define build-help
# Build xNVMe with Meson
endef
.PHONY: build
build: info _require_builddir
	@echo "## xNVMe: make build"
	$(MESON) compile -C $(BUILD_DIR)
	@echo "## xNVMe: make build [DONE]"

define verify-ramdisk-help
# Build, install, prepare fio, and run the CIJOE ramdisk test suite
endef
.PHONY: verify-ramdisk
verify-ramdisk:
	@if [ "$$(id -u)" -ne 0 ]; then \
		echo ""; \
		echo "+=============================================+"; \
		echo "                                               "; \
		echo " ERR: 'make verify-ramdisk' requires root      "; \
		echo "                                               "; \
		echo " This target assumes a CI environment running  "; \
		echo " as root. Run it in CI or as root locally.     "; \
		echo "                                               "; \
		echo "+=============================================+"; \
		echo ""; \
		false; \
	fi
	@echo "## xNVMe: make verify-ramdisk"
	@if [ ! -d "$(BUILD_DIR)" ]; then \
		CC=$(CC) CXX=$(CXX) $(MESON) setup $(BUILD_DIR); \
	fi
	$(MESON) compile -C $(BUILD_DIR)
	$(MESON) install -C $(BUILD_DIR)
	ldconfig || true
	cd cijoe && cijoe fio_prep \
		--monitor \
		--config "configs/ramdisk-linux.toml" \
		--config "configs/fio.toml" \
		--config "configs/xnvme.toml" \
		$(if $(CIJOE_OUTPUT),--output "$(CIJOE_OUTPUT)-prep-fio")
	cd cijoe && cijoe "workflows/test-ramdisk.yaml" \
		--monitor \
		--config "configs/ramdisk-linux.toml" \
		--config "configs/fio.toml" \
		--config "configs/xnvme.toml" \
		$(if $(CIJOE_OUTPUT),--output "$(CIJOE_OUTPUT)-test-ramdisk")
	@echo "## xNVMe: make verify-ramdisk [DONE]"

define verify-guest-help
# Chain: guest-start + guest-provision + guest-test (reads from cijoe/current.guest)
#
# Requires: A custom QEMU environment (run: make docker)
# Requires: Guest selected via 'make guest-select'
# Requires: /tmp/artifacts/xnvme-src.tar.gz (run: make gen-artifacts ALLOW_DIRTY=1)
# Optional: CIJOE_OUTPUT=<prefix> for CI artifact naming
endef
.PHONY: verify-guest
verify-guest:
	@if [ ! -f /tmp/artifacts/xnvme-src.tar.gz ]; then \
		echo ""; \
		echo "+=============================================+"; \
		echo "                                               "; \
		echo " ERR: /tmp/artifacts/xnvme-src.tar.gz missing  "; \
		echo "                                               "; \
		echo " Run: make gen-artifacts ALLOW_DIRTY=1         "; \
		echo "                                               "; \
		echo "+=============================================+"; \
		echo ""; \
		false; \
	fi
	$(MAKE) guest-start guest-provision guest-test

define docgen-guest-help
# Provision a QEMU guest and generate documentation (reads from cijoe/current.guest)
#
# Requires: A custom QEMU environment (run: make docker)
# Requires: Guest selected via 'make guest-select'
# Requires: /tmp/artifacts/xnvme-src.tar.gz (run: make gen-artifacts ALLOW_DIRTY=1)
# Optional: CIJOE_OUTPUT=<prefix> for CI artifact naming
endef
.PHONY: docgen-guest
docgen-guest: guest-start guest-provision
	$(eval GUEST := $(shell cat $(CIJOE_GUEST_FILE)))
	@echo "## xNVMe: docgen-guest ($(GUEST))"
	cd cijoe && cijoe "workflows/docgen.yaml" \
		--monitor \
		--config "configs/$(GUEST).toml" \
		--config "configs/fio.toml" \
		--config "configs/xnvme.toml" \
		--output "$(if $(CIJOE_OUTPUT),$(CIJOE_OUTPUT)-docs,cijoe-output-docgen)"
	@echo "## xNVMe: docgen-guest [DONE]"

define install-help
# Install xNVMe with Meson
endef
.PHONY: install
install: _require_builddir
	@echo "## xNVMe: make install"
	$(MESON) install -C $(BUILD_DIR)
	@echo "## xNVMe: make install [DONE]"

define test-help
# Test xNVMe with Meson
endef
.PHONY: test
test: _require_builddir
	@echo "## xNVMe: make test"
	$(MESON) test -C $(BUILD_DIR)
	@echo "## xNVMe: make test [DONE]"

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
	find $(BUILD_DIR)/meson-dist/ -name '*.deb'

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

define build-rpm-help
# Build a binary RPM package
endef
.PHONY: build-rpm
build-rpm: _require_builddir
	@echo "## xNVMe: make build-rpm"
	meson ${BUILD_DIR}
	git archive --format=tar HEAD > xnvme.tar
	tar rf xnvme.tar ${BUILD_DIR}/xnvme.spec
	gzip -f -9 xnvme.tar
	rpmbuild -ta xnvme.tar.gz -v --define "_topdir $(shell pwd)/${BUILD_DIR}/meson-dist" --define "_prefix /usr/local"
	@echo "## xNVMe: make build-rpm [DONE]; find it here:"
	find $(BUILD_DIR)/meson-dist/ -name '*.rpm'

define install-rpm-help
# Install the binary RPM package created with: make build-rpm
# This will install it instead of copying it directly
# into the system paths. This is convenient as it is easier to purge it by
# running eg.
#   make uninstall-rpm
endef
.PHONY: install-rpm
install-rpm: _require_builddir
	@echo "## xNVMe: make install-rpm"
	rpm -ivh $(shell find $(BUILD_DIR)/meson-dist/ -name '*.rpm')
	@echo "## xNVMe: make install-rpm [DONE]"

define uninstall-rpm-help
# Uninstall xNVMe with yum
endef
.PHONY: uninstall-rpm
uninstall-rpm:
	@echo "## xNVMe: make uninstall-rpm"
	yum remove -y xnvme* || true
	@echo "## xNVMe: make uninstall-rpm [DONE]"

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
	if [ -d "subprojects/spdk" ] && [ ${PLATFORM_ID} != 'Darwin' ]; then cd subprojects/spdk &&$(MAKE) clean; fi;
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
# Produce a source-archive (.tar.gz) without subproject-source
endef
.PHONY: gen-src-archive
gen-src-archive:
	@echo "## xNVMe: make gen-src-archive"
	$(MESON) setup $(BUILD_DIR) -Dbuild_subprojects=false -Dwith-liburing=disabled -Dwith-libvfn=disabled
	if [ $(ALLOW_DIRTY) -eq 1 ]; then \
		$(MESON) dist -C $(BUILD_DIR) --no-tests --formats gztar --allow-dirty; \
	else \
		$(MESON) dist -C $(BUILD_DIR) --no-tests --formats gztar; \
	fi
	@echo "## xNVMe: make gen-src-archive [DONE]"

define gen-src-archive-with-subprojects-help
# Produce a source-archive (.tar.gz) with subproject-source
endef
.PHONY: gen-src-archive-with-subprojects
gen-src-archive-with-subprojects:
	@echo "## xNVMe: make gen-src-archive-with-subprojects"
	$(MESON) setup $(BUILD_DIR) -Dbuild_subprojects=false -Dwith-liburing=disabled -Dwith-libvfn=disabled
	if [ $(ALLOW_DIRTY) -eq 1 ]; then \
		$(MESON) dist -C $(BUILD_DIR) --no-tests --formats gztar --include-subprojects --allow-dirty; \
	else \
		$(MESON) dist -C $(BUILD_DIR) --no-tests --formats gztar --include-subprojects; \
	fi
	@echo "## xNVMe: make gen-src-archive-with-subprojects [DONE]"

define gen-artifacts-help
# Generate artifacts in "/tmp/artifacts" for local guest provisioning
#
# Optional: ALLOW_DIRTY=1 to allow uncommitted changes
endef
.PHONY: gen-artifacts
gen-artifacts: gen-src-archive-with-subprojects
	@echo "## xNVMe: make gen-artifacts"
	cd python/bindings && make clean build
	mkdir -p /tmp/artifacts
	cp builddir/meson-dist/xnvme-$(PROJECT_VER).tar.gz /tmp/artifacts/xnvme-src.tar.gz
	cp python/bindings/dist/xnvme-$(PROJECT_VER).tar.gz /tmp/artifacts/xnvme-py-sdist.tar.gz
	ls -l /tmp/artifacts
	@echo "## xNVMe: make gen-artifacts [DONE]"

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
	python3 ./$(TOOLBOX_DIR)/xnvmec_generator.py cpl --output $(TOOLBOX_DIR)/bash_completion.d
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
	python3 ./$(TOOLBOX_DIR)/xnvmec_generator.py man --output man/
	@echo "## xNVMe: make gen-man-pages [DONE]"

define tags-help
# Helper-target to produce tags and compile-commands
endef
.PHONY: tags
tags:
	@echo "## xNVMe: make tags"
	if [ -d "$(BUILD_DIR)" ]; then cp $(BUILD_DIR)/compile_commands.json .; fi;
	$(CTAGS) * --languages=C -h=".c.h" -R --exclude=$(BUILD_DIR) \
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
	$(CTAGS) --c-kinds=dfpgs -R include/lib*.h || true
	@echo "## xNVMe: make tags-xnvme-public [DONE]"

define _require_builddir-help
# This helper is not intended to be invoked via the command-line-interface.
endef
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
# Invoke 'make clean' and: remove subproject builds, git-clean and git-checkout .
#
# This is intended as a way to clearing out the repository for any old "build-debris",
# take care that you have stashed/commit any of your local changes as anything unstaged
# will be removed.
endef
.PHONY: clobber
clobber: clean
	@echo "## xNVMe: clobber"
	git clean -dfx
	git clean -dfX
	git checkout .
	rm -rf subprojects/libvfn || true
	rm -rf subprojects/spdk || true
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
