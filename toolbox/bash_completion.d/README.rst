.. SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
..
.. SPDX-License-Identifier: BSD-3-Clause

Bash Completions
================

One script, ``xnvme``, completes the ``xnvme`` front-end and every program it
runs. It holds no lists of its own: it asks ``xnvme --list-cmds`` for the
commands and installed programs, ``xnvme --list-cmds=<group>`` for those of a
group such as ``test``, a program for its subcommands with ``--list-cmds``, and
a command for its options with ``--help``. So it completes what is installed,
and needs no regenerating when a command changes.
