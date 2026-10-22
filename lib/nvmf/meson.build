xnvmelib_nvmf_source = files(
  'xnvme_be_nvmf_admin.c',
  'xnvme_be_nvmf_async.c',
  'xnvme_be_nvmf_ctrlr.c',
  'xnvme_be_nvmf_debug.c',
  'xnvme_be_nvmf_dev.c',
  'xnvme_be_nvmf_discovery.c',
  'xnvme_be_nvmf_fabric.c',
  'xnvme_be_nvmf_mem.c',
  'xnvme_be_nvmf_ns.c',
  'xnvme_be_nvmf_qpair.c',
  'xnvme_be_nvmf_subsys.c',
  'xnvme_be_nvmf_sync.c',
  'rdma/xnvme_be_nvmf_rdma.c',
  'rdma/xnvme_be_nvmf_rdma_cm.c',
  'rdma/xnvme_be_nvmf_rdma_verbs.c',
  'util/xnvme_be_nvmf_rbtree.c',
)

if get_option('with-nvmf').enabled()
  xnvmelib_deps += [ibverbs_dep, rdmacm_dep]
  conf_inc += [ include_directories('.', 'rdma', 'util') ]
  xnvmelib_source += xnvmelib_nvmf_source
endif
