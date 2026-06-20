#!/usr/bin/env bash
for branch in misc_fixes_format libconf_refactor cleanup_api_ident; do
    git cherry-pick $(git log next..$branch --oneline | awk '{print $1}')
done
