// Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
// SPDX-License-Identifier: Apache-2.0
#include <stdio.h>
#include <errno.h>
#include <liblblk.h>
#include <libxnvmec.h>

static int
boilerplate(struct xnvmec *cli, uint8_t **wbuf, uint8_t **rbuf,
	    size_t *buf_nbytes, uint64_t *mdts_naddr, uint32_t *nsid,
	    uint64_t *rng_slba, uint64_t *rng_elba)
{
	struct xnvme_dev *dev = cli->args.dev;
	const struct xnvme_geo *geo = cli->args.geo;
	uint64_t rng_naddr;

	*nsid = xnvme_dev_get_nsid(cli->args.dev);

	*rng_slba = cli->args.slba;
	*rng_elba = cli->args.elba;

	//*mdts_naddr = XNVME_MIN(geo->mdts_nbytes / geo->nbytes, 256);
	*mdts_naddr = 64;
	*buf_nbytes = (*mdts_naddr) * geo->nbytes;

	// Construct a range if none is given
	if (!(cli->given[XNVMEC_OPT_SLBA] && cli->given[XNVMEC_OPT_ELBA])) {
		*rng_slba = 0;
		*rng_elba = (1 << 28) / geo->nbytes;	// Around 256MB
	}
	rng_naddr = *rng_elba - *rng_slba + 1;

	// Verify range
	if (*rng_elba <= *rng_slba) {
		xnvmec_perror("Invalid range: [rng_slba,rng_elba]");
		errno = EINVAL;
		return 1;
	}
	// TODO: verify that the range is sufficiently large

	*wbuf = xnvme_buf_alloc(dev, *buf_nbytes, NULL);
	if (!*wbuf) {
		xnvmec_perror("xnvme_buf_alloc()");
		return 1;
	}

	*rbuf = xnvme_buf_alloc(dev, *buf_nbytes, NULL);
	if (!*rbuf) {
		xnvmec_perror("xnvme_buf_alloc()");
		return 1;
	}

	xnvmec_pinfo("nsid: 0x%x", *nsid);
	xnvmec_pinfo("range: { slba: 0x%016x, elba: 0x%016x, naddr: %zu }",
		     *rng_slba, *rng_elba, rng_naddr);
	xnvmec_pinfo("buf_nbytes: %zu", *buf_nbytes);
	xnvmec_pinfo("mdts_naddr: %zu", *mdts_naddr);

	return 0;
}

/**
 * 0) Fill wbuf with '!'
 * 1) Write the entire LBA range [slba, elba] using wbuf
 * 2) Fill wbuf with a repeating sequence of letters A to Z
 * 3) Scatter the content of wbuf within [slba,elba]
 * 4) Read, with exponential stride, within [slba,elba] using rbuf
 * 5) Verify that the content of rbuf is the same as wbuf
 */
static int
sub_io(struct xnvmec *cli)
{
	struct xnvme_dev *dev = cli->args.dev;
	const struct xnvme_geo *geo = cli->args.geo;
	uint32_t nsid;
	uint64_t rng_slba, rng_elba, mdts_naddr;
	size_t buf_nbytes;
	uint8_t *wbuf = NULL, *rbuf = NULL;
	int rcode = 1;					// Assume error

	if (boilerplate(cli, &wbuf, &rbuf, &buf_nbytes, &mdts_naddr, &nsid,
			&rng_slba, &rng_elba)) {
		goto exit;
	}

	xnvmec_pinfo("Writing '!' to LBA range [slba,elba]");
	memset(wbuf, '!', buf_nbytes);
	for (uint64_t slba = rng_slba; slba < rng_elba; slba += mdts_naddr) {
		uint64_t nlb = XNVME_MIN(rng_elba - slba, mdts_naddr) - 1;
		struct xnvme_req req = { 0 };
		int err = 0;

		err = xnvme_cmd_write(dev, nsid, slba, nlb, wbuf, NULL,
				      XNVME_CMD_SYNC, &req);
		if (err || xnvme_req_cpl_status(&req)) {
			xnvmec_perror("xnvme_cmd_write()");
			xnvmec_pinfo("err: 0x%x, slba: 0x%016x", err, slba);
			xnvme_req_pr(&req, XNVME_PR_DEF);
			goto exit;
		}
	}

	xnvmec_pinfo("Writing payload scattered within LBA range [slba,elba]");
	xnvmec_buf_fill(wbuf, buf_nbytes);
	for (uint64_t count = 0; count < mdts_naddr; ++count) {
		size_t wbuf_ofz = count * geo->nbytes;
		uint64_t slba = rng_slba + count * 4;
		struct xnvme_req req = { 0 };
		int err = 0;

		err = xnvme_cmd_write(dev, nsid, slba, 0, wbuf + wbuf_ofz, NULL,
				      XNVME_CMD_SYNC, &req);
		if (err || xnvme_req_cpl_status(&req)) {
			xnvmec_perror("xnvme_cmd_write()");
			xnvmec_pinfo("err: 0x%x, slba: 0x%016x", err, slba);
			xnvme_req_pr(&req, XNVME_PR_DEF);
			goto exit;
		}
	}

	xnvmec_pinfo("Read scattered payload within LBA range [slba,elba]");

	memset(rbuf, 0, buf_nbytes);
	for (uint64_t count = 0; count < mdts_naddr; ++count) {
		size_t rbuf_ofz = count * geo->nbytes;
		uint64_t slba = rng_slba + count * 4;
		struct xnvme_req req = { 0 };
		int err = 0;

		err = xnvme_cmd_read(dev, nsid, slba, 0, rbuf + rbuf_ofz, NULL,
				     XNVME_CMD_SYNC, &req);
		if (err || xnvme_req_cpl_status(&req)) {
			xnvmec_perror("xnvme_cmd_read()");
			xnvmec_pinfo("err: 0x%x, slba: 0x%016x", err, slba);
			xnvme_req_pr(&req, XNVME_PR_DEF);
			goto exit;
		}
	}

	xnvmec_pinfo("Comparing wbuf and rbuf");
	if (xnvmec_buf_diff(wbuf, rbuf, buf_nbytes)) {
		xnvmec_buf_diff_pr(wbuf, rbuf, buf_nbytes, XNVME_PR_DEF);
		goto exit;
	}

	rcode = 0;

exit:
	xnvme_buf_free(dev, wbuf);
	xnvme_buf_free(dev, rbuf);

	return rcode;
}

/**
 * 0) Fill wbuf with '!'
 *
 * 1) Write the entire LBA range [slba, elba] using wbuf
 *
 * 2) Fill wbuf with a repeating sequence of letters A to Z
 *
 * 3) Scatter the content of wbuf within [slba,elba]
 *
 * 4) Read, with exponential stride, within [slba,elba] using rbuf
 *
 * 5) Verify that the content of rbuf is the same as wbuf
 */
static int
test_scopy(struct xnvmec *cli)
{
	struct xnvme_dev *dev = cli->args.dev;
	const struct xnvme_geo *geo = cli->args.geo;
	uint32_t nsid;
	uint64_t rng_slba, rng_elba, mdts_naddr;
	size_t buf_nbytes;
	uint8_t *wbuf = NULL, *rbuf = NULL;
	int rcode = 1;					// Assume error

	struct lblk_source_range *sranges = NULL;	// For the copy-payload
	uint64_t sdlba = 0;

	if (boilerplate(cli, &wbuf, &rbuf, &buf_nbytes, &mdts_naddr, &nsid,
			&rng_slba, &rng_elba)) {
		goto exit;
	}

	sranges = xnvme_buf_alloc(dev, sizeof(*sranges), NULL);
	if (!wbuf) {
		xnvmec_perror("xnvme_buf_alloc()");
		goto exit;
	}
	memset(sranges, 0, sizeof(*sranges));

	// Copy to the end of [slba,elba]
	sdlba = rng_elba - mdts_naddr;

	lblk_idfy_ctrlr_pr((struct lblk_idfy_ctrlr *)xnvme_dev_get_ctrlr(dev),
			   XNVME_PR_DEF);
	lblk_idfy_ns_pr((struct lblk_idfy_ns *)xnvme_dev_get_ns(dev),
			XNVME_PR_DEF);

	xnvmec_pinfo("Writing '!' to LBA range [slba,elba]");
	memset(wbuf, '!', buf_nbytes);
	for (uint64_t slba = rng_slba; slba < rng_elba; slba += mdts_naddr) {
		uint64_t nlb = XNVME_MIN(rng_elba - slba, mdts_naddr) - 1;
		struct xnvme_req req = { 0 };
		int err = 0;

		err = xnvme_cmd_write(dev, nsid, slba, nlb, wbuf, NULL,
				      XNVME_CMD_SYNC, &req);
		if (err || xnvme_req_cpl_status(&req)) {
			xnvmec_perror("xnvme_cmd_write()");
			xnvmec_pinfo("err: 0x%x, slba: 0x%016x", err, slba);
			xnvme_req_pr(&req, XNVME_PR_DEF);
			goto exit;
		}
	}

	xnvmec_pinfo("Writing payload scattered within LBA range [slba,elba]");
	xnvmec_buf_fill(wbuf, buf_nbytes);
	for (uint64_t count = 0; count < mdts_naddr; ++count) {
		size_t wbuf_ofz = count * geo->nbytes;
		uint64_t slba = rng_slba + count * 4;
		struct xnvme_req req = { 0 };
		int err = 0;

		sranges->entry[count].slba = slba;
		sranges->entry[count].nlb = 0;

		err = xnvme_cmd_write(dev, nsid, slba, 0, wbuf + wbuf_ofz, NULL,
				      XNVME_CMD_SYNC, &req);
		if (err || xnvme_req_cpl_status(&req)) {
			xnvmec_perror("xnvme_cmd_write()");
			xnvmec_pinfo("err: 0x%x, slba: 0x%016x", err, slba);
			xnvme_req_pr(&req, XNVME_PR_DEF);
			goto exit;
		}
	}

	{
		struct xnvme_req req = { 0 };
		int err = 0;
		uint8_t nr = mdts_naddr - 1;

		xnvmec_pinfo("scopy sranges to sdlba: 0x%016x", sdlba);
		lblk_source_range_pr(sranges, nr, XNVME_PR_DEF);

		err = lblk_cmd_scopy(dev, nsid, sdlba, sranges->entry, nr,
				     XNVME_CMD_SYNC, &req);
		if (err || xnvme_req_cpl_status(&req)) {
			xnvmec_perror("lblk_cmd_scopy()");
			xnvmec_pinfo("err: 0x%x, sdlba: 0x%016x, nr: %u",
				     err, sdlba, nr);
			xnvme_req_pr(&req, XNVME_PR_DEF);
			goto exit;
		}

		xnvmec_pinfo("read sdlba: 0x%016x", sdlba);
		memset(rbuf, 0, buf_nbytes);
		err = xnvme_cmd_read(dev, nsid, sdlba, nr, rbuf, NULL,
				     XNVME_CMD_SYNC, &req);
		if (err || xnvme_req_cpl_status(&req)) {
			xnvmec_perror("xnvme_cmd_read()");
			xnvmec_pinfo("err: 0x%x, sdlba: 0x%016x", err, sdlba);
			xnvme_req_pr(&req, XNVME_PR_DEF);
			goto exit;
		}

		xnvmec_pinfo("Comparing wbuf and rbuf");
		if (xnvmec_buf_diff(wbuf, rbuf, buf_nbytes)) {
			xnvmec_buf_diff_pr(wbuf, rbuf, buf_nbytes, XNVME_PR_DEF);
			goto exit;
		}
	}

	rcode = 0;

exit:
	xnvme_buf_free(dev, wbuf);
	xnvme_buf_free(dev, rbuf);
	xnvme_buf_free(dev, sranges);

	return rcode;
}


//
// Command-Line Interface (CLI) definition
//
static struct xnvmec_sub subs[] = {
	{
		"io",
		"Basic Verification of being able to read, what was written",
		"Basic Verification of being able to read, what was written",
		sub_io,
		{
			{XNVMEC_OPT_URI, XNVMEC_POSA},
			{XNVMEC_OPT_SLBA, XNVMEC_LOPT},
			{XNVMEC_OPT_ELBA, XNVMEC_LOPT},
		}
	},
	{
		"scopy",
		"Basic Verification of the Simple-Copy Command",
		"Basic Verification of the Simple-Copy Command",
		test_scopy, {
			{XNVMEC_OPT_URI, XNVMEC_POSA},
			{XNVMEC_OPT_LBA, XNVMEC_LOPT},
		}
	},
};

static struct xnvmec cli = {
	.title = "Basic LBLK Verification",
	.descr_short = "Basic LBLK Verification",
	.subs = subs,
	.nsubs = sizeof(subs) / sizeof(subs[0]),
};

int
main(int argc, char **argv)
{
	return xnvmec(&cli, argc, argv, XNVMEC_INIT_DEV_OPEN);
}
