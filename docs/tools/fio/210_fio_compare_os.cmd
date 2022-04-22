fio \
 /usr/local/share/xnvme/xnvme-compare.fio \
 --section=default \
 --ioengine=external:/usr/local/lib/x86_64-linux-gnu/libxnvme-fio-engine.so \
 --filename=/dev/nvme0n1 \
 --xnvme_async=io_uring
