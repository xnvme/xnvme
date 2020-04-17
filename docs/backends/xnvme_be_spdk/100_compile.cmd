gcc ../backends/xnvme_be_spdk/hello.c \
	-Wl,--whole-archive -Wl,--no-as-needed \
	-lxnvme \
	-Wl,--no-whole-archive -Wl,--as-needed \
	-laio -luuid -lnuma -pthread \
	-o hello
