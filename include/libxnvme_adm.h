#ifndef __LIBXNVME_ADM_H
#define __LIBXNVME_ADM_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Submit and wait for completion of an NVMe Identify command
 *
 * @param dev Device handle obtained with xnvme_dev_open() / xnvme_dev_openf()
 * @param cns the ::xnvme_spec_idfy_cns to retrieve
 * @param cntid Controller Identifier
 * @param nsid Namespace Identifier
 * @param nvmsetid NVM set Identifier
 * @param uuid UUID index
 * @param dbuf ponter to data-payload
 * @param req Pointer to structure for NVMe completion
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned.
 */
int
xnvme_adm_idfy(struct xnvme_dev *dev, uint8_t cns, uint16_t cntid, uint8_t nsid, uint16_t nvmsetid,
	       uint8_t uuid, struct xnvme_spec_idfy *dbuf, struct xnvme_req *req);

/**
 * Submit and wait for completion of an NVMe Identify command for the
 * controller associated with the given device
 *
 * @param dev Device handle obtained with xnvme_dev_open() / xnvme_dev_openf()
 * @param dbuf pointer to data-payload
 * @param req Pointer to structure for NVMe completion
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned.
 */
int
xnvme_adm_idfy_ctrlr(struct xnvme_dev *dev, struct xnvme_spec_idfy *dbuf, struct xnvme_req *req);

/**
 * Submit and wait for completion of an NVMe Identify command for the
 * controller associated with the given device
 *
 * @param dev Device handle obtained with xnvme_dev_open() / xnvme_dev_openf()
 * @param csi Command Set Identifier
 * @param dbuf Buffer allocated by xnvme_buf_alloc()
 * @param req Pointer to structure for NVMe completion
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned.
 */
int
xnvme_adm_idfy_ctrlr_csi(struct xnvme_dev *dev, uint8_t csi, struct xnvme_spec_idfy *dbuf,
			 struct xnvme_req *req);

/**
 * Submit and wait for completion of an NVMe Identify command for the given nsid
 *
 * Execute an NVMe identify namespace command for the namespace associated with
 * the given 'dev' when 'nsid'=0, execute for the given 'nsid' when it is > 0,
 * when the given 'nsid' == 0, then the command is executed for the namespace
 * associated with the given device
 *
 * @todo document
 *
 * @param dev Device handle obtained with xnvme_dev_open() / xnvme_dev_openf()
 * @param nsid Namespace identifier
 * @param dbuf Buffer allocated by xnvme_buf_alloc()
 * @param req Pointer to structure for NVMe completion
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned.
 */
int
xnvme_adm_idfy_ns(struct xnvme_dev *dev, uint32_t nsid, struct xnvme_spec_idfy *dbuf,
		  struct xnvme_req *req);

/**
 * Submit and wait for completion of an NVMe Identify command for the given nsid
 *
 * Execute an I/O Command Set specific NVMe identify namespace command for the
 * given 'nsid'. When the given 'nsid' == 0, then the command is executed for
 * the namespace associated with the given device
 *
 * @param dev Device handle obtained with xnvme_dev_open() / xnvme_dev_openf()
 * @param nsid Namespace identifier
 * @param csi Command Set Identifier
 * @param dbuf Buffer allocated by xnvme_buf_alloc()
 * @param req Pointer to structure for NVMe completion
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned.
 */
int
xnvme_adm_idfy_ns_csi(struct xnvme_dev *dev, uint32_t nsid, uint8_t csi,
		      struct xnvme_spec_idfy *dbuf, struct xnvme_req *req);

/**
 * Submit and wait for completion of an NVMe Get Log Page command
 *
 * @param dev Device handle obtained with xnvme_dev_open() / xnvme_dev_openf()
 * @param lid Log Page Identifier for the log to retrieve entries for
 * @param lsp Log Specific Field for the log to retrieve entries for
 * @param lpo_nbytes Log page Offset in BYTES
 * @param nsid Namespace Identifier
 * @param rae Retain Asynchronous Event, 0=Clear, 1=Retain
 * @param dbuf Buffer allocated with `xnvme_buf_alloc`
 * @param dbuf_nbytes Number of BYTES to write from log-page to buf
 * @param req Pointer to structure for NVMe completion
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned.
 */
int
xnvme_adm_log(struct xnvme_dev *dev, uint8_t lid, uint8_t lsp, uint64_t lpo_nbytes, uint32_t nsid,
	      uint8_t rae, void *dbuf, uint32_t dbuf_nbytes, struct xnvme_req *req);

/**
 * Submit and wait for completion of an NVMe Get Features (gfeat) command
 *
 * @param dev Device handle obtained with xnvme_dev_open() / xnvme_dev_openf()
 * @param nsid Namespace identifier
 * @param fid Feature identifier
 * @param sel Select which value of the feature to select, that is, one of
 * current/default/saved/supported
 * @param dbuf pointer to data-payload, use if required for the given `fid`
 * @param dbuf_nbytes size of data-payload in bytes
 * @param req Pointer to structure for NVMe completion
 *
 * @return
 */
int
xnvme_adm_gfeat(struct xnvme_dev *dev, uint32_t nsid, uint8_t fid, uint8_t sel, void *dbuf,
		size_t dbuf_nbytes, struct xnvme_req *req);

/**
 * Submit and wait for completion of an NVMe Set Feature (sfeat) command
 *
 * @param dev Device handle obtained with xnvme_dev_open() / xnvme_dev_openf()
 * @param nsid Namespace identifier
 * @param fid Feature identifier (see NVMe 1.3; Figure 84)
 * @param feat Structure defining feature attributes
 * @param save
 * @param dbuf pointer to data-payload
 * @param dbuf_nbytes size of the data-payload in bytes
 * @param req Pointer to structure for NVMe completion
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned.
 */
int
xnvme_adm_sfeat(struct xnvme_dev *dev, uint32_t nsid, uint8_t fid, uint32_t feat, uint8_t save,
		const void *dbuf, size_t dbuf_nbytes, struct xnvme_req *req);

/**
 * Submit and wait for completion of an NVMe Format NVM command
 *
 * TODO: consider timeout, reset, and need for library/dev re-initialization
 *
 * @param dev Device handle obtained with xnvme_dev_open() / xnvme_dev_openf()
 * @param nsid Namespace identifier
 * @param lbaf The LBA format to format the Namespace with
 * @param zf Zone Format
 * @param mset Metadata Settings
 * @param ses Secure erase settings; 0x0: No Secure Erase, 0x1: User Data erase,
 * 0x2: Cryptographic Erase
 * @param pil Protection information location; 0x0: last eight bytes of
 * metadata, 0x1: first eight bytes of metadata
 * @param pi Protection information; 0x0: Disabled, 0x1: Enabled(Type1), 0x2:
 * Enabled(Type2), 0x3: Enabled(Type3)
 * @param req Pointer to structure for NVMe completion
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned.
 */
int
xnvme_adm_format(struct xnvme_dev *dev, uint32_t nsid, uint8_t lbaf, uint8_t zf, uint8_t mset,
                 uint8_t ses, uint8_t pi, uint8_t pil, struct xnvme_req *req);

/**
 * Submit and wait for completion of an Sanitize command
 *
 * @todo consider timeout, reset, and need for library/dev re-initialization
 *
 * @note Success of this function only means that the sanitize *command*
 * succeeded and that the sanitize *operation* has started.
 *
 * @param dev Device handle obtained with xnvme_dev_open() / xnvme_dev_openf()
 * @param sanact Sanitize action; 0x1: Exit failure mode, 0x2: Block Erase, 0x3
 * Overwrite, 0x4 Crypto Erase
 * @param ause: Allow Unrestricted Sanitize Exit; 0x0: Restricted Mode, 0x1:
 * Unrestricted Mode
 * @param ovrpat Overwrite Pattern; 32-bit pattern used by the Overwrite action
 * @param owpass Overwrite pass Count, how many times the media is to be
 * overwritten; 0x0: 15 overwrite passes
 * @param oipbp Overwrite invert pattern between passes; 0x0: Disabled, 0x1:
 * Enabled
 * @param nodas No Deallocate After Sanitize; 0x0: Attempt to deallocate; 0x1 Do
 * not attempt to deallocate
 * @param req Pointer to structure for NVMe completion
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned.
 */
int
xnvme_nvm_sanitize(struct xnvme_dev *dev, uint8_t sanact, uint8_t ause, uint32_t ovrpat,
                   uint8_t owpass, uint8_t oipbp, uint8_t nodas, struct xnvme_req *req);

#ifdef __cplusplus
}
#endif

#endif /* __LIBXNVME_ADM_H */
