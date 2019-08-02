zoned mgmt-reset /dev/nvme0n2 --slba 0x0
zoned report /dev/nvme0n2 --slba 0x0 --limit 1
zoned append /dev/nvme0n2 --slba 0x0 --nlb 0
zoned append /dev/nvme0n2 --slba 0x0 --nlb 0
zoned append /dev/nvme0n2 --slba 0x0 --nlb 0
zoned report /dev/nvme0n2 --slba 0x0 --limit 1
