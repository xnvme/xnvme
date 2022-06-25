#
# This Makefile serves as convenient command-line auto-completion for the Cython header
#
PY ?= python3
PY_ENV_ACTIVATE = . .build-py-env/bin/activate
PY_ENV = ${PY_ENV_ACTIVATE} && python
WORKSPACE_ROOT ?= ..
TOOLBOX_DIR?=../../toolbox
PROJECT_NAME=xnvme-cy-header

define default-help
# invoke: 'make uninstall', 'make install'
endef
.PHONY: default
default: build
	@echo "## py: make default"
	@echo "## py: make default [DONE]"

define build-py-env-help
# Init python build virtual env
endef
.PHONY: build-py-env
build-py-env:
	@echo "Creating Python virtual env"
	@${PY} -m venv .build-py-env
	@echo "Upgrading pip"
	@${PY_ENV} -m pip install --upgrade pip
	@echo "Install python dependencies"
	@${PY_ENV} -m pip install -r requirements.txt

define build-help
# Do the common task during development of: uninstall clean build install test
endef
.PHONY: all
all: uninstall clean build install test

define build-help
# Generate ctypes, patch them, then create a source package
endef
.PHONY: build
build: build-py-env generate-cython-libxnvme-pxd
	@echo "## py: make build-sdist"
	@${PY_ENV} setup.py sdist
	@echo "## py: make build-sdist [DONE]"

define install-help
# install for current user
endef
.PHONY: install
install:
	@echo "## py: make install"
	@${PY} -m pip install dist/xnvme-*.tar.gz --user
	@echo "## py: make install [DONE]"

define uninstall-help
# uninstall
#
# Prefix with 'sudo' when uninstalling a system-wide installation
endef
.PHONY: uninstall
uninstall:
	@echo "## py: make uninstall"
	@${PY} -m pip uninstall ${PROJECT_NAME} --yes || echo "Cannot uninstall => That is OK"
	@echo "## py: make uninstall [DONE]"

define install-system-help
# install system-wide
#
# install system-wide
endef
.PHONY: install-system
install-system:
	@echo "## py: make install-system"
	@${PY} -m pip install dist/xnvme-*.tar.gz
	@echo "## py: make install-system [DONE]"

define test-help
# Run pytest on tests/
endef
.PHONY: test
test:
	@echo "## py: make test"
	@${PY} -m pytest --pyargs xnvme
	@echo "## py: make test [DONE]"

define generate-cython-libxnvme-pxd-help
# Generate Cython C API mapping (xnvme.cython_header) from xNVMe public C API headers
endef
.PHONY: generate-cython-libxnvme-pxd
generate-cython-libxnvme-pxd: build-py-env
	@echo "## py: generate-cython-libxnvme-pxd"
	@${PY_ENV} aux/generate_cython_header.py --src-root ../..
	@echo "## py: generate-cython-libxnvme-pxd [DONE]"

define clean-help
# clean the Python build dirs (build, dist)
endef
.PHONY: clean
clean:
	@echo "## py: clean"
	@rm -r .build-py-env || echo "Cannot remove => That is OK"
	@rm -r build || echo "Cannot remove => That is OK"
	@rm -r dist || echo "Cannot remove => That is OK"
	@rm -r *.egg-info || echo "Cannot remove => That is OK"
	@rm MANIFEST || echo "Cannot remove => That is OK"
	@echo "## py: clean [DONE]"

define help-help
# Print the description of every target
#
# Print the description of every target
endef
.PHONY: help
help:
	@./$(TOOLBOX_DIR)/print_help.py --repos .

define help-verbose-help
# Print the verbose description of every target
#
# Print the verbose description of every target
endef
.PHONY: help-verbose
help-verbose:
	@./$(TOOLBOX_DIR)/print_help.py --verbose --repos .
