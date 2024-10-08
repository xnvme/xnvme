# The builddir path is hardcoded in the config/doxy.conf and config/conf.py
BUILD_DIR?=builddir
SOURCE_DIR?=../../
CONFIG_DIR?=./
DOCS_DIR?=../

VENV_NAME = .venv
VENV = . ${VENV_NAME}/bin/activate && 
PYTHON = ${VENV} python3

default: docs docs-view-html

.PHONY: deps
deps:
#	python3 -m venv $(VENV_NAME)
	pipx install poetry
	poetry install --no-root
	echo "export PYTHONPATH=${PYTHONPATH}:${PWD}/$(SOURCE_DIR)/toolbox" >> ${VENV_NAME}/bin/activate

# The PYTHONPATH is needed such that we can call the 'xnvme_ver.py' script

.PHONY: clean
clean: clean-apis
	rm -fr $(BUILD_DIR) || true

.PHONY: configure
configure: clean
	mkdir -p $(BUILD_DIR)

# This needs the xNVMe source-code
.PHONY: apis
apis:
	cd $(SOURCE_DIR) && make tags-xnvme-public
	${PYTHON} apigen.py --tags $(SOURCE_DIR)/tags --output $(DOCS_DIR)/api

.PHONY: clean-apis
clean-apis:
	rm -f $(DOCS_DIR)/api/core/xnvme*
	rm -f $(DOCS_DIR)/api/extended/xnvme*


.PHONY: commands
commands:
	@echo "# commands"
	${VENV} kmdo $(DOCS_DIR)/getting_started --exclude "build_"
	${VENV} kmdo $(DOCS_DIR)/tools
	${VENV} kmdo $(DOCS_DIR)/tutorial

.PHONY: doxy
doxy:
	mkdir -p $(BUILD_DIR)/doxy
	doxygen doxy.cfg

.PHONY: sphinx
sphinx:
	mkdir -p $(BUILD_DIR)/html
	${PYTHON} -m sphinx -E -b html -c $(CONFIG_DIR) $(DOCS_DIR) $(BUILD_DIR)/html

.PHONY: linkcheck
linkcheck:
	${PYTHON} -m sphinx -b linkcheck -c $(CONFIG_DIR) $(DOCS_DIR) $(BUILD_DIR)/html

# Produce the sphinx stuff (without commands)
.PHONY: docs
docs: clean deps configure doxy apis sphinx
	@echo "# DONE"

.PHONY: docs-view-html
docs-view-html: serve
	@xdg-open http://localhost:8888/ || open http://localhost:8888 || echo "Failed launching browser"

.PHONY: serve
serve:
	@screen -S xnvmedocs -X kill || echo "Could not kill server, probably not running"
	@cd $(BUILD_DIR)/html && screen -S xnvmedocs -d -m python3 -m http.server 8888