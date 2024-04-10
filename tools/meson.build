# SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
#
# SPDX-License-Identifier: BSD-3-Clause

tools_source = [
  'lblk.c',
  'xdd.c',
  'xnvme.c',
  'xnvme_file.c',
  'zoned.c',
  'kvs.c',
]
tools = {
  'kvs.c': [],
  'lblk.c': [
    ['enum', ['enum']],
    ['info', ['info', '1GB']],
    ['idfy', ['idfy', '1GB']],
    ['read', ['read', '1GB', '--slba', '0x0', '--nlb', '0']],
    ['write', ['write', '1GB', '--slba', '0x0', '--nlb', '0']],
    ['write-zeros', ['write-zeros', '1GB', '--slba', '0x0', '--nlb', '0']],
    ['write-dir', ['write-dir', '1GB', '--slba', '0x0', '--nlb', '7']], 
    ['write-read-pi, prchk not set', ['write-read-pi', '1GB', '--slba', '0x0', '--nlb', '8', '--pract', '0x0']], 
    ['write-read-pi, prchk=0x4', ['write-read-pi', '1GB', '--slba', '0x0', '--nlb', '0', '--pract', '1', '--prchk', '0x4']], 
    ['write-read-pi, prchk=0x5', ['write-read-pi', '1GB', '--slba', '0x0', '--nlb', '0', '--pract', '1', '--prchk', '0x5']], 
  ],
  'xdd.c': [],
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
  'xnvme_file.c': [
    ['write-read', ['write-read', '1GB', '8']],
  ],
  'zoned.c': [],
}

foreach source, tests : tools
  bin_name = fs.stem(source)
  bin = executable(
    bin_name,
    source,
    include_directories: [conf_inc, xnvme_inc],
    link_args: link_args_hardening,
    link_with: xnvmelib_static,
    install: true,
  )
  foreach test : tests
    test_name = test[0]
    test_args = test[1]
    test('tools - '+bin_name+', '+test_name, bin, args : test_args)
  endforeach
endforeach
