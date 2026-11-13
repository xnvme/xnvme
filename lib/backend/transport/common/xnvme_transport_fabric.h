// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef __INTERNAL_XNVME_TRANSPORT_FABRIC_H
#define __INTERNAL_XNVME_TRANSPORT_FABRIC_H

#include <stdint.h>

#include <xnvme_transport_ctrlr.h>
#include <xnvme_transport_qpair.h>

/**
 * Sets CC.EN and waits on CSTS.RDY through Fabrics Property Get and Set on
 * @admin_qpair. Backs `xnvme_transport_ctrlr_enable()`. When @mqes is not NULL, it receives
 * CAP.MQES of the controller association.
 */
int
xnvme_transport_fabric_enable(struct xnvme_transport_ctrlr *ctrlr,
			      struct xnvme_transport_qpair *admin_qpair, bool is_discovery,
			      uint16_t *mqes);

/**
 * Sends the Fabrics Connect command on @qpair, the core half of
 * `xnvme_transport_qpair_connect()`. The transport connect must already have
 * reached `CONNECTED`.
 */
int
xnvme_transport_fabric_connect(struct xnvme_transport_qpair *qpair);

/**
 * Reads the discovery log page and creates a subsystem for every entry in it.
 */
int
xnvme_transport_discovery_get_log(struct xnvme_transport_ctrlr *ctrlr,
				  struct xnvme_transport_qpair *admin_qpair);

#endif /* __INTERNAL_XNVME_TRANSPORT_FABRIC_H */