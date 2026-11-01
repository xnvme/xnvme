# SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
#
# SPDX-License-Identifier: BSD-3-Clause

meson.add_dist_script('meson_dist_archive_fixer.py')

if is_linux and get_option('tools')
  subdir('systemd')
endif
