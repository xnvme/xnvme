// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef __INTERNAL_XNVME_BE_NVMF_FABRIC_H
#define __INTERNAL_XNVME_BE_NVMF_FABRIC_H

#include <stdint.h>

#include <xnvme_be_nvmf_ctrlr.h>
#include <xnvme_be_nvmf_qpair.h>

/**
 * Sets CC.EN and waits on CSTS.RDY through Fabrics Property Get and Set on
 * @admin_qpair. Backs `xnvme_be_nvmf_ctrlr_enable()`.
 */
int
xnvme_be_nvmf_fabric_enable(struct xnvme_be_nvmf_ctrlr *ctrlr,
			    struct xnvme_be_nvmf_qpair *admin_qpair, bool is_discovery);

/**
 * Sends the Fabrics Connect command on @qpair, the core half of
 * `xnvme_be_nvmf_qpair_connect()`. The transport connect must already have
 * reached `CONNECTED`.
 */
int
xnvme_be_nvmf_fabric_connect(struct xnvme_be_nvmf_qpair *qpair);

/**
 * Reads the discovery log page and creates a subsystem for every entry in it.
 */
int
xnvme_be_nvmf_discovery_get_log(struct xnvme_be_nvmf_ctrlr *ctrlr,
				struct xnvme_be_nvmf_qpair *admin_qpair,
				struct xnvme_spec_discovery_log_page *log_page);

#endif /* __INTERNAL_XNVME_BE_NVMF_FABRIC_H */