"""
In a development environment where the project source is edited on a "local"
system, and deployed for build and test on another/different "remote" system.

Then one needs to transfer the source, or changes to the source, to the remote
system. This can be done with tools such as rsync and mutagen. However, these
can cause unwanted behavior when syncing repository data such as ``.git``.

Ideally, one would execute the required git commands, transferring the changes
to the remote, however, this can be tedious. Thus, this script, running the
various commands, is doing just that.

Here is what is does:

* remote: checkout main
* local: commit changes with message "autocommit ..."
* local: push changes to remote
* remote: checkout step.repository.branch

With the above, then git is utilized for syncing, and the cijoe-scripting takes
care of the need to switch branch remotely.
"""

import logging as log


def main(args, cijoe, step):
    """Entry point"""

    repos = step.get("with", {}).get("repository", {})

    path_local = repos.get("path", {}).get("local", "")
    path_remote = repos.get("path", {}).get("remote", "")
    branch = repos.get("branch", "")
    remote = repos.get("remote", "")

    err, _ = cijoe.run("git checkout main", cwd=path_remote)
    if err:
        return err

    for cmd in [
        "git remote -v",
        'git commit -a -s -m "autocommit ..."',
        f"git push {remote} {branch} -f",
    ]:
        err, state = cijoe.run_local(cmd, cwd=path_local)
        if err:
            if "nothing to commit" in state.output():
                continue
            return err

    err, _ = cijoe.run(f"git checkout {branch}", cwd=path_remote)
    if err:
        return err
