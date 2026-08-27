# SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
#
# SPDX-License-Identifier: BSD-3-Clause

qublk_source = [
  'main.c',
  'ctrl.c',
  'io.c',
]

if not is_linux
  warning('No support for ublk on this platform, skipping qublk')
elif not uring_dep.found()
  warning('No liburing, skipping qublk')
elif not cc.has_header('linux/ublk_cmd.h')
  warning('No linux/ublk_cmd.h, skipping qublk')
elif not cc.has_header_symbol('linux/ublk_cmd.h', 'UBLK_U_CMD_GET_FEATURES')
  warning('linux/ublk_cmd.h lacks the ioctl-encoded ublk commands (kernel headers < 6.3), skipping qublk')
else
  executable(
    'qublk',
    qublk_source,
    include_directories: [conf_inc, xnvme_inc],
    link_args: link_args_hardening,
    link_with: xnvmelib,
    install_rpath: xnvmelib_rpath,
    dependencies: [uring_dep, thread_dep],
    install: true,
  )
endif
