#
# This Makefile serves as convenient command-line auto-completion when working on Python bindings
#
#

define default-help
# invoke: 'make uninstall', 'make install'
endef
.PHONY: default
default: build
	@echo "## py: make default"
	@echo "## py: make default [DONE]"


define all-help
# Do the common task during development of: uninstall clean build install
endef
.PHONY: all
all: uninstall clean build install

define build-help
# Call build on all subpackages
endef
.PHONY: build
build:
	@cd xnvme-core && make build
	@cd xnvme-cy-header && make build
	@cd xnvme-cy-bindings && make build


define install-help
# install all subpackages for current user
endef
.PHONY: install
install:
	@cd xnvme-core && make install
	@cd xnvme-cy-header && make install
	@cd xnvme-cy-bindings && make install

define install-system-help
# install all subpackages system-wide
endef
.PHONY: install-system
install-system:
	@cd xnvme-core && make install-system
	@cd xnvme-cy-header && make install-system
	@cd xnvme-cy-bindings && make install-system

define uninstall-help
# uninstall all subpackages
#
# Prefix with 'sudo' when uninstalling a system-wide installation
endef
.PHONY: uninstall
uninstall:
	@cd xnvme-core && make uninstall
	@cd xnvme-cy-header && make uninstall
	@cd xnvme-cy-bindings && make uninstall

define clean-help
# clean all subpackages
endef
.PHONY: clean
clean:
	@cd xnvme-core && make clean
	@cd xnvme-cy-header && make clean
	@cd xnvme-cy-bindings && make clean

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
