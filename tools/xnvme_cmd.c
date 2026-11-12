// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <errno.h>
#include <getopt.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libxnvme.h>
#include "xnvme_cmd.h"

enum {
	CLI_OPT_BE = 0x100,
	CLI_OPT_ADMIN,
	CLI_OPT_SYNC,
	CLI_OPT_ASYNC,
	CLI_OPT_DEV_NSID,
	CLI_OPT_HOMI_ID,
	CLI_OPT_VEC_CNT,
	CLI_OPT_NO_RETRIES,
	CLI_OPT_OFV,
	CLI_OPT_HELP,
};

static const struct option g_cli_cmd_opts[] = {
	// nvme-cli 2.16 admin-passthru and io-passthru, which share one set
	{"opcode", required_argument, NULL, 'O'},
	{"flags", required_argument, NULL, 'f'},
	{"prefill", required_argument, NULL, 'p'},
	{"rsvd", required_argument, NULL, 'R'},
	{"namespace-id", required_argument, NULL, 'n'},
	{"data-len", required_argument, NULL, 'l'},
	{"metadata-len", required_argument, NULL, 'm'},
	{"cdw2", required_argument, NULL, '2'},
	{"cdw3", required_argument, NULL, '3'},
	{"cdw10", required_argument, NULL, '4'},
	{"cdw11", required_argument, NULL, '5'},
	{"cdw12", required_argument, NULL, '6'},
	{"cdw13", required_argument, NULL, '7'},
	{"cdw14", required_argument, NULL, '8'},
	{"cdw15", required_argument, NULL, '9'},
	{"input-file", required_argument, NULL, 'i'},
	{"metadata", required_argument, NULL, 'M'},
	{"raw-binary", no_argument, NULL, 'b'},
	{"show-command", no_argument, NULL, 's'},
	{"dry-run", no_argument, NULL, 'd'},
	{"read", no_argument, NULL, 'r'},
	{"write", no_argument, NULL, 'w'},
	{"latency", no_argument, NULL, 'T'},
	{"timeout", required_argument, NULL, 't'},
	{"output-format", required_argument, NULL, 'o'},
	{"verbose", no_argument, NULL, 'v'},
	{"no-retries", no_argument, NULL, CLI_OPT_NO_RETRIES},
	{"output-format-version", required_argument, NULL, CLI_OPT_OFV},
	// xNVMe's own: long options only, so none takes a letter nvme-cli uses
	{"be", required_argument, NULL, CLI_OPT_BE},
	{"admin", required_argument, NULL, CLI_OPT_ADMIN},
	{"sync", required_argument, NULL, CLI_OPT_SYNC},
	{"async", required_argument, NULL, CLI_OPT_ASYNC},
	{"dev-nsid", required_argument, NULL, CLI_OPT_DEV_NSID},
	{"homi-id", required_argument, NULL, CLI_OPT_HOMI_ID},
	{"shm_id", required_argument, NULL, CLI_OPT_HOMI_ID},
	{"vec-cnt", required_argument, NULL, CLI_OPT_VEC_CNT},
	{"help", no_argument, NULL, CLI_OPT_HELP},
	{NULL, 0, NULL, 0},
};

static void
cli_cmd_usage(const char *name, const char *counterpart)
{
	printf("Usage: xnvme %s <uri> [OPTIONS]\n\n", name);
	printf("Send a user-defined command to the device at <uri>, any xNVMe URI: a\n");
	printf("character or block device, a BDF, or a fabrics address. The options are\n");
	printf("those of %s, with the same names and letters:\n\n", counterpart);
	printf("  -O, --opcode=NUM       -f, --flags=NUM        -p, --prefill=NUM\n");
	printf("  -R, --rsvd=NUM         -n, --namespace-id=NUM -l, --data-len=NUM\n");
	printf("  -m, --metadata-len=NUM -2, --cdw2=NUM         -3, --cdw3=NUM\n");
	printf("  -4 .. -9, --cdw10=NUM .. --cdw15=NUM          -i, --input-file=FILE\n");
	printf("  -M, --metadata=FILE    -b, --raw-binary       -s, --show-command\n");
	printf("  -d, --dry-run          -r, --read             -w, --write\n");
	printf("  -T, --latency          -o, --output-format=FMT -v, --verbose\n");
	printf("  --no-retries\n\n");
	printf("but -t, --timeout, as the synchronous path has no per-command timeout. Then,\n");
	printf("choosing the path: --be, --admin, --sync, --async, --dev-nsid and\n");
	printf("--homi-id or --shm_id%s.\n", strcmp(name, "cmd-iov")
						     ? ""
						     : ", and --vec-cnt=NUM, the number of equal "
						       "segments the data is sent in");
}

static uint64_t
cli_num(const char *arg, bool *ok)
{
	char *end = NULL;
	unsigned long long val;

	errno = 0;
	val = strtoull(arg, &end, 0);
	*ok = *ok && !errno && end && end != arg && !*end;

	return val;
}

static void
cli_hexdump(const uint8_t *buf, size_t nbytes)
{
	printf("       0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f\n");
	for (size_t ofs = 0; ofs < nbytes; ofs += 16) {
		printf("%04zx:", ofs);
		for (size_t i = 0; i < 16; ++i) {
			if (ofs + i < nbytes) {
				printf(" %02x", buf[ofs + i]);
			} else {
				printf("   ");
			}
		}
		printf(" \"");
		for (size_t i = 0; i < 16 && ofs + i < nbytes; ++i) {
			uint8_t c = buf[ofs + i];

			putchar(c >= 0x20 && c < 0x7f ? c : '.');
		}
		printf("\"\n");
	}
}

static int
cli_file_load(const char *path, void *buf, size_t nbytes)
{
	FILE *fp = path ? fopen(path, "rb") : stdin;
	size_t nread;

	if (!fp) {
		return -errno;
	}
	nread = fread(buf, 1, nbytes, fp);
	if (path) {
		fclose(fp);
	}

	return nread == nbytes ? 0 : -EIO;
}

static int
cli_file_save(const char *path, const void *buf, size_t nbytes)
{
	FILE *fp = fopen(path, "wb");
	size_t nwritten;

	if (!fp) {
		return -errno;
	}
	nwritten = fwrite(buf, 1, nbytes, fp);
	fclose(fp);

	return nwritten == nbytes ? 0 : -EIO;
}

int
cli_cmd(enum cli_cmd_kind kind, int argc, char **argv)
{
	const char *name =
		kind == CLI_CMD_ADMIN ? "cmd-admin" : (kind == CLI_CMD_IO ? "cmd-io" : "cmd-iov");
	const char *counterpart =
		kind == CLI_CMD_ADMIN ? "nvme admin-passthru" : "nvme io-passthru";
	struct xnvme_opts opts = {0};
	uint64_t cdw[16] = {0}, opcode = 0, flags = 0, prefill = 0, rsvd = 0, data_len = 0,
		 meta_len = 0, vec_cnt = 1;
	const char *input = NULL, *metafile = NULL, *ofmt = "normal", *uri;
	bool have_opcode = false, raw = false, show = false, dry = false, rd = false, wr = false,
	     latency = false, ok = true;
	struct xnvme_dev *dev = NULL;
	struct xnvme_cmd_ctx ctx;
	uint8_t *dbuf = NULL, *mbuf = NULL;
	struct iovec *iov = NULL;
	struct xnvme_timer timer = {0};
	uint32_t *dwords;
	int opt, err = 0;

	xnvme_opts_set_defaults(&opts);

	optind = 1;
	opterr = 1;
	while ((opt = getopt_long(argc, argv, "O:f:p:R:n:l:m:2:3:4:5:6:7:8:9:i:M:bsdrwTt:o:v",
				  g_cli_cmd_opts, NULL)) != -1) {
		switch (opt) {
		case 'O':
			opcode = cli_num(optarg, &ok);
			have_opcode = true;
			break;
		case 'f':
			flags = cli_num(optarg, &ok);
			break;
		case 'p':
			prefill = cli_num(optarg, &ok);
			break;
		case 'R':
			rsvd = cli_num(optarg, &ok);
			break;
		case 'n':
			cdw[1] = cli_num(optarg, &ok);
			break;
		case 'l':
			data_len = cli_num(optarg, &ok);
			break;
		case 'm':
			meta_len = cli_num(optarg, &ok);
			break;
		case '2':
		case '3':
			cdw[opt - '0'] = cli_num(optarg, &ok);
			break;
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			cdw[opt - '4' + 10] = cli_num(optarg, &ok);
			break;
		case 'i':
			input = optarg;
			break;
		case 'M':
			metafile = optarg;
			break;
		case 'b':
			raw = true;
			break;
		case 's':
			show = true;
			break;
		case 'd':
			dry = true;
			break;
		case 'r':
			rd = true;
			break;
		case 'w':
			wr = true;
			break;
		case 'T':
			latency = true;
			break;
		case 't':
			fprintf(stderr,
				"xnvme %s: --timeout is not supported: xNVMe's synchronous "
				"command path has no per-command timeout\n",
				name);
			return 1;
		case 'o':
			ofmt = optarg;
			break;
		case 'v':
		case CLI_OPT_NO_RETRIES:
		case CLI_OPT_OFV:
			break;
		case CLI_OPT_BE:
			opts.be = optarg;
			break;
		case CLI_OPT_ADMIN:
			opts.admin = optarg;
			break;
		case CLI_OPT_SYNC:
			opts.sync = optarg;
			break;
		case CLI_OPT_ASYNC:
			opts.async = optarg;
			break;
		case CLI_OPT_DEV_NSID:
			opts.nsid = (uint32_t)cli_num(optarg, &ok);
			break;
		case CLI_OPT_HOMI_ID:
			// As xnvme_cli fills both: SPDK and multi-process uPCIe read shm_id
			opts.homi_id = (uint32_t)cli_num(optarg, &ok);
			opts.shm_id = opts.homi_id;
			break;
		case CLI_OPT_VEC_CNT:
			vec_cnt = cli_num(optarg, &ok);
			break;
		case CLI_OPT_HELP:
			cli_cmd_usage(name, counterpart);
			return 0;
		default:
			cli_cmd_usage(name, counterpart);
			return 1;
		}
	}
	if (!ok || optind != argc - 1 || !have_opcode || opcode > 0xFF || flags > 0xFF ||
	    prefill > 0xFF) {
		fprintf(stderr,
			"xnvme %s: needs <uri> and --opcode, and numbers that fit; see "
			"'xnvme %s --help'\n",
			name, name);
		return 1;
	}
	if (rsvd) {
		fprintf(stderr,
			"xnvme %s: --rsvd is not supported: an xNVMe command has no field "
			"for it\n",
			name);
		return 1;
	}
	if (strcmp(ofmt, "normal") && strcmp(ofmt, "binary")) {
		fprintf(stderr,
			"xnvme %s: --output-format '%s' is not supported; normal or binary\n",
			name, ofmt);
		return 1;
	}
	if (kind == CLI_CMD_IOV && (!vec_cnt || !data_len || data_len % vec_cnt)) {
		fprintf(stderr, "xnvme %s: --data-len must split into --vec-cnt equal segments\n",
			name);
		return 1;
	}
	raw = raw || !strcmp(ofmt, "binary");
	uri = argv[optind];

	cdw[0] = opcode | (flags & 0x3) << 8;
	if (show || dry) {
		printf("opcode       : 0x%02" PRIx64 "\n", opcode);
		printf("flags        : 0x%02" PRIx64 "\n", flags);
		printf("nsid         : 0x%08" PRIx64 "\n", cdw[1]);
		printf("cdw2         : 0x%08" PRIx64 "\n", cdw[2]);
		printf("cdw3         : 0x%08" PRIx64 "\n", cdw[3]);
		printf("data_len     : 0x%08" PRIx64 "\n", data_len);
		printf("metadata_len : 0x%08" PRIx64 "\n", meta_len);
		for (int i = 10; i <= 15; ++i) {
			printf("cdw%-10d: 0x%08" PRIx64 "\n", i, cdw[i]);
		}
	}
	if (dry) {
		return 0;
	}

	dev = xnvme_dev_open(uri, &opts);
	if (!dev) {
		fprintf(stderr, "xnvme %s: opening '%s' failed: %s\n", name, uri, strerror(errno));
		return 1;
	}
	if (data_len) {
		dbuf = xnvme_buf_alloc(dev, data_len);
		if (!dbuf) {
			err = -ENOMEM;
			goto exit;
		}
		memset(dbuf, (int)prefill, data_len);
		if (wr) {
			err = cli_file_load(input, dbuf, data_len);
			if (err) {
				fprintf(stderr, "xnvme %s: reading the data failed: %s\n", name,
					strerror(-err));
				goto exit;
			}
		}
	}
	if (meta_len) {
		mbuf = xnvme_buf_alloc(dev, meta_len);
		if (!mbuf) {
			err = -ENOMEM;
			goto exit;
		}
		memset(mbuf, (int)prefill, meta_len);
		if (wr && metafile) {
			err = cli_file_load(metafile, mbuf, meta_len);
			if (err) {
				goto exit;
			}
		}
	}

	xnvme_dev_get_cmd_ctx(dev, &ctx);
	dwords = (uint32_t *)&ctx.cmd;
	for (int i = 0; i < 16; ++i) {
		if (i < 4 || i >= 10) {
			dwords[i] = (uint32_t)cdw[i];
		}
	}

	xnvme_timer_start(&timer);
	switch (kind) {
	case CLI_CMD_ADMIN:
		err = xnvme_cmd_pass_admin(&ctx, dbuf, data_len, mbuf, meta_len);
		break;
	case CLI_CMD_IO:
		err = xnvme_cmd_pass(&ctx, dbuf, data_len, mbuf, meta_len);
		break;
	case CLI_CMD_IOV:
		iov = calloc(vec_cnt, sizeof(*iov));
		if (!iov) {
			err = -ENOMEM;
			break;
		}
		for (uint64_t i = 0; i < vec_cnt; ++i) {
			iov[i].iov_base = dbuf + i * (data_len / vec_cnt);
			iov[i].iov_len = data_len / vec_cnt;
		}
		err = xnvme_cmd_pass_iov(&ctx, iov, vec_cnt, data_len, mbuf, meta_len);
		break;
	}
	xnvme_timer_stop(&timer);

	if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
		fprintf(stderr, "xnvme %s: the command failed: err(%d), sct(0x%x), sc(0x%x)\n",
			name, err,
			(unsigned int)xnvme_bits_get(ctx.cpl.status, XNVME_SPEC_CQE_STATUS_SCT),
			(unsigned int)xnvme_bits_get(ctx.cpl.status, XNVME_SPEC_CQE_STATUS_SC));
		err = err ? err : -EIO;
		goto exit;
	}
	if (!raw) {
		printf("%s Command is Success and result: 0x%08x\n",
		       kind == CLI_CMD_ADMIN ? "Admin" : "IO", ctx.cpl.cdw0);
	}
	if (latency) {
		printf("latency: %" PRIu64 " us\n", (uint64_t)xnvme_timer_elapsed_usecs(&timer));
	}
	if (rd && dbuf) {
		if (input) {
			err = cli_file_save(input, dbuf, data_len);
		} else if (raw) {
			fwrite(dbuf, 1, data_len, stdout);
		} else {
			cli_hexdump(dbuf, data_len);
		}
	}
	if (rd && mbuf && metafile) {
		err = err ? err : cli_file_save(metafile, mbuf, meta_len);
	}

exit:
	free(iov);
	xnvme_buf_free(dev, mbuf);
	xnvme_buf_free(dev, dbuf);
	xnvme_dev_close(dev);

	return err ? 1 : 0;
}
