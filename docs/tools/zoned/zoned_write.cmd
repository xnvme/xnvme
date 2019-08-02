zoned mgmt-reset /dev/nvme0n2 --slba 0x0
zoned write /dev/nvme0n2 --slba 0x0 --nlb 0
