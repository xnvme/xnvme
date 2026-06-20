#
# SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
#
# SPDX-License-Identifier: BSD-3-Clause

DOCX_PATH := $(HOME)/specs/nvme/docx
BUILD_DIR := $(PWD)/builddir
HEADERS_CORE := $(shell sed 's|^|../include/|' headers_core.txt)

.PHONY: default
default: info clean ir capi

.PHONY: info
info:
	@echo "# DOCX_PATH($(DOCX_PATH))"
	@echo "# BUILD_DIR($(BUILD_DIR))"

.PHONY: clean
clean:
	rm -r $(BUILD_DIR) || true

.PHONY: deps
deps:
	pipx install senfd --force
	pipx inject senfd yace --include-deps --force

.PHONE: senfd
senfd:
	@mkdir -p $(BUILD_DIR)/senfd-documents
	@ls $(DOCX_PATH)/*.docx 1> /dev/null 2>&1 || { echo "Cannot find specification documents in .docx format in DOCX_PATH($(DOCX_PATH))"; exit 1; }
	@senfd $(DOCX_PATH)/*.docx --output $(BUILD_DIR)/senfd-documents
	@cp $(BUILD_DIR)/senfd-documents/merged.model.document.json datamodel/nvme.model.document.json
	@cd datamodel && sha256sum nvme.model.document.json > nvme.model.document.json.sha256sum
	@tar --mtime='2024-01-01' --owner=0 --group=0 -czf datamodel/nvme.model.document.json.tgz datamodel/*.json datamodel/*.sha256sum

.PHONY: ir
ir:
	mkdir -p $(BUILD_DIR)/ir
	yace $(HEADERS_CORE) --output $(BUILD_DIR)/ir

.PHONY: capi
capi:
	mkdir -p $(BUILD_DIR)/capi
	yace $(BUILD_DIR)/ir/*yaml --emit capi --output $(BUILD_DIR)

.PHONY: ctypes
ctypes:
	mkdir -p $(BUILD_DIR)/capi
	yace $(BUILD_DIR)/ir/*yaml --emit ctypes --output $(BUILD_DIR)