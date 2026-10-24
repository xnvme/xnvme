// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

/*
 * The public queries of libxnvme_cplane.h, answered by the server over its socket.
 */
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <libxnvme.h>
#include <libxnvme_cplane.h>
#include <xnvme_be.h>
#include <xnvme_dev.h>

#ifdef XNVME_BE_UPCIE_ENABLED
#include <xnvme_be_upcie.h>

int
xnvme_cplane_server_alive(uint32_t cplane_id)
{
	struct nvme_cplane_msg msg;
	char path[256] = {0};
	int sock, err;

	xnvme_be_upcie_cplane_socket_path(cplane_id, path, sizeof(path));

	sock = xnvme_be_upcie_cplane_connect(path);
	if (sock < 0) {
		return (sock == -ENOENT) ? 0 : sock;
	}

	err = xnvme_be_upcie_cplane_ask_status(sock, 0, &msg);
	close(sock);

	return err ? err : 1;
}

int
xnvme_cplane_get_info(uint32_t cplane_id, struct xnvme_cplane_info *info)
{
	struct nvme_cplane_msg msg = {0};
	char path[256] = {0};
	uint32_t ndevs;
	int sock, err;

	if (!info) {
		return -EINVAL;
	}

	xnvme_be_upcie_cplane_socket_path(cplane_id, path, sizeof(path));

	sock = xnvme_be_upcie_cplane_connect(path);
	if (sock < 0) {
		return sock;
	}

	err = xnvme_be_upcie_cplane_ask_status(sock, 0, &msg);
	if (err) {
		close(sock);

		return err;
	}

	memset(info, 0, sizeof(*info));

	/* The server's count leaves out the connection asking */
	info->nconnections = msg.u.status.nclients + 1;

	ndevs = msg.u.status.ndevs;
	info->nctrlrs_held = ndevs;

	for (uint32_t i = 0; (i < ndevs) && (info->nctrlrs < XNVME_CPLANE_MAX_CTRLRS); ++i) {
		if (i) {
			err = xnvme_be_upcie_cplane_ask_status(sock, i, &msg);
			if (err) {
				close(sock);

				return err;
			}
		}

		snprintf(info->ctrlrs[info->nctrlrs], sizeof(info->ctrlrs[0]), "%s",
			 msg.u.status.bdf);
		info->nctrlrs++;
	}

	close(sock);

	return 0;
}

int
xnvme_cplane_get_ctrlr_info(const char *uri, struct xnvme_cplane_ctrlr_info *info)
{
	struct dirent *entry;
	DIR *dir;
	int sock;

	if (!uri || !info) {
		return -EINVAL;
	}

	/* Nothing registers runtimes, so each socket in /tmp is asked in turn */
	dir = opendir("/tmp");
	if (!dir) {
		return -errno;
	}

	while ((entry = readdir(dir))) {
		struct nvme_cplane_msg msg = {0};
		char path[256] = {0};
		unsigned int cplane_id;

		if (sscanf(entry->d_name, "xnvme-homi-%u.sock", &cplane_id) != 1) {
			continue;
		}

		snprintf(path, sizeof(path), "/tmp/%s", entry->d_name);

		sock = xnvme_be_upcie_cplane_connect(path);
		if (sock < 0) {
			continue; /* Gone between the readdir and the connect */
		}

		for (uint32_t i = 0; !xnvme_be_upcie_cplane_ask_status(sock, i, &msg); ++i) {
			if (strcmp(msg.u.status.bdf, uri)) {
				if ((i + 1) >= msg.u.status.ndevs) {
					break;
				}
				continue;
			}

			close(sock);
			closedir(dir);

			memset(info, 0, sizeof(*info));
			info->nconnections = msg.u.status.nclients_ctrlr;
			info->nsq_used = msg.u.status.nqueues;
			info->ncq_used = msg.u.status.nqueues;
			info->nsq_total = msg.u.status.nsq_total;
			info->ncq_total = msg.u.status.ncq_total;
			info->initialized = 1;

			return 0;
		}

		close(sock);
	}

	closedir(dir);

	return -ENOENT;
}

#else
int
xnvme_cplane_server_alive(uint32_t XNVME_UNUSED(cplane_id))
{
	return -ENOSYS;
}

int
xnvme_cplane_get_info(uint32_t XNVME_UNUSED(cplane_id),
		      struct xnvme_cplane_info *XNVME_UNUSED(info))
{
	return -ENOSYS;
}

int
xnvme_cplane_get_ctrlr_info(const char *XNVME_UNUSED(uri),
			    struct xnvme_cplane_ctrlr_info *XNVME_UNUSED(info))
{
	return -ENOSYS;
}
#endif
