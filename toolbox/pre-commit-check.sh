#!/usr/bin/env bash
hash pre-commit 2>/dev/null || {
	cat <<\EOF
==============================================================================
  Info: xNVMe uses the 'pre-commit' framework https://pre-commit.com/
==============================================================================

  xNVMe has a 'pre-commit' configuration, see: .pre-commit-config.yaml

  It takes care of checking:

  * git-commit messages
  * Code formating
  * Code linter issues

  You can enable it by running, e.g.:

    pip3 install --user pre-commit
    pre-commit install
    pre-commit install --hook-type commit-msg

  Or, in whichever way best serves you on your platform.

  When enabled, it will trigger when you commit/rebase, and will inspect the
  files affected by your staged changes. You can run it manually, using:

    # On staged changes
    pre-commit run

    # On all files
    pre-commit run --all-files

  There are Makefile helpers to do the same:

    make format
    make format-all

==============================================================================
EOF
	exit 0
}
