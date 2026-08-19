(sec-contributing-release)=

# Release Checklist

With every release of **xNVMe**, the following tasks must be ticked off.
Initially, the window of features is closed, that is, all PRs intended for the
release are integrated on `main` and all tests are passing. Then work through
the steps below in order.

Two things make this checkable rather than merely readable, which matters as
much for an agent following it as for a person:

`make release-check`
: Settles the mechanical parts with an exit code rather than a claim: every
  file embedding the version agrees, the tag is not already taken, the
  CHANGELOG has a section for the version, CONTRIBUTORS names everyone who
  has committed since the previous tag, and the tree is clean. Run it after
  each step below; it is the answer to "did I miss something".

`make version`, `make contributors`, `make gen-man-pages`, `make gen-bash-completions`
: Do the mechanical edits, so the steps that can be automated are.

What `release-check` deliberately does not settle: whether the man pages and
completions were regenerated, since neither embeds a version and establishing
staleness means regenerating them; whether the CHANGELOG section actually
describes the release rather than merely existing; and everything after the
tag, which is manual because it cannot be undone. Those stay the releaser's
judgement.

## Step 1: bump the version-number

The version-number is not derived from the tag, it is written out in several
files. Rather than editing them by hand, run:

```bash
make version VERSION=X.Y.Z
```

Leaving out `VERSION` bumps the minor-number of the current version. Pass
`DRY_RUN=1` to see what would change without writing it. The helper behind the
target, `toolbox/xnvme_ver.py`, updates:

* `meson.build`: the `version:` field of `project()`, this is the authoritative
  one, everything built by meson derives from it

* `python/bindings/setup.py`: the `version=` argument

* `rust/xnvme-sys/Cargo.toml`: **two** places, the `version` of the package
  itself and the `xnvme` entry under `[package.metadata.system-deps]`, the
  latter is the version of the C library that the crate links against

* `docs/tooling/doxy.cfg`: `PROJECT_NUMBER`

* `cijoe/pyproject.toml`: the `version` of the `xnvme-cijoe` package, which
  ships the test-suite for a given xNVMe and tracks it

It errors out when a file does not contain the version where the helper expects
it, so add the file to the table in `toolbox/xnvme_ver.py` when a new one starts
embedding the version. The exception is `rust/xnvme/Cargo.toml`: that crate is a
name reservation on crates.io holding no bindings, so it is versioned
independently and left alone. `rust/xnvme-sys/Cargo.toml`, which carries the
actual bindings, tracks xNVMe as above.

Commit the result with the message `ver: bump to vX.Y.Z`. `make release-check`
should now report the version consistent across every file, and complain only
about the steps still ahead.

## Step 2: update man-pages and Bash completion-scripts

* Build and install the version-bumped **xNVMe**
* Then run: `make gen-man-pages gen-bash-completions`
* Commit the man-pages with message: `docs(man): update for vX.Y.Z`
* Commit the Bash-completion-scripts with message:
  `feat(toolbox/completions): update for vX.Y.Z`

## Step 3: update `CONTRIBUTORS.md`

* List who is missing with `make contributors`, which compares the authors
  since the previous tag against the file and prints only the difference
* Add them to the current-release list, and move the previous release's
  entries down
* Commit changes with the message: `CONTRIBUTORS: update for vX.Y.Z`

## Step 4: update `CHANGELOG.rst`

* Go over the changes and summarize the different scopes. `git log
  <previous-tag>..HEAD --pretty=%s` grouped by scope is a starting point, but
  the entry is a summary for users, not a list of commits
* Call out anything that breaks the public API in its own section, since that
  is what a reader upgrading needs first. Commits marked `!` in their subject
  are the ones to look for
* Commit changes with the message: `CHANGELOG: update for vX.Y.Z`

Also check that `ISSUES.rst` still describes the known issues, the changelog
points at it.

## Step 5: open a pull-request for the release-prep commits

`make release-check` should pass before you open it; if it does not, the
pull-request is not ready. The release-prep commits (version bump, man-pages,
completions, CONTRIBUTORS, CHANGELOG) then go through review like any other
change, see {ref}`sec-contributing-process`.

* Get review and wait for tests to finalize / pass
* Double-check the generated docs at <https://xnvme.io/> (served from `main`)
* Merge onto `main`

## Step 6: tag `main` as `vX.Y.Z`

From here on nothing can be undone, which is why there is no `make release`
that would run these for you. Tags so far are lightweight tags on the merged
CHANGELOG commit:

```bash
git tag vX.Y.Z
git push origin vX.Y.Z
```

Pushing a `v*` tag triggers the `verify` workflow again, and it is that run
whose artifacts are attached to the release, so let it finish before
continuing. Budget about an hour for it.

## Step 7: create a release on GitHub

* Go to the GitHub page and create a release for the tag `vX.Y.Z`
* Use a title similar to the previous releases
* Add content similar to the previous releases, that is, refer to the
  `CHANGELOG.rst` and click the "Generate release notes" button
* Add the artifacts generated on the **tag** (src-archives,
  test-verify-results, Python sdist)

CI uploads the source-archives under un-versioned names, so download them from
the tag-run and rename before attaching:

* `xnvme-src.tar.gz` from `xnvme-src-archive` becomes `xnvme-X.Y.Z.tar.gz`, the
  source archive without subprojects
* `xnvme-src.tar.gz` from `xnvme-src-archive-with-subprojects` becomes
  `xnvme-fat-X.Y.Z.tar.gz`, the full source archive including SPDK sources

## Step 8: publish the Python package

Publish the same package that was used for testing and provided as a release
artifact, rather than building a fresh one. Download it, rename it, then upload
it:

```bash
mv xnvme-py-sdist.tar.gz xnvme-X.Y.Z.tar.gz
twine upload xnvme-X.Y.Z.tar.gz
```

## Step 9: publish the Rust crate

* Create an API token on <https://crates.io>
* Set up credentials locally with `cargo login`
* Upload: `cd rust/xnvme-sys && cargo publish`

## Step 10: downstream packaging

The packages listed under {ref}`sec-contributing-packaging` are updated after
the release is published, since they consume the tagged source-archive and its
checksum. The ones maintained by the project are AUR and brew, the rest are
maintained elsewhere and pick the release up on their own schedule.
