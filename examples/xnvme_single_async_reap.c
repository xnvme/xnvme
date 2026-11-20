// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <libxnvme.h>

/**
 * This example shows how to send a single asynchronous command and reap its completion
 *
 * - Allocate a buffer for the command payload
 * - Setup a Command Queue for reaping, with XNVME_QUEUE_REAP
 *   | Using command-contexts, attaching data of your own to each
 * - Submit the asynchronous command
 *   | Re-submission when busy
 * - Reap the completed command with xnvme_queue_reap(), until none is outstanding
 * - Teardown
 */
int
main(int argc, char **argv)
{
	struct xnvme_opts opts = {0};
	struct xnvme_dev *dev = NULL;
	uint32_t nsid;
	char *buf = NULL;
	size_t buf_nbytes;
	struct xnvme_queue *queue = NULL;
	const int qdepth = 16;
	struct xnvme_cmd_ctx *completions[16];
	uint32_t ncompleted;
	const char *tag = "the first read";
	int ret = 0, err = 0;

	xnvme_opts_set_defaults(&opts);

	if (argc < 2 || !strcmp(argv[1], "--help")) {
		printf("Usage: %s <uri>\n\nRead the first block of the device at <uri> with one "
		       "asynchronous command, reaping its completion\n",
		       argv[0]);
		return argc < 2;
	}

	dev = xnvme_dev_open(argv[1], &opts);
	if (!dev) {
		fprintf(stderr, "xnvme_dev_open(): %s\n", strerror(errno));
		return 1;
	}
	nsid = xnvme_dev_get_nsid(dev);

	// Initialize a command-queue for reaping. Completed commands are reaped and no callback is
	// called, so no callback is set
	ret = xnvme_queue_init(dev, qdepth, XNVME_QUEUE_REAP, &queue);
	if (ret) {
		fprintf(stderr, "xnvme_queue_init(): %s\n", strerror(-ret));
		xnvme_dev_close(dev);
		return 1;
	}

	buf_nbytes = xnvme_dev_get_geo(dev)->lba_nbytes;

	printf("Allocate a payload-buffer of nbytes: %zu\n", buf_nbytes);
	buf = xnvme_buf_alloc(dev, buf_nbytes);
	if (!buf) {
		err = -errno;
		fprintf(stderr, "xnvme_buf_alloc(): %s\n", strerror(-err));
		goto exit;
	}

	err = xnvme_buf_clear(buf, buf_nbytes);
	if (err) {
		fprintf(stderr, "xnvme_buf_clear(): %s\n", strerror(-err));
		goto exit;
	}

	// This block would typically be a loop as more than a single command would be submitted
	{
		struct xnvme_cmd_ctx *ctx = xnvme_queue_get_cmd_ctx(queue);

		// Attach data of your own to the command; it comes back with the command-context
		// when the command is reaped, here a string, typically a pointer to a struct
		xnvme_cmd_ctx_set_data(ctx, (void *)tag);

submit:
		// Submit a read command
		err = xnvme_nvm_read(ctx, nsid, 0x0, 0, buf, NULL);
		switch (err) {
		case 0:
			printf("Submission succeeded\n");
			break;

		// Submission failed: queue is full => reap completions and try again
		case -EBUSY:
		case -EAGAIN:
			ret = xnvme_queue_reap(queue, completions, qdepth, &ncompleted);
			for (uint32_t i = 0; i < ncompleted; ++i) {
				xnvme_queue_put_cmd_ctx(queue, completions[i]);
			}
			if (ret == -EINTR || ret == -EAGAIN || ret == -EBUSY) {
				ret = 0;
			}
			if (ret) {
				fprintf(stderr, "xnvme_queue_reap(): %s\n", strerror(-ret));
				xnvme_queue_put_cmd_ctx(queue, ctx);
				goto exit;
			}
			goto submit;

		// Submission failed: unexpected error, put the command-context back in the queue
		default:
			fprintf(stderr, "xnvme_nvm_read(): %s\n", strerror(-err));
			xnvme_queue_put_cmd_ctx(queue, ctx);
			goto exit;
		}
	}

	// Done submitting, reap until no command is outstanding as we are about to exit
	while (xnvme_queue_get_outstanding(queue)) {
		// An error reports the state of the queue, never that a command failed.
		// 'ncompleted' is assigned on every return, so the commands it counts are handled
		// before the error is looked at
		ret = xnvme_queue_reap(queue, completions, qdepth, &ncompleted);

		for (uint32_t i = 0; i < ncompleted; ++i) {
			struct xnvme_cmd_ctx *ctx = completions[i];

			if (xnvme_cmd_ctx_cpl_status(ctx)) {
				printf("Command did not complete successfully\n");
				xnvme_cmd_ctx_pr(ctx, XNVME_PR_DEF);
				err = -EIO;
			} else {
				printf("Command completed successfully: '%s'\n",
				       (const char *)xnvme_cmd_ctx_get_data(ctx));
			}

			// Completed: Put the command-context back in the queue
			xnvme_queue_put_cmd_ctx(queue, ctx);
		}

		// -EINTR, -EAGAIN and -EBUSY leave the queue intact, so reap again. With any
		// other error the queue failed, and is torn down
		if (ret == -EINTR || ret == -EAGAIN || ret == -EBUSY) {
			ret = 0;
		}
		if (ret) {
			fprintf(stderr, "xnvme_queue_reap(): %s\n", strerror(-ret));
			goto exit;
		}
	}

exit:
	xnvme_queue_term(queue);
	xnvme_buf_free(dev, buf);
	xnvme_dev_close(dev);

	return ret || err;
}
