xnvmelib_rdma_source = files(
  'xnvme_be_rdma.c',
)

if conf_data.get('XNVME_BE_RDMA_ENABLED')
  xnvmelib_deps += [ibverbs_dep, rdmacm_dep]
  conf_inc += [ include_directories('.'), xnvmelib_transport_common_inc ]
  xnvmelib_source += xnvmelib_rdma_source + xnvmelib_transport_common_source
endif
