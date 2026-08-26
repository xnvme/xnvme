// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) Simon Andreas Frimann Lund <os@safl.dk>

/**
 * Versions of the layouts uPCIe shares between processes
 * ======================================================
 *
 * Each is bumped when the layout it names changes, and only then. They are
 * independent of the library version, so that a server and a client built
 * from different releases work together for as long as these agree.
 *
 * @file upcie_versions.h
 * @version 0.10.0
 */

#ifndef __UPCIE_VERSIONS_H
#define __UPCIE_VERSIONS_H

#define HOSTMEM_SHARED_DESC_VERSION 1U ///< struct hostmem_shared_desc
#define NVME_CPLANE_VERSION 1U         ///< struct nvme_cplane_msg and what it describes

#endif /* __UPCIE_VERSIONS_H */
