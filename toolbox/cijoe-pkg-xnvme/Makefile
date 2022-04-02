PROJECT_NAME=cijoe-pkg-xnvme
PROJECT_VERSION_MAJOR=$(shell grep -oP 'version=\"(\K[0-9\.]+)' setup.py | cut -d "." -f 1)
PROJECT_VERSION_MINOR=$(shell grep -oP 'version=\"(\K[0-9\.]+)' setup.py | cut -d "." -f 2)
PROJECT_VERSION_PATCH=$(shell grep -oP 'version=\"(\K[0-9\.]+)' setup.py | cut -d "." -f 3)
PROJECT_VERSION=${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}
NEXT_VERSION_PATCH=$$((${PROJECT_VERSION_PATCH} + 1))
NEXT_VERSION=${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${NEXT_VERSION_PATCH}

.PHONY: default
default: uninstall install
	@echo "# DONE!"

.PHONY: install
install:
	pip3 install . --user

.PHONY: uninstall
uninstall:
	pip3 uninstall ${PROJECT_NAME} --yes || echo "Cannot uninstall => That is OK"

.PHONY: install-system
install-system:
	pip3 install .

.PHONY: dev
dev: uninstall install selftest-view
	@echo -n "# dev: "; date

.PHONY: bump
bump:
	@echo "# Bumping '${PROJECT_VERSION}' to '${NEXT_VERSION}'"
	@sed -i -e s/"version=\".*\""/"version=\"${NEXT_VERSION}\""/g setup.py

.PHONY: clean
clean:
	@rm -r build || echo "Cannot remove => That is OK"
	@rm -r dist || echo "Cannot remove => That is OK"
	@rm -r selftest_results || echo "Cannot remove => That is OK"

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

.PHONY: selftest
selftest:
	@rm -r selftest_results || echo "Cannot remove => That is OK"
	cij_selftest 0 0 selftest_results

.PHONY: selftest-view
selftest-view:
	@rm -r selftest_results || echo "Cannot remove => That is OK"
	cij_selftest 0 1 selftest_results
