"""
Load the NVMe driver with poll-queues and setup misc. block-device options
"""
from pathlib import Path


def main(args, cijoe, step):
    err, _ = cijoe.run("modprobe -r nvme && modprobe nvme poll_queues=1")
    if err:
        return err

    for path in sorted(
        [
            Path(dev["uri"]).name
            for dev in cijoe.config.options.get("devices")
            if "bdev" in dev["labels"]
        ]
    ):
        commands = [
            f"echo 0 > /sys/block/{path}/queue/iostats",
            f"echo 2 > /sys/block/{path}/queue/nomerges",
            f"echo 0 > /sys/block/{path}/queue/wbt_lat_usec",
        ]
        for cmd in commands:
            err, _ = cijoe.run(cmd)
            if err:
                return err
