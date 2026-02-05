zoned mgmt-reset /dev/nvme1n1 --slba 0x0
zoned write /dev/nvme1n1 --slba 0x0 --nlb 0
