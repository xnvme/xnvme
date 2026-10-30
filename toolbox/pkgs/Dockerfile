# An image with xNVMe's dependencies installed by its toolbox/pkgs script, for
# CI to build in and for anyone to try a distribution in. The 'distro-deps'
# workflow builds one per distribution, with the toolbox as the context.
ARG BASE
FROM ${BASE}
LABEL org.opencontainers.image.source="https://github.com/xnvme/xnvme"
COPY . /opt/toolbox
WORKDIR /opt
ARG SCRIPT
RUN bash -e ./toolbox/pkgs/${SCRIPT}
WORKDIR /root
CMD ["bash"]
