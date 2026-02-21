# Create a NULL Block instance
modprobe null_blk nr_devices=1
# Open and query the NULL Block instance with xNVMe
xnvme info /dev/nullb0
# Remove the NULL Block instance
modprobe -r null_blk
