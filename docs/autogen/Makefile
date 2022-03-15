# The builddir path is hardcoded in the config/doxy.conf and config/conf.py
BUILD_DIR?=builddir
SOURCE_DIR?=../../
CONFIG_DIR?=./
DOCS_DIR?=../

default: docs docs-view-html

.PHONY: deps
deps:
	pip3 install -r requirements.txt --user

.PHONY: deps-system
deps-system:
	pip3 install -r requirements.txt

.PHONY: clean
clean:
	rm -fr $(BUILD_DIR) || true

.PHONY: configure
configure: clean
	mkdir -p $(BUILD_DIR)

# This needs the xNVMe source-code
.PHONY: apis
apis:
	@cd $(SOURCE_DIR) && make tags-xnvme-public
	@python3 apigen.py --tags $(SOURCE_DIR)/tags --output $(DOCS_DIR)/capis

.PHONY: commands
commands:
	@echo "# commands"
	@kmdo $(DOCS_DIR)/getting_started
	@kmdo $(DOCS_DIR)/capis
	@kmdo $(DOCS_DIR)/examples
	@kmdo $(DOCS_DIR)/tools
	@kmdo $(DOCS_DIR)/tutorial

.PHONY: doxy
doxy:
	@mkdir -p $(BUILD_DIR)/doxy
	doxygen doxy.cfg

.PHONY: doxy-view
doxy-view:
	xdg-open $(BUILD_DIR)/doxy/html/index.html

.PHONY: sphinx
sphinx:
	@mkdir -p $(BUILD_DIR)/html
	# The PYTHONPATH is needed such that we can call the 'xnvme_ver.py' script
	PYTHONPATH="${PYTHONPATH}:${PWD}/$(SOURCE_DIR)/toolbox" sphinx-build -E -b html -c $(CONFIG_DIR) $(DOCS_DIR) $(BUILD_DIR)/html

# Produce the sphinx stuff (without commands)
.PHONY: docs
docs: clean deps configure doxy apis sphinx
	@echo "# DONE"

.PHONY: docs-view-html
docs-view-html:
	xdg-open $(BUILD_DIR)/html/index.html &
