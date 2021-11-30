FROM refenv/qemu-nvme:latest

COPY xnvme xnvme

RUN apt-get -qy update && \
	apt-get -qy \
		-o "Dpkg::Options::=--force-confdef" \
		-o "Dpkg::Options::=--force-confold" upgrade && \
	apt-get -qy autoclean

WORKDIR xnvme

RUN bash scripts/pkgs/debian-bullseye.sh

RUN ./configure --disable-be-spdk && \
	make && \
	make install-deb && \
	make clean

WORKDIR /opt

CMD ["bash"]
