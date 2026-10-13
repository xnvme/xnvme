# SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
#
# SPDX-License-Identifier: BSD-3-Clause

tools_source = [
  'homi.c',
  'xnvme.c',
]
tools = {
  'homi.c': [],
  'xnvme.c': [
    ['list', ['list']],
    ['enum', ['enum']],
    ['info', ['info', '1GB']],
    ['idfy, no args', ['idfy', '1GB', '--cns', '0x0']],
    ['idfy, cntid=0x0', ['idfy', '1GB', '--cns', '0x0', '--cntid', '0x0', '--setid', '0x0', '--uuid', '0x0']],
    ['idfy-ns', ['idfy-ns', '1GB']],
    ['idfy-ctrlr', ['idfy-ctrlr', '1GB']],
    ['library-info', ['library-info']],
  ],
}

foreach source, tests : tools
  bin_name = fs.stem(source)
  bin_cuse = bin_name == 'homi'
  bin = executable(
    bin_name,
    [source, xnvme_vcs_h],
    include_directories: [conf_inc, xnvme_inc] + (bin_cuse ? [xnvme_cuse_inc] : []),
    link_args: link_args_hardening,
    link_with: [xnvmelib] + (bin_cuse ? [xnvme_cuse_lib] : []),
    install_rpath: xnvmelib_rpath,
    install: true,
  )
  foreach test : tests
    test_name = test[0]
    test_args = test[1]
    test('tools - '+bin_name+', '+test_name, bin, args : test_args)
  endforeach
endforeach

subdir('xnvmeperf')
subdir('qublk')
