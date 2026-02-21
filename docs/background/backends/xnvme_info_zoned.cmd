# Create a Zoned NULL Block instance
modprobe null_blk nr_devices=1 zoned=1
# Open and query the Zoned NULL Block instance with xNVMe
xnvme info /dev/nullb0
# Remove the Zoned NULL Block instance
modprobe -r null_blk
