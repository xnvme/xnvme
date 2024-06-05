import logging as log


def main(args, cijoe, step):
    commands = [
        "sysctl -a hw",
        "(dmesg | grep memory) || true",
        "lspci || true",
    ]
    first_err = 0
    for command in commands:
        err, _ = cijoe.run(command)
        if err:
            log.error(f"failed({command}); err({err})")
        first_err = first_err if first_err else err

    return first_err
