// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <ctype.h>
#include <endian.h>
#include <errno.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/random.h>
#include <time.h>

#include <xnvme_transport_base.h>
#include <xnvme_transport_debug.h>
#include <xnvme_transport_ctrlr.h>
#include <xnvme_transport_qpair.h>
#include <xnvme_transport_fabric.h>

#define TRANSPORT_DEBUG_CATEGORY TRANSPORT_DEBUG_CATEGORY_FABRICS

// Transport fabric timing constants
// CAP.TO is in units of 500 ms, fall back to 5 seconds when the controller reports 0
#define TRANSPORT_FABRIC_CAP_TO_UNIT_NS (500ULL * 1000000ULL)
#define TRANSPORT_FABRIC_READY_TIMEOUT_NS (5ULL * 1000000000ULL)

// Fabrics specific status codes
#define TRANSPORT_FABRIC_SCT_COMMAND_SPECIFIC 0x1
#define TRANSPORT_FABRIC_SC_CONNECT_INVALID_PARAMS 0x82

// Minimum retry interval for ready state in nanoseconds, 100us min, 5ms max
#define TRANSPORT_FABRIC_RETRY_MIN_NS (100ULL * 1000ULL)
#define TRANSPORT_FABRIC_READY_MAX_NS (5ULL * 1000000ULL)

#define RETRY_CLAMP(val, max) ((val) > (max) ? (max) : (val))

#define TRANSPORT_FABRIC_HOSTNQN_UUID_BASE "nqn.2014-08.org.nvmexpress:uuid:"

// Used only when the kernel cannot provide random bytes for the per-process host identity
#define TRANSPORT_FABRIC_FALLBACK_HOSTNQN \
	TRANSPORT_FABRIC_HOSTNQN_UUID_BASE "4f6b0c94-cafe-4ab6-a011-fef3f2678ffd"

#define TRANSPORT_FABRIC_UUID_STR_LEN 36
#define TRANSPORT_FABRIC_HOSTNQN_UUID_PREFIX ":uuid:"

/*
 * Per-process default host identity, as in SPDK: a random UUID generated once, used as HOSTID
 * and embedded in the default host NQN, so every Connect from this process carries the same
 * identity.
 */
static pthread_once_t g_transport_fabric_host_once = PTHREAD_ONCE_INIT;
static uint8_t g_transport_fabric_hostid[16];
static char g_transport_fabric_hostnqn[sizeof(TRANSPORT_FABRIC_HOSTNQN_UUID_BASE) +
				       TRANSPORT_FABRIC_UUID_STR_LEN];

static inline void
_transport_fabric_set_empty_sgl(struct xnvme_spec_sgl_descriptor *sgl)
{
	sgl->addr = 0;
	sgl->generic.type = XNVME_SPEC_SGL_DESCR_TYPE_KEYED_DATA_BLOCK;
	sgl->generic.subtype = XNVME_SPEC_SGL_DESCR_SUBTYPE_ADDRESS;
	sgl->unkeyed.len = 0;
}

/**
 * Parses the canonical text form of a UUID, "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx", into its 16
 * bytes in RFC 4122 (big-endian) order, the order used for HOSTID. @str may continue after the
 * UUID, so it can point into a larger string.
 */
static int
_transport_fabric_uuid_parse(const char *str, uint8_t uuid[16])
{
	// sscanf() also accepts a sign or whitespace in a %x field, so check the format first
	for (int i = 0; i < TRANSPORT_FABRIC_UUID_STR_LEN; ++i) {
		bool dash = (i == 8 || i == 13 || i == 18 || i == 23);

		if (dash ? str[i] != '-' : !isxdigit((unsigned char)str[i])) {
			return -EINVAL;
		}
	}

	return sscanf(str,
		      "%2hhx%2hhx%2hhx%2hhx-%2hhx%2hhx-%2hhx%2hhx-%2hhx%2hhx-"
		      "%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx",
		      &uuid[0], &uuid[1], &uuid[2], &uuid[3], &uuid[4], &uuid[5], &uuid[6],
		      &uuid[7], &uuid[8], &uuid[9], &uuid[10], &uuid[11], &uuid[12], &uuid[13],
		      &uuid[14], &uuid[15]) == 16
		       ? 0
		       : -EINVAL;
}

/**
 * Derives HOSTID from a host NQN of the form "nqn.2014-08.org.nvmexpress:uuid:<UUID>", the same
 * convention as nvme-cli, where the host identifier is the UUID of the host NQN.
 */
static int
_transport_fabric_hostid_from_hostnqn(const char *hostnqn, uint8_t hostid[16])
{
	const char *uuid = strstr(hostnqn, TRANSPORT_FABRIC_HOSTNQN_UUID_PREFIX);

	if (!uuid) {
		return -EINVAL;
	}
	uuid += strlen(TRANSPORT_FABRIC_HOSTNQN_UUID_PREFIX);

	if (strlen(uuid) != TRANSPORT_FABRIC_UUID_STR_LEN) {
		return -EINVAL;
	}

	return _transport_fabric_uuid_parse(uuid, hostid);
}

// Generates a random (version 4, RFC 4122 variant) UUID
static int
_transport_fabric_uuid_generate(uint8_t uuid[16])
{
	ssize_t nbytes;

	do {
		nbytes = getrandom(uuid, 16, 0);
	} while (nbytes < 0 && errno == EINTR);

	if (nbytes != 16) {
		return nbytes < 0 ? -errno : -EIO;
	}

	uuid[6] = (uuid[6] & 0x0f) | 0x40;
	uuid[8] = (uuid[8] & 0x3f) | 0x80;

	return 0;
}

static void
_transport_fabric_host_init(void)
{
	const uint8_t *id = g_transport_fabric_hostid;

	if (_transport_fabric_uuid_generate(g_transport_fabric_hostid)) {
		TRANSPORT_WARN("WARN: cannot generate a host UUID, using a fixed host identity");
		snprintf(g_transport_fabric_hostnqn, sizeof(g_transport_fabric_hostnqn), "%s",
			 TRANSPORT_FABRIC_FALLBACK_HOSTNQN);
		_transport_fabric_hostid_from_hostnqn(g_transport_fabric_hostnqn,
						      g_transport_fabric_hostid);
		return;
	}

	snprintf(g_transport_fabric_hostnqn, sizeof(g_transport_fabric_hostnqn),
		 "%s%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
		 TRANSPORT_FABRIC_HOSTNQN_UUID_BASE, id[0], id[1], id[2], id[3], id[4], id[5],
		 id[6], id[7], id[8], id[9], id[10], id[11], id[12], id[13], id[14], id[15]);

	TRANSPORT_DEBUG("INFO: default hostnqn: %s", g_transport_fabric_hostnqn);
}

static inline void
_transport_fabric_handle_connect(struct xnvme_transport_qpair *qpair, void *buf)
{
	struct xnvme_spec_cpl *cpl = buf;
	struct xnvme_spec_transports_connect_resp_cpl *connect_cpl =
		(struct xnvme_spec_transports_connect_resp_cpl *)cpl;
	uint8_t sc = xnvme_bits_get(cpl->status, XNVME_SPEC_CQE_STATUS_SC);
	uint8_t sct = xnvme_bits_get(cpl->status, XNVME_SPEC_CQE_STATUS_SCT);

	if (sc != 0) {
		TRANSPORT_ERROR("FAILED: Fabric Connect rejected, sc: %u sct: %u", sc, sct);
		if (sct == TRANSPORT_FABRIC_SCT_COMMAND_SPECIFIC &&
		    sc == TRANSPORT_FABRIC_SC_CONNECT_INVALID_PARAMS) {
			TRANSPORT_ERROR(
				"FAILED: invalid parameter, ipo: %u ips: %" PRIu64,
				connect_cpl->connect_invalid.ipo,
				xnvme_bits_get(connect_cpl->connect_invalid.iattr,
					       XNVME_SPEC_TRANSPORTS_CONNECT_RESP_CPL_IATTR_IPS));
		}
		xnvme_transport_debug_print_cpl_error_code(cpl);
		qpair->state = XNVME_TRANSPORT_QPAIR_STATE_ERROR;
		return;
	}

	// Authentication is not supported yet, return error if required
	if (xnvme_bits_get(connect_cpl->success.authreq,
			   XNVME_SPEC_TRANSPORTS_CONNECT_RESP_CPL_AUTHREQ_ASCR)) {
		TRANSPORT_ERROR("ERROR: Fabric Connect accepted, but authentication is "
				"required (ascr=1)");
		qpair->state = XNVME_TRANSPORT_QPAIR_STATE_ERROR;
	} else if (xnvme_bits_get(connect_cpl->success.authreq,
				  XNVME_SPEC_TRANSPORTS_CONNECT_RESP_CPL_AUTHREQ_ATR)) {
		TRANSPORT_ERROR("ERROR: Fabric Connect accepted, but authentication is "
				"required (atr=1)");
		qpair->state = XNVME_TRANSPORT_QPAIR_STATE_ERROR;
	} else {
		qpair->attr.cntlid = connect_cpl->success.cntlid;
		TRANSPORT_DEBUG("INFO: Fabric Connect accepted, cntlid: %u", qpair->attr.cntlid);
		qpair->state = XNVME_TRANSPORT_QPAIR_STATE_READY;
	}

	return;
}

static int
_transport_fabric_prop_get(struct xnvme_transport_qpair *admin_qpair, uint32_t property,
			   bool is_8byte, uint64_t *value)
{
	struct xnvme_cmd_ctx ctx = xnvme_cmd_ctx_from_dev(admin_qpair->dev);
	struct xnvme_spec_transports_cmd *fcmd = (struct xnvme_spec_transports_cmd *)&ctx.cmd;
	struct xnvme_spec_transports_generic_cpl *cpl =
		(struct xnvme_spec_transports_generic_cpl *)&ctx.cpl;
	int err;

	fcmd->fcommon.opcode = XNVME_SPEC_TRANSPORTS_OPC;

	fcmd->fcommon.flags =
		xnvme_bits_set(fcmd->fcommon.flags, XNVME_SPEC_TRANSPORTS_COMMON_CMD_FLAGS_PSDT,
			       XNVME_SPEC_PSDT_SGL_MPTR_CONTIGUOUS);
	_transport_fabric_set_empty_sgl(&fcmd->fcommon.sgl1);

	fcmd->property_get.fctype = XNVME_SPEC_TRANSPORTS_COMMAND_PROPERTY_GET;
	fcmd->property_get.attrib = xnvme_bits_set(
		fcmd->property_get.attrib, XNVME_SPEC_TRANSPORTS_PROPERTY_GET_CMD_ATTRIB_PRS,
		is_8byte ? 0b001 : 0b000);
	fcmd->property_get.ofst = htole32(property);

	err = xnvme_transport_qpair_submit_sync(admin_qpair, &ctx);
	if (err) {
		TRANSPORT_ERROR("FAILED: property get 0x%x, err: %d", property, err);
		return err;
	}

	if (xnvme_bits_get(cpl->prop_get.status, XNVME_SPEC_CQE_STATUS_SC)) {
		TRANSPORT_ERROR("FAILED: property get 0x%x, sc: 0x%" PRIx64 ", sct: 0x%" PRIx64,
				property,
				xnvme_bits_get(cpl->prop_get.status, XNVME_SPEC_CQE_STATUS_SC),
				xnvme_bits_get(cpl->prop_get.status, XNVME_SPEC_CQE_STATUS_SCT));
		return -EIO;
	}

	*value = is_8byte ? cpl->prop_get.value : cpl->prop_get.ddw;

	return 0;
}

static inline int
_transport_fabric_prop_get32(struct xnvme_transport_qpair *admin_qpair, uint32_t property,
			     uint32_t *value)
{
	uint64_t value64 = 0;
	int err;

	err = _transport_fabric_prop_get(admin_qpair, property, false, &value64);
	*value = (uint32_t)value64;

	return err;
}

static inline int
_transport_fabric_prop_get64(struct xnvme_transport_qpair *admin_qpair, uint32_t property,
			     uint64_t *value)
{
	return _transport_fabric_prop_get(admin_qpair, property, true, value);
}

static int
_transport_fabric_prop_set32(struct xnvme_transport_qpair *admin_qpair, uint32_t property,
			     uint32_t value)
{
	struct xnvme_cmd_ctx ctx = xnvme_cmd_ctx_from_dev(admin_qpair->dev);
	struct xnvme_spec_transports_cmd *fcmd = (struct xnvme_spec_transports_cmd *)&ctx.cmd;
	struct xnvme_spec_transports_generic_cpl *cpl =
		(struct xnvme_spec_transports_generic_cpl *)&ctx.cpl;
	int err;

	ctx.cmd.common.opcode = XNVME_SPEC_TRANSPORTS_OPC;

	fcmd->fcommon.flags =
		xnvme_bits_set(fcmd->fcommon.flags, XNVME_SPEC_TRANSPORTS_COMMON_CMD_FLAGS_PSDT,
			       XNVME_SPEC_PSDT_SGL_MPTR_CONTIGUOUS);
	_transport_fabric_set_empty_sgl(&fcmd->fcommon.sgl1);

	fcmd->property_set.fctype = XNVME_SPEC_TRANSPORTS_COMMAND_PROPERTY_SET;
	fcmd->property_set.attrib =
		xnvme_bits_set(fcmd->property_set.attrib,
			       XNVME_SPEC_TRANSPORTS_PROPERTY_SET_CMD_ATTRIB_PUS, 0b000);
	fcmd->property_set.ofst = property;
	fcmd->property_set.ddw = value;

	err = xnvme_transport_qpair_submit_sync(admin_qpair, &ctx);
	if (err) {
		TRANSPORT_ERROR("FAILED: property set 0x%x, err: %d", property, err);
		return err;
	}

	if (xnvme_bits_get(cpl->prop_set.status, XNVME_SPEC_CQE_STATUS_SC)) {
		TRANSPORT_ERROR("FAILED: property set 0x%x, sc: 0x%" PRIx64 ", sct: 0x%" PRIx64,
				property,
				xnvme_bits_get(cpl->prop_set.status, XNVME_SPEC_CQE_STATUS_SC),
				xnvme_bits_get(cpl->prop_set.status, XNVME_SPEC_CQE_STATUS_SCT));
		xnvme_transport_print_completion(&ctx.cpl);
		return -EIO;
	}

	return 0;
}

static inline void
_transport_fabric_connect_data_init(struct xnvme_transport_qpair *qpair, void *buf)
{
	struct xnvme_spec_transports_connect_data *data = buf;
	const char *hostnqn;

	memset(data, 0, sizeof(*data));

	data->cntlid = qpair->attr.cntlid;

	if (strlen(qpair->dev->ident.subnqn) == 0) {
		snprintf((char *)data->subnqn, sizeof(data->subnqn), "%s", XNVME_DISCOVERY_NQN);
	} else {
		snprintf((char *)data->subnqn, sizeof(data->subnqn), "%s",
			 qpair->dev->ident.subnqn);
	}

	pthread_once(&g_transport_fabric_host_once, _transport_fabric_host_init);

	// Without a user host NQN, use the per-process identity
	hostnqn = qpair->dev->opts.hostnqn ? qpair->dev->opts.hostnqn : g_transport_fabric_hostnqn;
	snprintf((char *)data->hostnqn, sizeof(data->hostnqn), "%s", hostnqn);

	// A user host NQN carrying a UUID supplies HOSTID, as with nvme-cli; otherwise, as in
	// SPDK, the per-process HOSTID is used
	if (_transport_fabric_hostid_from_hostnqn(hostnqn, data->hostid)) {
		memcpy(data->hostid, g_transport_fabric_hostid, sizeof(data->hostid));
	}

	TRANSPORT_DEBUG("INFO: Fabric connect data initialized with subnqn: %s, hostnqn: %s, "
			"cntlid: %u",
			data->subnqn, data->hostnqn, data->cntlid);
}

int
xnvme_transport_fabric_connect(struct xnvme_transport_qpair *qpair)
{
	struct xnvme_spec_transports_connect_data *connect_data;
	struct xnvme_cmd_ctx ctx = xnvme_cmd_ctx_from_dev(qpair->dev);
	union xnvme_spec_sqe *cmd = &ctx.cmd;
	struct xnvme_spec_transports_cmd *fcmd = (struct xnvme_spec_transports_cmd *)&ctx.cmd;
	struct xnvme_spec_sgl_descriptor *sgl = &fcmd->connect.sgl1;
	void *handle = NULL;
	void *buffer = NULL;
	uint32_t rkey = 0;
	int err;

	buffer = xnvme_buf_virt_alloc(0x1000, sizeof(*connect_data));
	if (!buffer) {
		TRANSPORT_ERROR("FAILED: xnvme_buf_virt_alloc() for connect_data");
		qpair->state = XNVME_TRANSPORT_QPAIR_STATE_ERROR;
		return -ENOMEM;
	}

	err = xnvme_transport_ctrlr_reg(qpair->ctrlr, buffer, sizeof(*connect_data), &handle,
					&rkey);
	if (err) {
		TRANSPORT_ERROR("FAILED: ctrlr_reg() for connect_data");
		err = -EIO;
		goto free_data_buffer;
	}
	TRANSPORT_DEBUG(
		"INFO: Fabric connect data buffer allocated at %p, size: %zu, rkey: %" PRIu32,
		buffer, sizeof(*connect_data), rkey);

	cmd->common.opcode = XNVME_SPEC_TRANSPORTS_OPC; /* Fabric command opcode */
	/* There are no fused fabrics commands, so FUSE stays zero */
	cmd->common.flags = xnvme_bits_set(0, XNVME_SPEC_CMD_COMMON_FLAGS_PSDT,
					   XNVME_SPEC_PSDT_SGL_MPTR_CONTIGUOUS);

	fcmd->connect.fctype = XNVME_SPEC_TRANSPORTS_COMMAND_CONNECT;
	fcmd->connect.recfmt = 0;
	fcmd->connect.qid = qpair->attr.qid;
	fcmd->connect.sqsize = qpair->attr.qsize - 1;
	fcmd->connect.kato = 0;     /* No keep-alive timeout */
	fcmd->connect.nvmsetid = 0; /* Default NVM set */
	fcmd->connect.cattr =
		xnvme_bits_set(0, XNVME_SPEC_TRANSPORTS_CONNECT_CMD_CATTR_INDIVIOQDELS, 1);

	_transport_fabric_connect_data_init(qpair, buffer);

	xnvme_transport_keyed_sgl_init(sgl, buffer, sizeof(*connect_data), rkey);

	err = xnvme_transport_qpair_submit_sync(qpair, &ctx);
	if (err) {
		TRANSPORT_ERROR("FAILED: qpair_submit_sync() for Fabric Connect, err: %d", err);
		qpair->state = XNVME_TRANSPORT_QPAIR_STATE_ERROR;
		goto dereg_data_buffer;
	}

	TRANSPORT_DEBUG("INFO: Handling fabric connect completion");
	xnvme_transport_hexdump_range(TRANSPORT_DEBUG_CATEGORY_FABRICS, &ctx.cpl, sizeof(ctx.cpl));
	_transport_fabric_handle_connect(qpair, &ctx.cpl);

	if (qpair->state == XNVME_TRANSPORT_QPAIR_STATE_ERROR) {
		TRANSPORT_ERROR("FAILED: QPair in error state after fabric connect");
		err = -ECONNREFUSED;
		goto dereg_data_buffer;
	}

	xnvme_transport_ctrlr_dereg(qpair->ctrlr, handle);

	xnvme_buf_virt_free(buffer);
	return 0;

dereg_data_buffer:
	xnvme_transport_ctrlr_dereg(qpair->ctrlr, handle);
free_data_buffer:
	xnvme_buf_virt_free(buffer);
	qpair->state = XNVME_TRANSPORT_QPAIR_STATE_ERROR;
	return err;
}

static inline void
_transport_sleep_ns(uint64_t ns)
{
	struct timespec ts;
	ts.tv_sec = ns / 1000000000;
	ts.tv_nsec = ns % 1000000000;
	nanosleep(&ts, NULL);
}

// Polls CSTS.RDY until it equals @rdy, backing off exponentially, or fails with -ETIMEDOUT
static int
_transport_fabric_wait_ready(struct xnvme_transport_qpair *admin_qpair, bool rdy,
			     uint64_t timeout_ns, struct xnvme_spec_ctrlr_csts *csts)
{
	uint64_t start = _xnvme_timer_clock_sample();
	uint64_t interval = TRANSPORT_FABRIC_RETRY_MIN_NS;
	int err;

	for (;;) {
		uint64_t elapsed;

		err = _transport_fabric_prop_get32(admin_qpair, XNVME_SPEC_TRANSPORTS_PROP_CSTS,
						   &csts->raw);
		if (err) {
			return err;
		}

		if (!!xnvme_bits_get(csts->sts, XNVME_SPEC_CTRLR_CSTS_STS_RDY) == rdy) {
			return 0;
		}

		elapsed = _xnvme_timer_clock_sample() - start;
		if (elapsed > timeout_ns) {
			TRANSPORT_ERROR("FAILED: CSTS.RDY not %u after %" PRIu64 " ns", rdy,
					elapsed);
			return -ETIMEDOUT;
		}

		_transport_sleep_ns(interval);
		interval = RETRY_CLAMP(interval * 2, TRANSPORT_FABRIC_READY_MAX_NS);
	}
}

static inline uint64_t
_transport_calculate_timeout(struct xnvme_spec_ctrlr_cap cap)
{
	uint64_t timeout_ns = cap.to ? cap.to * TRANSPORT_FABRIC_CAP_TO_UNIT_NS
				     : TRANSPORT_FABRIC_READY_TIMEOUT_NS;
	timeout_ns = timeout_ns < TRANSPORT_FABRIC_READY_TIMEOUT_NS
			     ? TRANSPORT_FABRIC_READY_TIMEOUT_NS
			     : timeout_ns;
	return timeout_ns;
}

static inline int
_transport_calculate_css(struct xnvme_spec_ctrlr_cap cap, struct xnvme_spec_ctrlr_cc *cc)
{
	uint64_t iocss = xnvme_bits_get(cap.cdw1, XNVME_SPEC_CTRLR_CAP_CDW1_IOCSS);
	uint64_t ncss = xnvme_bits_get(cap.cdw1, XNVME_SPEC_CTRLR_CAP_CDW1_NCSS);
	uint64_t noiocss = xnvme_bits_get(cap.cdw1, XNVME_SPEC_CTRLR_CAP_CDW1_NOIOCSS);
	uint64_t css;

	if (iocss) {
		css = XNVME_SPEC_CC_CSS_ALL_IO_SUPPORTED;
	} else if (ncss) {
		css = XNVME_SPEC_CC_CSS_NVM_IO_SUPPORTED;
	} else if (noiocss) {
		css = XNVME_SPEC_CC_CSS_ADMIN_ONLY_SUPPORTED;
	} else {
		TRANSPORT_ERROR(
			"FAILED: unsupported IO command set configuration, cap.css: 0x%" PRIx64,
			noiocss | iocss | ncss);
		return -EINVAL;
	}
	cc->cdw0 = xnvme_bits_set(cc->cdw0, XNVME_SPEC_CTRLR_CC_CDW0_CSS, css);

	return 0;
}

int
xnvme_transport_fabric_enable(struct xnvme_transport_ctrlr *ctrlr,
			      struct xnvme_transport_qpair *admin_qpair, bool is_discovery,
			      uint16_t *mqes)
{
	struct xnvme_spec_ctrlr_cap cap;
	struct xnvme_spec_ctrlr_cc cc;
	struct xnvme_spec_ctrlr_csts csts = {0};
	uint64_t timeout_ns;
	int err;

	// determine controller capabilities
	err = _transport_fabric_prop_get64(admin_qpair, XNVME_SPEC_TRANSPORTS_PROP_CAP, &cap.raw);
	if (err) {
		return err;
	}
	if (mqes) {
		*mqes = cap.mqes;
	}

	err = _transport_fabric_prop_get32(admin_qpair, XNVME_SPEC_TRANSPORTS_PROP_CC, &cc.raw);
	if (err) {
		return err;
	}

	TRANSPORT_DEBUG("INFO: Controller CAP:");
	xnvme_transport_hexdump_range(TRANSPORT_DEBUG_CATEGORY_FABRICS, &cap, sizeof(cap));

	TRANSPORT_DEBUG("INFO: Controller CC:");
	xnvme_transport_hexdump_range(TRANSPORT_DEBUG_CATEGORY_FABRICS, &cc, sizeof(cc));

	// calculate timeout
	timeout_ns = _transport_calculate_timeout(cap);

	// A controller that is already enabled must be left as is, as disabling it is not handled
	// here
	if (xnvme_bits_get(cc.cdw0, XNVME_SPEC_CTRLR_CC_CDW0_EN)) {
		return 0;
	}

	// host configures controller settings
	if (is_discovery) {
		cc = (struct xnvme_spec_ctrlr_cc){0}; // Cleared for discovery controllers
	} else {
		err = _transport_calculate_css(cap, &cc);
		if (err) {
			return err;
		}

		// memory page size, default to 4KiB
		cc.cdw0 = xnvme_bits_set(cc.cdw0, XNVME_SPEC_CTRLR_CC_CDW0_MPS, 0b000);
		// round-robin
		cc.cdw0 = xnvme_bits_set(cc.cdw0, XNVME_SPEC_CTRLR_CC_CDW0_AMS, 0b000);

		// NVME specification allows different queue entry sizes, but
		// spdk only allows 64b for IO Submission Queue Entries
		// IO Submission Queue Entry Size, 64 bytes
		cc.cdw0 = xnvme_bits_set(cc.cdw0, XNVME_SPEC_CTRLR_CC_CDW0_IOSQES, 0b0110);
		// IO Completion Queue Entry Size, 16 bytes
		cc.cdw0 = xnvme_bits_set(cc.cdw0, XNVME_SPEC_CTRLR_CC_CDW0_IOCQES, 0b0100);
	}

	// enable controller
	cc.cdw0 = xnvme_bits_set(cc.cdw0, XNVME_SPEC_CTRLR_CC_CDW0_EN, 1);
	err = _transport_fabric_prop_set32(admin_qpair, XNVME_SPEC_TRANSPORTS_PROP_CC, cc.raw);
	if (err) {
		return err;
	}

	err = _transport_fabric_wait_ready(admin_qpair, true, timeout_ns, &csts);
	if (err) {
		return err;
	}

	TRANSPORT_DEBUG("INFO: Controller CSTS:");
	xnvme_transport_hexdump_range(TRANSPORT_DEBUG_CATEGORY_FABRICS, &csts, sizeof(csts));

	TRANSPORT_DEBUG("INFO: Controller CSTS Ready: %" PRIu64,
			xnvme_bits_get(csts.sts, XNVME_SPEC_CTRLR_CSTS_STS_RDY));

	if (is_discovery) {
		err = xnvme_transport_discovery_get_log(ctrlr, admin_qpair);
		if (err) {
			TRANSPORT_ERROR("FAILED: xnvme_transport_discovery_get_log(), err: %d",
					err);
			return err;
		}
		TRANSPORT_DEBUG("INFO: Successfully retrieved discovery log");
	}

	return 0;
}
