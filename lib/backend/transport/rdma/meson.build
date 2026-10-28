xnvmelib_rdma_source = files(
  'xnvme_be_rdma.c',
  'xnvme_be_rdma_cm.c',
  'xnvme_be_rdma_ctrlr.c',
  'xnvme_be_rdma_mem.c',
  'xnvme_be_rdma_qpair.c',
  'xnvme_be_rdma_verbs.c',
)

if conf_data.get('XNVME_BE_RDMA_ENABLED')
  xnvmelib_inc += [ include_directories('.'), xnvmelib_transport_common_inc ]
  xnvmelib_source += xnvmelib_rdma_source + xnvmelib_transport_common_source
endif
