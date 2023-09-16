#
# SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
#
# SPDX-License-Identifier: BSD-3-Clause

.PHONY: default
default: all

.PHONY: deps
deps:
	pipx install senfd --force
	pipx inject senfd yace --include-deps --force

.PHONE: senfd
senfd:
	@ls specs/*.docx 1> /dev/null 2>&1 || { echo "Place .docx specification documents in 'specs' folder"; exit 1; }
	senfd specs/*.docx

.PHONY: all
all:
	yace --c-to-yace ../include/libxnvme.h
	yace --c-to-yace ../include/libxnvme.h 2>&1 | grep ERROR | wc
