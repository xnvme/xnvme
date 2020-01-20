DOC_BUILD_DIR=build
PROJECT_NAME=pyxnvme
PROJECT_VERSION_MAJOR=$(shell grep "VERSION_MAJOR = ." xnvme/__init__.py | cut -d " " -f 3)
PROJECT_VERSION_MINOR=$(shell grep "VERSION_MINOR = ." xnvme/__init__.py | cut -d " " -f 3)
PROJECT_VERSION_PATCH=$(shell grep "VERSION_PATCH = ." xnvme/__init__.py | cut -d " " -f 3)
PROJECT_VERSION=${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}
NEXT_VERSION_PATCH=$$((${PROJECT_VERSION_PATCH} + 1))
NEXT_VERSION=${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${NEXT_VERSION_PATCH}

PIP=pip3

INSTALL_OPTS?=

.PHONY: system
system:
	$(PIP) install . $(INSTALL_OPTS)

.PHONY: install
install:
	$(PIP) install . $(INSTALL_OPTS) --user

.PHONY: uninstall
uninstall:
	$(PIP) uninstall ${PROJECT_NAME} --yes || echo "Cannot uninstall => That is OK"

.PHONY: dev
dev: uninstall install selftest-view
	@echo -n "# dev: "; date

.PHONY: bump
bump:
	@echo "# Bumping '${PROJECT_VERSION}' to '${NEXT_VERSION}'"
	@sed -i -e s/"version=\".*\""/"version=\"${NEXT_VERSION}\""/g setup.py
	@sed -i -e s/"^VERSION_PATCH = .*"/"VERSION_PATCH = ${NEXT_VERSION_PATCH}"/g xnvme/__init__.py

.PHONY: clean
clean:
	@rm -r build || echo "Cannot remove => That is OK"
	@rm -r dist || echo "Cannot remove => That is OK"
	@rm -r selftest_results || echo "Cannot remove => That is OK"

#
# Produce release and upload to the Python package index
#
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
