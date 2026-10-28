xnvmelib_rdma_source = files(
  'xnvme_be_rdma.c',
  'xnvme_be_rdma_admin.c',
  'xnvme_be_rdma_async.c',
  'xnvme_be_rdma_cm.c',
  'xnvme_be_rdma_dev.c',
  'xnvme_be_rdma_mem.c',
  'xnvme_be_rdma_sync.c',
  'xnvme_be_rdma_transport.c',
  'xnvme_be_rdma_verbs.c',
)

if get_option('with-rdma').enabled()
  xnvmelib_deps += [ibverbs_dep, rdmacm_dep]
  conf_inc += [ include_directories('.'), xnvmelib_transport_common_inc ]
  xnvmelib_source += xnvmelib_rdma_source + xnvmelib_transport_common_source
endif
