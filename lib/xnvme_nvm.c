// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <errno.h>
#include <libxnvme.h>
#include <xnvme_be.h>
#include <xnvme_dev.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#ifndef MIN
#define MIN(a, b)	((a) < (b) ? (a) : (b))
#endif


int
xnvme_adm_format(struct xnvme_cmd_ctx *ctx, uint32_t nsid, uint8_t lbafl, uint8_t lbafu,
		 uint8_t mset, uint8_t ses, uint8_t pi, uint8_t pil)
{
	ctx->cmd.common.opcode = XNVME_SPEC_NVM_OPC_FMT;
	ctx->cmd.common.nsid = nsid;
	ctx->cmd.format.lbafl = lbafl;
	ctx->cmd.format.lbafu = lbafu;
	ctx->cmd.format.mset = mset;
	ctx->cmd.format.pi = pi;
	ctx->cmd.format.pil = pil;
	ctx->cmd.format.ses = ses;

	return xnvme_cmd_pass_admin(ctx, NULL, 0, NULL, 0);
}

int
xnvme_nvm_sanitize(struct xnvme_cmd_ctx *ctx, uint8_t sanact, bool ause, uint32_t ovrpat,
		   uint8_t owpass, bool oipbp, bool nodas)
{
	ctx->cmd.common.opcode = XNVME_SPEC_NVM_OPC_SANITIZE;
	ctx->cmd.sanitize.sanact = sanact;
	ctx->cmd.sanitize.ause = ause;
	ctx->cmd.sanitize.owpass = owpass;
	ctx->cmd.sanitize.oipbp = oipbp;
	ctx->cmd.sanitize.nodas = nodas;
	ctx->cmd.sanitize.ause = ause;
	ctx->cmd.sanitize.ovrpat = ovrpat;

	return xnvme_cmd_pass_admin(ctx, NULL, 0, NULL, 0);
}


long getFileSize(int fd) {
    struct stat statBuf;
    if (fstat(fd, &statBuf) != 0) {
        XNVME_DEBUG("FAILED: FStat call error ");
        return -1;
    }
    return statBuf.st_size;
}

int
xnvme_nvm_firmware_download(struct xnvme_cmd_ctx *ctx, char *filename) {
	uint32_t xfer = 16384;
	uint32_t offset = 0;
    struct xnvme_dev *dev = ctx->dev;

	ctx->cmd.common.opcode = XNVME_SPEC_NVM_OPC_FIRMWARE_IMAGE_DOWNLOAD;

    int fd = open(filename, O_RDONLY);
    if (fd == -1) {
        XNVME_DEBUG("FAILED: Open File Error");
        return -1;
    }

    long fileSize = getFileSize(fd);
    if (fileSize < 0) {
        XNVME_DEBUG("FAILED: Stat call error ");
        return -1;
    }

	if (fileSize & 0x3) {
		XNVME_DEBUG("Invalid Size for Firmware Image");
		return -1;
	}

    int roundedSize = (fileSize + xfer -1)/xfer *xfer;
	//int totalSize = roundedSize;
	int errCode = 0;
    void *fw_buf = xnvme_buf_alloc(dev, xfer);
    //ctx->cmd.firmware_image_download.offst = 0;

    while (roundedSize > 0) {
		memset(fw_buf, 0, xfer);
        int bytes_read = read(fd, fw_buf, xfer);
		xfer = MIN(xfer, roundedSize);
		ctx->cmd.common.ndt = (xfer >> 2) - 1;
		ctx->cmd.common.ndm = offset >> 2;
		//memset(ctx->cmd.firmware_image_download.dptr, 0, 16); // 128 Bit = 16 bytes
		//memcpy(ctx->cmd.firmware_image_download.dptr, fw_buf, 4);        // Data Pointer
		XNVME_DEBUG("NVMe 128-bit Address during Firmware Download is : ");
        //for (int i = 15; i >= 0; i--) {
        //    XNVME_DEBUG("%02X", ctx->cmd.firmware_image_download.dptr[i]);
        //}
	    //ctx->cmd.firmware_image_download.numbd = bytes_read ;  // Number of Dwords
        errCode = xnvme_cmd_pass_admin(ctx, fw_buf, 16384, NULL, 0);

        XNVME_DEBUG("Firmware Download Command errCode : %d", errCode);
		roundedSize -= xfer;
		offset += xfer;
		///totalSize -= bytes_read;                              // Decrement the totalSize by bytes_read
		//ctx->cmd.firmware_image_download.offst += bytes_read; //Increment the Offset by bytes read
        XNVME_DEBUG("Total Size : %d, Offset = %d", roundedSize, offset);
		xnvme_cmd_ctx_cpl_status(ctx);
		xnvme_cmd_ctx_pr(ctx, XNVME_PR_DEF);
		//sleep(1); // Sleep for 2 seconds between each download command
	}
    xnvme_buf_free(dev, fw_buf);      // Free the buffer allocated
	close(fd);
	return errCode;
}

int
xnvme_nvm_firmware_commit(struct xnvme_cmd_ctx *ctx)
{
	ctx->cmd.common.opcode = XNVME_SPEC_NVM_OPC_FIRMWARE_COMMIT;
	 XNVME_DEBUG("Firmware Commit Command");
	//ctx->cmd.firmware_image_commit.fs = 0x0;        // Firmware Slot is 3 bits, set to 0
	//ctx->cmd.firmware_image_commit.ca = 0x1;        // Commit Action is 3 bits, set to 1 :
	                                                // Downloaded image replaces the existing image, if any, in the specified
                                                    // Firmware Slot. The newly placed image is activated at the next Controller Level Reset
	//ctx->cmd.firmware_image_commit.rsvd = 0;        // Reserved, 25 bits set to 0
	//ctx->cmd.firmware_image_commit.bpid = 1;        // Boot Partition, 1 bit. Set means boot partition will be used for commit action.
	
	uint32_t bpid = 0, action = 0x001, slot = 0;  // NVMe command parameters
    ctx->cmd.common.ndt = (bpid << 31) | (action << 3) | slot;

	ctx->cmd.common.rsvd = 0;
	return xnvme_cmd_pass_admin(ctx, NULL, 0, NULL, 0);
}

int
xnvme_nvm_read(struct xnvme_cmd_ctx *ctx, uint32_t nsid, uint64_t slba, uint16_t nlb, void *dbuf,
	       void *mbuf)
{
	size_t dbuf_nbytes = dbuf ? ctx->dev->geo.lba_nbytes * (nlb + 1) : 0;
	size_t mbuf_nbytes = mbuf ? ctx->dev->geo.nbytes_oob * (nlb + 1) : 0;

	// TODO: consider returning -EINVAL when mbuf is provided and namespace
	// have extended-lba in effect

	ctx->cmd.common.opcode = XNVME_SPEC_NVM_OPC_READ;
	ctx->cmd.common.nsid = nsid;
	ctx->cmd.nvm.slba = slba;
	ctx->cmd.nvm.nlb = nlb;

	return xnvme_cmd_pass(ctx, dbuf, dbuf_nbytes, mbuf, mbuf_nbytes);
}

int
xnvme_nvm_write(struct xnvme_cmd_ctx *ctx, uint32_t nsid, uint64_t slba, uint16_t nlb,
		const void *dbuf, const void *mbuf)
{
	void *cdbuf = (void *)dbuf;
	void *cmbuf = (void *)mbuf;

	size_t dbuf_nbytes = cdbuf ? ctx->dev->geo.lba_nbytes * (nlb + 1) : 0;
	size_t mbuf_nbytes = cmbuf ? ctx->dev->geo.nbytes_oob * (nlb + 1) : 0;

	// TODO: consider returning -EINVAL when mbuf is provided and namespace
	// have extended-lba in effect

	ctx->cmd.common.opcode = XNVME_SPEC_NVM_OPC_WRITE;
	ctx->cmd.common.nsid = nsid;
	ctx->cmd.nvm.slba = slba;
	ctx->cmd.nvm.nlb = nlb;

	return xnvme_cmd_pass(ctx, cdbuf, dbuf_nbytes, cmbuf, mbuf_nbytes);
}

int
xnvme_nvm_write_uncorrectable(struct xnvme_cmd_ctx *ctx, uint32_t nsid, uint64_t slba,
			      uint16_t nlb)
{
	ctx->cmd.common.opcode = XNVME_SPEC_NVM_OPC_WRITE_UNCORRECTABLE;
	ctx->cmd.common.nsid = nsid;
	ctx->cmd.nvm.slba = slba;
	ctx->cmd.nvm.nlb = nlb;

	return xnvme_cmd_pass(ctx, NULL, 0, NULL, 0);
}

int
xnvme_nvm_write_zeroes(struct xnvme_cmd_ctx *ctx, uint32_t nsid, uint64_t slba, uint16_t nlb)
{
	ctx->cmd.common.opcode = XNVME_SPEC_NVM_OPC_WRITE_ZEROES;
	ctx->cmd.common.nsid = nsid;
	ctx->cmd.write_zeroes.slba = slba;
	ctx->cmd.write_zeroes.nlb = nlb;

	return xnvme_cmd_pass(ctx, NULL, 0, NULL, 0);
}

void
xnvme_prep_nvm(struct xnvme_cmd_ctx *ctx, uint8_t opcode, uint32_t nsid, uint64_t slba,
	       uint16_t nlb)
{
	ctx->cmd.common.opcode = opcode;
	ctx->cmd.common.nsid = nsid;
	ctx->cmd.nvm.slba = slba;
	ctx->cmd.nvm.nlb = nlb;
}

int
xnvme_nvm_dsm(struct xnvme_cmd_ctx *ctx, uint32_t nsid, struct xnvme_spec_dsm_range *dsm_range,
	      uint8_t nr, bool ad, bool idw, bool idr)
{
	ctx->cmd.common.opcode = XNVME_SPEC_NVM_OPC_DATASET_MANAGEMENT;
	ctx->cmd.common.nsid = nsid;
	ctx->cmd.dsm.nr = nr;
	ctx->cmd.dsm.ad = ad;
	ctx->cmd.dsm.idw = idw;
	ctx->cmd.dsm.idr = idr;

	return xnvme_cmd_pass(ctx, (void *)dsm_range, sizeof(*dsm_range) * (nr + 1), NULL, 0);
}

int
xnvme_nvm_scopy(struct xnvme_cmd_ctx *ctx, uint32_t nsid, uint64_t sdlba,
		struct xnvme_spec_nvm_scopy_fmt_zero *ranges, uint8_t nr,
		enum xnvme_nvm_scopy_fmt copy_fmt)
{
	size_t ranges_nbytes = 0;

	if (copy_fmt & XNVME_NVM_SCOPY_FMT_ZERO) {
		ranges_nbytes = (nr + 1) * sizeof(*ranges);
	}
	if (copy_fmt & XNVME_NVM_SCOPY_FMT_SRCLEN) {
		ranges_nbytes = (nr + 1) * sizeof(struct xnvme_spec_nvm_cmd_scopy_fmt_srclen);
	}

	ctx->cmd.common.opcode = XNVME_SPEC_NVM_OPC_SCOPY;
	ctx->cmd.common.nsid = nsid;
	ctx->cmd.scopy.sdlba = sdlba;
	ctx->cmd.scopy.nr = nr;

	// TODO: consider the remaining command-fields

	return xnvme_cmd_pass(ctx, ranges, ranges_nbytes, NULL, 0);
}

int
xnvme_nvm_mgmt_recv(struct xnvme_cmd_ctx *ctx, uint32_t nsid, uint8_t mo, uint16_t mos, void *dbuf,
		    uint32_t dbuf_nbytes)
{
	ctx->cmd.common.opcode = XNVME_SPEC_NVM_OPC_IO_MGMT_RECV;
	ctx->cmd.common.nsid = nsid;
	ctx->cmd.mgmt.mgmt_recv.mo = mo;
	ctx->cmd.mgmt.mgmt_recv.mos = mos;
	ctx->cmd.mgmt.mgmt_recv.numd = dbuf_nbytes / sizeof(uint32_t) - 1u;

	return xnvme_cmd_pass(ctx, dbuf, dbuf_nbytes, NULL, 0x0);
}

int
xnvme_nvm_mgmt_send(struct xnvme_cmd_ctx *ctx, uint32_t nsid, uint8_t mo, uint16_t mos, void *dbuf,
		    uint32_t dbuf_nbytes)
{
	ctx->cmd.common.opcode = XNVME_SPEC_NVM_OPC_IO_MGMT_SEND;
	ctx->cmd.common.nsid = nsid;
	ctx->cmd.mgmt.mgmt_send.mo = mo;
	ctx->cmd.mgmt.mgmt_send.mos = mos;

	return xnvme_cmd_pass(ctx, dbuf, dbuf_nbytes, NULL, 0x0);
}

int
xnvme_nvm_compare(struct xnvme_cmd_ctx *ctx, uint32_t nsid, uint64_t slba, uint16_t nlb,
		  void *dbuf, void *mbuf)
{
	size_t dbuf_nbytes = dbuf ? ctx->dev->geo.lba_nbytes * (nlb + 1) : 0;
	size_t mbuf_nbytes = mbuf ? ctx->dev->geo.nbytes_oob * (nlb + 1) : 0;

	ctx->cmd.common.opcode = XNVME_SPEC_NVM_OPC_COMPARE;
	ctx->cmd.common.nsid = nsid;
	ctx->cmd.compare.slba = slba;
	ctx->cmd.compare.nlb = nlb;

	return xnvme_cmd_pass(ctx, dbuf, dbuf_nbytes, mbuf, mbuf_nbytes);
}