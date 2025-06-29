// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Simon Andreas Frimann Lund <os@safl.dk>

/**
 * Bitmap-based tracking of NVMe I/O Queue Pair `qid` allocation
 * =============================================================
 *
 * This bitmap tracks the allocation status of all NVMe Queue Identifiers (QIDs), where each bit
 * represents a QID. According to the NVMe specification, the maximum valid amount of QIDs are
 * 0xFFFF (65,535), including QID 0, which is reserved for the Admin Queue.
 *
 * By default, NVME_QID_MAX is set to 0xFFFF, allowing for tracking of the full
 * specification-defined range of QIDs. The helper function nvme_qid_find_free() will never return
 * QID 0 to avoid allocating the reserved Admin Queue ID, so the practical maximum number of
 * allocatable I/O QPairs is 65,534.
 *
 * If your system only uses a smaller number of queues (e.g., 1024 or 2048), you can reduce
 * NVME_QID_MAX to conserve memory. For example, with NVME_QID_MAX set to 4096, the bitmap will
 * consume 512 bytes.
 *
 * @file nvme_qid.h
 * @version 0.3.2
 */

#define BITS_PER_WORD 64

#define NVME_QID_MAX 0xFFFF
#define NVME_QID_BITMAP_WORDS (NVME_QID_MAX / BITS_PER_WORD)

static inline int
nvme_qid_is_allocated(uint64_t *qid_bitmap, uint16_t qid)
{
	if (qid >= NVME_QID_MAX) {
		return -EINVAL;
	}

	return (qid_bitmap[qid / BITS_PER_WORD] >> (qid % BITS_PER_WORD)) & 1;
}

static inline int
nvme_qid_free(uint64_t *qid_bitmap, uint16_t qid)
{
	if (qid >= NVME_QID_MAX) {
		return -EINVAL;
	}

	qid_bitmap[qid / BITS_PER_WORD] &= ~(1ULL << (qid % BITS_PER_WORD));

	return 0;
}

static inline int
nvme_qid_alloc(uint64_t *qid_bitmap, uint16_t qid)
{
	if (qid >= NVME_QID_MAX) {
		return -EINVAL;
	}

	qid_bitmap[qid / BITS_PER_WORD] |= (1ULL << (qid % BITS_PER_WORD));

	return 0;
}

static inline int
nvme_qid_bitmap_init(uint64_t *qid_bitmap)
{
	memset(qid_bitmap, 0, sizeof(*qid_bitmap) * NVME_QID_BITMAP_WORDS);

	return nvme_qid_alloc(qid_bitmap, 0); ///< Reserve qid 0 since it is the admin-queue
}

static inline int
nvme_qid_find_free(uint64_t *qid_bitmap)
{
	for (int word = 0; word < NVME_QID_BITMAP_WORDS; ++word) {
		if (qid_bitmap[word] != UINT64_MAX) {
			for (int bit = 0; bit < BITS_PER_WORD; ++bit) {
				if (!(qid_bitmap[word] & (1ULL << bit))) {
					return word * BITS_PER_WORD + bit;
				}
			}
		}
	}

	return -ENOMEM;
}
