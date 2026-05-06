// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <string.h>
#include <errno.h>
#include <xnvme_be_cref.h>

struct xnvme_be_cref_entry {
	struct xnvme_be_cref cref;
	xnvme_be_cref_destructor_fn destructor;
	int refcount;
	char uri[XNVME_IDENT_URI_LEN + 1];
};

static struct xnvme_be_cref_entry g_cref_table[XNVME_BE_CREF_MAX_ENTRIES];

static int
_entry_matches(const struct xnvme_be_cref_entry *entry, const char *uri, const char *be_name)
{
	if (strncmp(entry->uri, uri, XNVME_IDENT_URI_LEN)) {
		return 0;
	}
	if (be_name && (!entry->cref.be_name || strcmp(entry->cref.be_name, be_name))) {
		return 0;
	}

	return 1;
}

struct xnvme_be_cref *
xnvme_be_cref_lookup(const char *uri)
{
	for (int i = 0; i < XNVME_BE_CREF_MAX_ENTRIES; ++i) {
		if (!g_cref_table[i].cref.ctrlr) {
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

		return &g_cref_table[i].cref;
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

		g_cref_table[i].cref.ctrlr = ctrlr;
		g_cref_table[i].destructor = destructor;
		g_cref_table[i].cref.be_name = be_name;
		g_cref_table[i].refcount = 1;
		strncpy(g_cref_table[i].uri, uri, XNVME_IDENT_URI_LEN);
		g_cref_table[i].uri[XNVME_IDENT_URI_LEN] = '\0';

		return 0;
	}

	XNVME_DEBUG("FAILED: out of slots");
	return -ENOMEM;
}

void *
xnvme_be_cref_ref(const char *uri, const char *be_name, void *ctrlr,
		  xnvme_be_cref_destructor_fn destructor)
{
	int free_pos = -1;

	for (int i = 0; i < XNVME_BE_CREF_MAX_ENTRIES; ++i) {
		if (!g_cref_table[i].cref.ctrlr) {
			if (free_pos < 0) {
				free_pos = i;
			}
			continue;
		}
		if (!_entry_matches(&g_cref_table[i], uri, be_name)) {
			continue;
		}
		if (g_cref_table[i].refcount < 0) {
			XNVME_DEBUG("FAILED: corrupted refcount");
			return NULL;
		}
		if (ctrlr != NULL && ctrlr != g_cref_table[i].cref.ctrlr) {
			XNVME_DEBUG("FAILED: multiple ctrlr with same ident");
			return NULL;
		}

		g_cref_table[i].refcount += 1;

		return g_cref_table[i].cref.ctrlr;
	}

	if (!ctrlr) {
		XNVME_DEBUG("FAILED: no matching ctrlr and no ctrlr provided");
		return NULL;
	}

	if (free_pos < 0) {
		XNVME_DEBUG("FAILED: out of slots");
		return NULL;
	}

	g_cref_table[free_pos].cref.ctrlr = ctrlr;
	g_cref_table[free_pos].destructor = destructor;
	g_cref_table[free_pos].cref.be_name = be_name;
	g_cref_table[free_pos].refcount = 1;
	strncpy(g_cref_table[free_pos].uri, uri, XNVME_IDENT_URI_LEN);
	g_cref_table[free_pos].uri[XNVME_IDENT_URI_LEN] = '\0';

	return g_cref_table[free_pos].cref.ctrlr;
}

int
xnvme_be_cref_deref(void *ctrlr, enum xnvme_be_cref_flags flags)
{
	if (!ctrlr) {
		XNVME_DEBUG("FAILED: !ctrlr");
		return -EINVAL;
	}

	for (int i = 0; i < XNVME_BE_CREF_MAX_ENTRIES; ++i) {
		if (g_cref_table[i].cref.ctrlr != ctrlr) {
			continue;
		}

		if (g_cref_table[i].refcount < 1) {
			XNVME_DEBUG("FAILED: invalid refcount: %d", g_cref_table[i].refcount);
			return -EINVAL;
		}

		g_cref_table[i].refcount -= 1;

		if (g_cref_table[i].refcount == 0 && (flags & XNVME_BE_CREF_DESTROY_IMMEDIATE)) {
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

int
xnvme_be_cref_cleanup(const char *be_name)
{
	int ret = 0;

	for (int i = 0; i < XNVME_BE_CREF_MAX_ENTRIES; ++i) {
		if (!g_cref_table[i].cref.ctrlr) {
			continue;
		}

		if (!be_name != !g_cref_table[i].cref.be_name ||
		    (be_name && strcmp(g_cref_table[i].cref.be_name, be_name))) {
			continue;
		}

		if (g_cref_table[i].refcount < 0) {
			XNVME_DEBUG("FAILED: invalid refcount: %d", g_cref_table[i].refcount);
			continue;
		}

		if (g_cref_table[i].refcount == 0) {
			XNVME_DEBUG("INFO: refcount: %d => detaching", g_cref_table[i].refcount);
			if (g_cref_table[i].destructor) {
				int err = g_cref_table[i].destructor(g_cref_table[i].cref.ctrlr);
				if (err) {
					XNVME_DEBUG("FAILED: destructor(): %d", err);
					ret = err;
				}
			}
			memset(&g_cref_table[i], 0, sizeof(g_cref_table[i]));
		}
	}

	return ret;
}
