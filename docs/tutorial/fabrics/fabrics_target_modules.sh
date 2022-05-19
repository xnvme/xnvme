#!/usr/bin/env bash
echo "## Loading modules for Fabrics target"
modprobe nvme
modprobe nvmet
modprobe nvmet_tcp
