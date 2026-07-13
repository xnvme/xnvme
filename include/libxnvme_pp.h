/**
 * SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * @file libxnvme_pp.h
 */

/**
 * Options for pretty-printer (``*_pr``, ``*_fpr``) functions
 *
 * @see libxnvme_pp.h
 *
 * @enum xnvme_pr
 */
enum xnvme_pr {
	XNVME_PR_DEF   = 0x0, ///< XNVME_PR_DEF: Default options
	XNVME_PR_YAML  = 0x1, ///< XNVME_PR_YAML: Print formatted as YAML
	XNVME_PR_TERSE = 0x2  ///< XNVME_PR_TERSE: Print without formatting
};