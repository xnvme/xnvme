#!/usr/bin/env bash
# Deploy the documentation in the docs.tar.gz of a 'docgen-full' job to SITE, a
# checkout of the GitHub Pages repository, as the documentation of REF, and push
# it. xnvme-docs-dest, of docs/tooling, places it under SITE/en/<name of REF>.
#
# Usage: deploy-docs.sh <docs.tar.gz> <site> <ref>
set -euo pipefail

tarball=$(realpath "$1")
site=$2
ref=$3

work=$(mktemp -d)
tar xzf "$tarball" -C "$work"
xnvme-docs-dest --docs "$work/html" --site "$site" --ref "$ref"

cd "$site"
git add .
if git diff --cached --quiet; then
	echo "The site has these docs already"
	exit 0
fi
git -c user.name='GitHUB Service' -c user.email='ghs@safl.dk' \
	commit -m "Auto-deployed update of 'docs/' for '$ref'"
# A release deploys main and its tag at once; the deploy that pushes second
# takes the other's commit and tries again
for attempt in 1 2 3 4 5; do
	if git push; then
		exit 0
	fi
	sleep $((attempt * 10))
	git -c user.name='GitHUB Service' -c user.email='ghs@safl.dk' pull --rebase
done
exit 1
