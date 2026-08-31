xnvmelib_nvmf_source = files(
  'xnvme_be_nvmf_admin.c',
  'xnvme_be_nvmf_async.c',
  'xnvme_be_nvmf_ctrlr.c',
  'xnvme_be_nvmf_debug.c',
  'xnvme_be_nvmf_dev.c',
  'xnvme_be_nvmf_qpair.c',
  'xnvme_be_nvmf_sync.c',
  'xnvme_be_nvmf.c',
)

if get_option('with-nvmf').enabled()
  subdir('rdma')
  conf_inc += [ include_directories('.') ]
  xnvmelib_source += xnvmelib_nvmf_source
endif
