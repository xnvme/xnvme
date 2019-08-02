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
	@python apigen.py --tags ../../tags --output ../capis

.PHONY: commands-linux
commands-linux:
	@echo "# building: building"
	@kmdo ../building || true
	@echo "# capis: building"
	@kmdo ../capis || true
	@echo "# cli: building"
	@kmdo ../cli || true
	@echo "# examples: building"
	@kmdo ../examples || true
	@echo "# prereqs: building"
	@kmdo ../prereqs || true
	@echo "# quick_start: building"
	@kmdo ../quick_start || true
	@echo "# tutorial: building"
	@kmdo ../tutorial || true

.PHONY: commands-spdk
commands-spdk:
	@kmdo ../backends/xnvme_be_spdk || true

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
	sphinx-build -E -b html -c ./ ../ $(BUILD_DIR)/html

# Produce the sphinx stuff (without commands)
.PHONY: docs
docs: clean deps configure doxy apis sphinx
	@echo "# DONE"

.PHONY: docs-view-html
docs-view-html:
	xdg-open $(BUILD_DIR)/html/index.html &
