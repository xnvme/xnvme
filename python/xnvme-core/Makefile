#
# This Makefile serves as convenient command-line auto-completion when working on Python bindings
#
TOOLBOX_DIR?=../toolbox
PROJECT_NAME=xnvme
XNVME_CAPI_HEADERS=$(shell ls ../../include/libxnvme* | grep -v util | grep -v libxnvmec)

define default-help
# invoke: 'make uninstall', 'make install'
endef
.PHONY: default
default: build
	@echo "## py: make default"
	@echo "## py: make default [DONE]"

define build-help
# Do the common task during development of: uninstall clean build install test
endef
.PHONY: all
all: uninstall clean build install test

define build-help
# Generate ctypes, patch them, then create a source package
endef
.PHONY: build
build: generate-ctypes patch-ctypes
	@echo "## py: make build-sdist"
	@python3 setup.py sdist
	@echo "## py: make build-sdist [DONE]"

define install-help
# install for current user
endef
.PHONY: install
install:
	@echo "## py: make install"
	@python3 -m pip install dist/xnvme-*.tar.gz --user
	@echo "## py: make install [DONE]"

define uninstall-help
# uninstall
#
# Prefix with 'sudo' when uninstalling a system-wide installation
endef
.PHONY: uninstall
uninstall:
	@echo "## py: make uninstall"
	@python3 -m pip uninstall ${PROJECT_NAME} --yes || echo "Cannot uninstall => That is OK"
	@echo "## py: make uninstall [DONE]"

define install-system-help
# install system-wide
#
# install system-wide
endef
.PHONY: install-system
install-system:
	@echo "## py: make install-system"
	@python3 -m pip install dist/xnvme-*.tar.gz
	@echo "## py: make install-system [DONE]"

define test-help
# Run pytest on tests/
endef
.PHONY: test
test:
	@echo "## py: make test"
	@python3 -m pytest --pyargs xnvme
	@echo "## py: make test [DONE]"

define generate-ctypes-help
# Generate definitions (xnvme_ctypes.header) from xNVMe public C API headers
endef
.PHONY: generate-ctypes
generate-ctypes:
	@echo "## py: gen"
	@echo "## py:headers: ${XNVME_CAPI_HEADERS}"
	@clang2py ${XNVME_CAPI_HEADERS} --clang-args="-I../../include" > xnvme/ctypes_mapping/api.py
	@echo "## Remember to also call 'make patch-ctypes' to fix format"
	@echo "## py: gen [DONE]"

define patch-ctypes-help
# Patch definitions (xnvme_ctypes.header)
endef
.PHONY: patch-ctypes
patch-ctypes:
	@echo "## py: gen"
	@python3 aux/patch_ctypes_mapping.py --path xnvme/ctypes_mapping/api.py
	@black xnvme/ctypes_mapping/api.py || echo "could not fix format"
	@echo "## py: gen [DONE]"

define clean-help
# clean the Python build dirs (build, dist)
endef
.PHONY: clean
clean:
	@echo "## py: clean"
	@rm -r build || echo "Cannot remove => That is OK"
	@rm -r dist || echo "Cannot remove => That is OK"
	@rm -r *.egg-info || echo "Cannot remove => That is OK"
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
