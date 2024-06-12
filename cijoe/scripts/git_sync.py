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


def git_remote_from_config(cijoe, remote_path):
    """Returns git-remote URI using configuration cijoe.transport.ssh"""

    hostname = (
        cijoe.config.options.get("cijoe", {})
        .get("transport", {})
        .get("ssh", {})
        .get("hostname", {})
    )
    if not hostname:
        return None

    username = (
        cijoe.config.options.get("cijoe", {})
        .get("transport", {})
        .get("ssh", {})
        .get("username", {})
    )
    port = (
        cijoe.config.options.get("cijoe", {})
        .get("transport", {})
        .get("ssh", {})
        .get("port", {})
    )

    remote = "ssh://"
    if username:
        remote += f"{username}@"
    remote += f"{hostname}"
    if port:
        remote += f":{port}"
    remote += f"{remote_path}"

    return remote


def main(args, cijoe, step):
    """Entry point"""

    repos = step.get("with", {}).get("repository", {})

    local_path = repos.get("path", {}).get("local", "")

    upstream = repos.get("upstream", "")
    branch = repos.get("branch", "")

    remote_path = repos.get("path", {}).get("remote", "")
    remote_alias = repos.get("remote_alias", "")
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
