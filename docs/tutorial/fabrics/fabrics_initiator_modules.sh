#!/usr/bin/env bash
echo "## Loading modules for Fabrics initator"
modprobe nvme
modprobe nvme_fabrics
modprobe nvme_tcp
