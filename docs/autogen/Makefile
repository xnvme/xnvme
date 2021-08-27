BUILD_DIR?=build

default: docs docs-view-html

.PHONY: deps
deps:
	pip3 install -r requirements.txt --user

.PHONY: clean
clean:
	rm -fr $(BUILD_DIR) || true

.PHONY: configure
configure: clean
	mkdir -p $(BUILD_DIR)

.PHONY: apis
apis:
	@cd ../.. && make tags-xnvme-public
	@python3 apigen.py --tags ../../tags --output ../capis

.PHONY: commands
commands:
	@echo "# commands"
	@kmdo ../getting_started  || true
	@kmdo ../capis || true
	@kmdo ../examples || true
	@kmdo ../tools || true

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
	PYTHONPATH="${PYTHONPATH}:${PWD}/../../scripts" sphinx-build -E -b html -c ./ ../ $(BUILD_DIR)/html

# Produce the sphinx stuff (without commands)
.PHONY: docs
docs: clean deps configure doxy apis sphinx
	@echo "# DONE"

.PHONY: docs-view-html
docs-view-html:
	xdg-open $(BUILD_DIR)/html/index.html &
