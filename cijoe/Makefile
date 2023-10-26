#
# This Makefile serves as convenient command-line auto-completion
#
PROJECT_NAME=cijoe-for-xnvme
PYTEST=pytest
PYTHON=python3
PIPX=pipx

define default-help
# invoke: 'make uninstall', 'make install'
endef
.PHONY: default
default: all
	@echo "## ${PROJECT_NAME}: make default"
	@echo "## ${PROJECT_NAME}: make default [DONE]"

define  all-help
# Do all: uninstall clean install info check
endef
.PHONY: all
all: uninstall clean install info check

define info-help
# Dump various Python / tooling information
endef
.PHONY: info
info:
	@echo "## ${PROJECT_NAME}: make info"
	${PYTEST} --version || true
	${PYTHON} --version || true
	${PIPX} --version || true
	@echo "## ${PROJECT_NAME}: make info [DONE]"

define install-help
# install for current user
endef
.PHONY: install
install:
	@echo "## ${PROJECT_NAME}: make install"
	@${PIPX} install cijoe==v0.9.26 --include-deps
	@${PIPX} inject cijoe cijoe-pkg-qemu==v6.1.15
	@${PIPX} inject cijoe cijoe-pkg-linux==v0.9.7
	@${PIPX} inject cijoe cijoe-pkg-fio==v0.9.7
	@${PIPX} inject cijoe matplotlib
	@${PIPX} inject cijoe numpy
	@${PIPX} install rst2pdf
	@echo "## ${PROJECT_NAME}: make install [DONE]"

define uninstall-help
# uninstall
#
# Prefix with 'sudo' when uninstalling a system-wide installation
endef
.PHONY: uninstall
uninstall:
	@echo "## ${PROJECT_NAME}: make uninstall"
	@${PIPX} uninstall cijoe || echo "Cannot uninstall => That is OK"
	@echo "## ${PROJECT_NAME}: make uninstall [DONE]"

define examples-help
# Run pytest on the testcase-test
endef
.PHONY: check
check:
	@echo "## ${PROJECT_NAME}: make check"
	${PYTEST} tests/selftest --config configs/default-config.toml
	@echo "## ${PROJECT_NAME}: make check [DONE]"

define clean-help
# clean the cijoe-output and other files which are not commited
endef
.PHONY: clean
clean:
	@echo "## ${PROJECT_NAME}: clean"
	@git clean -fdx || echo "Failed git-clean ==> That is OK"
	@echo "## ${PROJECT_NAME}: clean [DONE]"
