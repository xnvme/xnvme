xnvmelib_nvmf_source = files(
  'xnvme_be_nvmf_mem.c',
  'util/xnvme_be_nvmf_rbtree.c',
)

if get_option('with-nvmf').enabled()
  xnvmelib_deps += [ibverbs_dep, rdmacm_dep]
  conf_inc += [ include_directories('.', 'util') ]
  xnvmelib_source += xnvmelib_nvmf_source
endif
