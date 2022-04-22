# Use the pcie identifier + nsid id instead of device-path
fio \
 /usr/local/share/xnvme/xnvme-compare.fio \
 --section=default \
 --ioengine=external:/usr/local/lib/x86_64-linux-gnu/libxnvme-fio-engine.so \
 --filename=0000\\:03\\:00.0 \
 --xnvme_dev_nsid=1
