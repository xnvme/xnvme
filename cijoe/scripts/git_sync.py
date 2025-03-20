"""
In a development environment where the project source is edited on a "local"
system, and deployed for build and test on another/different "remote" system.

Then one needs to transfer the source, or changes to the source, to the remote
system. This can be done with tools such as rsync and mutagen. However, these
can cause unwanted behavior when syncing repository data such as ``.git``.

Ideally, one would execute the required git commands, transferring the changes
to the remote, however, this can be tedious. Thus, this script, running the
various commands, is doing just that.

Here is what it does:

* remote: checkout main
* local: commit changes with message "autocommit ..."
* local: push changes to remote
* remote: checkout step.repository.branch

With the above, then git is utilized for syncing, and the cijoe-scripting takes
care of the need to switch branch remotely.
"""

import logging as log
from argparse import ArgumentParser


def add_args(parser: ArgumentParser):
    parser.add_argument("--upstream", type=str, required=True)
    parser.add_argument("--branch", type=str, required=True)
    parser.add_argument("--remote_alias", type=str, required=True)
    parser.add_argument("--local_path", type=str, required=True)
    parser.add_argument("--remote_path", type=str, required=True)


def git_remote_from_config(cijoe, remote_path):
    """Returns git-remote URI using configuration cijoe.transport.ssh"""

    hostname = cijoe.getconf("cijoe.transport.ssh.hostname", None)
    if not hostname:
        return None

    username = cijoe.getconf("cijoe.transport.ssh.username", None)
    port = cijoe.getconf("cijoe.transport.ssh.port", None)

    remote = "ssh://"
    if username:
        remote += f"{username}@"
    remote += f"{hostname}"
    if port:
        remote += f":{port}"
    remote += f"{remote_path}"

    return remote


def main(args, cijoe):
    """Entry point"""

    if any(
        arg not in args
        for arg in ["upstream", "branch", "remote_alias", "local_path", "remote_path"]
    ):
        log.error("missing script arguments")

    local_path = args.local_path

    upstream = args.upstream
    branch = args.branch

    remote_path = args.remote_path
    remote_alias = args.remote_alias
    remote_url = git_remote_from_config(cijoe, remote_path)
    if not remote_url:
        return 1

    # Remotely: clone repository from upstream
    err, _ = cijoe.run(f'[ -d "{remote_path}/.git" ]')
    if err:
        for cmd in [
            f"mkdir -p {remote_path}",
            f"git clone {upstream} {remote_path}",
        ]:
            err, _ = cijoe.run(cmd)
            if err:
                return err

    # Locally: ensure that we have the 'target' as a remote to push to
    err, state = cijoe.run_local("git remote -v")
    if err:
        return err
    if not [
        line
        for line in state.output()
        if line.startswith(f"{remote_alias} ") and line.endswith(" (push)")
    ]:
        err, _ = cijoe.run_local(f"git remote add {remote_alias} {remote_url}")

    # Remotely: checkout to 'main' to allow pushing 'repository.branch'
    err, _ = cijoe.run("git checkout main", cwd=remote_path)
    if err:
        return err
    for cmd in [
        'git commit -a -s -m "autocommit ..."',
        f"git push {remote_alias} {branch} -f",
    ]:
        # Locally: do auto-commit and push to remote
        err, state = cijoe.run_local(cmd, cwd=local_path)
        if err:
            if "nothing to commit" in state.output():
                continue
            return err

    err, _ = cijoe.run(f"git checkout {branch}", cwd=remote_path)
    if err:
        return err
