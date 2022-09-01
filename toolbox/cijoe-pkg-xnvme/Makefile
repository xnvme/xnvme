#
# This Makefile serves as convenient command-line auto-completion
#
PROJECT_NAME=cijoe-pkg-xnvme

define default-help
# invoke: 'make uninstall', 'make install'
endef
.PHONY: default
default: build
	@echo "## py: make default"
	@echo "## py: make default [DONE]"

define  all-help
# Do all: clean uninstall build install
endef
.PHONY: all
all: uninstall clean build install test

define build-help
# Build the package (source distribution package)
endef
.PHONY: build
build:
	@echo "## py: make build-sdist"
	@python3 setup.py sdist
	@echo "## py: make build-sdist [DONE]"

define install-help
# install for current user
endef
.PHONY: install
install:
	@echo "## py: make install"
	@python3 -m pip install dist/*.tar.gz --user
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
	@python3 -m pip install dist/*.tar.gz
	@echo "## py: make install-system [DONE]"

define examples-help
# Run pytest on the testcase-test
endef
.PHONY: test
test:
	@echo "## py: make test"
	python3 -m pytest --pyargs cijoe.xnvme.selftest --config src/cijoe/xnvme/configs/default.config
	@echo "## py: make test [DONE]"

.PHONY: release-build
release-build:
	python setup.py sdist
	python setup.py bdist_wheel

.PHONY: release-upload
release-upload:
	twine upload dist/*

.PHONY: release
release: clean release-build release-upload
	@echo -n "# rel: "; date

define clean-help
# clean the Python build dirs (build, dist)
endef
.PHONY: clean
clean:
	@echo "## py: clean"
	@git clean -fdx || echo "Failed git-clean ==> That is OK"
	@echo "## py: clean [DONE]"
