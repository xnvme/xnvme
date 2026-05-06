// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <string.h>
#include <errno.h>
#include <xnvme_be_cref.h>

static struct xnvme_be_cref_entry g_cref_table[XNVME_BE_CREF_MAX_ENTRIES];

const struct xnvme_be_cref_entry *
xnvme_be_cref_lookup(const char *uri)
{
	for (int i = 0; i < XNVME_BE_CREF_MAX_ENTRIES; ++i) {
		if (!g_cref_table[i].ctrlr) {
			continue;
		}
		if (strncmp(g_cref_table[i].uri, uri, XNVME_IDENT_URI_LEN)) {
			continue;
		}
		if (g_cref_table[i].refcount < 1) {
			XNVME_DEBUG("FAILED: corrupted refcount");
			return NULL;
		}

		g_cref_table[i].refcount += 1;

		return &g_cref_table[i];
	}

	return NULL;
}

int
xnvme_be_cref_insert(const char *uri, const char *be_name, void *ctrlr,
		     xnvme_be_cref_destructor_fn destructor)
{
	if (!ctrlr) {
		XNVME_DEBUG("FAILED: !ctrlr");
		return -EINVAL;
	}

	for (int i = 0; i < XNVME_BE_CREF_MAX_ENTRIES; ++i) {
		if (g_cref_table[i].refcount) {
			continue;
		}

		g_cref_table[i].ctrlr = ctrlr;
		g_cref_table[i].destructor = destructor;
		g_cref_table[i].be_name = be_name;
		g_cref_table[i].refcount = 1;
		strncpy(g_cref_table[i].uri, uri, XNVME_IDENT_URI_LEN);
		g_cref_table[i].uri[XNVME_IDENT_URI_LEN] = '\0';

		return 0;
	}

	XNVME_DEBUG("FAILED: out of slots");
	return -ENOMEM;
}

int
xnvme_be_cref_deref(void *ctrlr)
{
	if (!ctrlr) {
		XNVME_DEBUG("FAILED: !ctrlr");
		return -EINVAL;
	}

	for (int i = 0; i < XNVME_BE_CREF_MAX_ENTRIES; ++i) {
		if (g_cref_table[i].ctrlr != ctrlr) {
			continue;
		}

		if (g_cref_table[i].refcount < 1) {
			XNVME_DEBUG("FAILED: invalid refcount: %d", g_cref_table[i].refcount);
			return -EINVAL;
		}

		g_cref_table[i].refcount -= 1;

		if (g_cref_table[i].refcount == 0) {
			int err = 0;

			XNVME_DEBUG("INFO: refcount: %d => detaching", g_cref_table[i].refcount);
			if (g_cref_table[i].destructor) {
				err = g_cref_table[i].destructor(ctrlr);
				if (err) {
					XNVME_DEBUG("FAILED: destructor(): %d", err);
				}
			}
			memset(&g_cref_table[i], 0, sizeof(g_cref_table[i]));
			return err;
		}

		return 0;
	}

	XNVME_DEBUG("FAILED: no tracking for %p", ctrlr);
	return -EINVAL;
}
