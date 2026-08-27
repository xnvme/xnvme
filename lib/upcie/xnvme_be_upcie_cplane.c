// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * Reporting on a runtime without joining it
 *
 * Everything here asks whoever is serving an identifier and takes the answer,
 * rather than reading a shared segment and inferring one. Connecting is most of
 * the answer: a process that answers is a process holding the controllers.
 *
 * A previous arrangement kept this in shared memory, where a killed server
 * left a segment that still read as plausible and had to be told apart from a
 * live one. Nothing here has that problem: a socket that answers has somebody
 * behind it.
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
	struct nvme_cplane_msg msg = {0};
	int err = xnvme_be_upcie_cplane_query(cplane_id, &msg);

	if (!err) {
		return 1;
	}

	return (err == -ENOENT) ? 0 : err;
}

/**
 * Ask an open connection about the controller at `index`
 *
 * @return 0 on success, -ERANGE past the last one, negative errno on error
 */
static int
cplane_status_at(int sock, uint32_t index, struct nvme_cplane_msg *msg)
{
	memset(msg, 0, sizeof(*msg));
	msg->index = index;

	return xnvme_be_upcie_cplane_ask_status(sock, msg);
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

	/* One connection for the whole walk: any of them answers for every
	 * controller the server holds, so a connection apiece would be a
	 * thread apiece on the server for nothing. */
	sock = xnvme_be_upcie_cplane_connect(path);
	if (sock < 0) {
		return sock;
	}

	err = cplane_status_at(sock, 0, &msg);
	if (err) {
		close(sock);

		return err;
	}

	memset(info, 0, sizeof(*info));

	/* The count is of connections; the one asking is one more. */
	info->nconnections = msg.u.status.nclients + 1;

	/* The server says how many it holds and describes them one at a time,
	 * so the set comes from the server rather than from what happens to be
	 * lying in /tmp. */
	ndevs = msg.u.status.ndevs;
	info->nctrlrs_held = ndevs;

	for (uint32_t i = 0; i < ndevs; ++i) {
		struct nvme_cplane_msg per = {0};

		if (info->nctrlrs == XNVME_CPLANE_MAX_CTRLRS) {
			break; ///< nctrlrs_held still says how many there were
		}
		if (i && cplane_status_at(sock, i, &per)) {
			continue;
		}

		snprintf(info->ctrlrs[info->nctrlrs], sizeof(info->ctrlrs[0]), "%s",
			 i ? per.u.status.bdf : msg.u.status.bdf);
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

	/* The caller names a controller, not a runtime. Nothing registers
	 * runtimes, so the candidates are the runtime-wide sockets in /tmp;
	 * what each one holds then comes from the runtime itself. */
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
			continue; ///< Gone between the readdir and the connect
		}

		for (uint32_t i = 0; !cplane_status_at(sock, i, &msg); ++i) {
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
