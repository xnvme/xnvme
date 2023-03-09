#
# This Makefile serves as convenient command-line auto-completion when working on Python bindings
#
#
PY ?= python3
PY_ENV_ACTIVATE = . .build-py-env/bin/activate
PY_ENV = ${PY_ENV_ACTIVATE} && python
WORKSPACE_ROOT ?= ..
TOOLBOX_DIR?=../../toolbox
PROJECT_NAME=xnvme-cy-bindings

define default-help
# invoke: 'make uninstall', 'make install'
endef
.PHONY: default
default: build
	@echo "## py: make default"
	@echo "## py: make default [DONE]"


define .build-py-env-help
# Init python build virtual env
endef
.build-py-env:
	@echo "Creating Python virtual env"
	@${PY} -m venv .build-py-env
	@echo "Upgrading pip"
	@${PY_ENV} -m pip install --upgrade pip setuptools
	@echo "Install local version of xnvme-core [REMEMBER TO REBUILD THESE!]"
	@${PY_ENV} -m pip install ../xnvme-core/dist/xnvme-core-*.tar.gz
	@${PY_ENV} -m pip install ../xnvme-cy-header/dist/xnvme-cy-header-*.tar.gz
	@echo "Install python dependencies"
	@${PY_ENV} -m pip install -r requirements.txt

define all-help
# Do the common task during development of: uninstall clean build install test
endef
.PHONY: all
all: uninstall clean build install test

define build-help
# Generate cython pxd, ctypes, patch them, then create a source package
endef
.PHONY: build
build: .build-py-env build-sdist build-bdist

define build-sdist-help
# Generate cython pxd, ctypes, patch them, then create a source package
endef
.PHONY: build-sdist
build-sdist: .build-py-env
	@echo "## py: make build-sdist"
	@echo "Building sdist"
	@${PY_ENV} setup.py sdist
	@echo "## py: make build-sdist [DONE]"

define build-bdist-help
# Generate cython pxd, ctypes, patch them, then create a source package
endef
.PHONY: build-bdist
build-bdist: .build-py-env
	@echo "## py: make build-bdist"
	@echo "Compiling Cython extension"
	@${PY_ENV} setup.py build_ext
	@echo "Building sdist and bdist wheel"
	@${PY_ENV} setup.py bdist_wheel
	@echo "Renaming packages"
	@echo "## py: make build-bdist [DONE]"

define install-help
# install for current user
endef
.PHONY: install
install:
	@echo "## py: make install"
	@${PY} -m pip install dist/$(subst -,_,$(PROJECT_NAME))-*.whl --user --break-system-packages || ${PY} -m pip install dist/${PROJECT_NAME}-*.tar.gz --user --no-build-isolation  --break-system-packages
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
endef
.PHONY: install-system
install-system:
	@echo "## py: make install-system"
	@${PY} --version
	# --force needed, as when installing a whl, it apparently sees that the local dir has the package "installed".
	# --no-deps needed, as it tries to find xnvme on PyPi.
	@${PY} -m pip install dist/$(subst -,_,$(PROJECT_NAME))-*.whl --force --no-deps --break-system-packages || ${PY} -m pip install dist/${PROJECT_NAME}-*.tar.gz --no-build-isolation  --break-system-packages
	@echo "## py: make install-system [DONE]"

define test-help
# Run pytest on tests/
endef
.PHONY: test
test:
	@echo "## py: make test"
	@${PY} -m pytest --pyargs xnvme
	@echo "## py: make test [DONE]"

define clean-help
# clean the Python build dirs (build, dist)
endef
.PHONY: clean
clean:
	@echo "## py: clean"
	@rm -f -r .build-py-env
	@rm -f -r build
	@rm -f -r dist
	@rm -f -r *.egg-info
	@rm -f MANIFEST
	@rm -f xnvme/cython_bindings/*.pxd
	@rm -f xnvme/cython_bindings/*.pyx
	@rm -f xnvme/cython_bindings/*.c
	@rm -f xnvme/cython_bindings/*.so
	@find . -name '__pycache__' -type d -exec rm -frv {} \; || echo "Cannot remove __pycache__ => That is OK"
	@echo "## py: clean [DONE]"

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
