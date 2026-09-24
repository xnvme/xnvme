xnvmelib_nvmf_rdma_source = files(
  'xnvme_be_nvmf_rdma.c',
  'xnvme_be_nvmf_rdma_cm.c',
  'xnvme_be_nvmf_rdma_ctrlr.c',
  'xnvme_be_nvmf_rdma_qpair.c',
)

if get_option('with-nvmf').enabled()
  xnvmelib_deps += [ibverbs_dep, rdmacm_dep]
  conf_inc += [ include_directories('.') ]
  xnvmelib_source += xnvmelib_nvmf_rdma_source
endif
