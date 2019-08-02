.. _sec-building:

============================
 Building xNVMe from source
============================

**xNVMe** builds and runs on Linux, FreeBSD, and Windows. The latter is not
publicly supported, however, have a look at :ref:`sec-building-windows` for
more information.

Retrieve the source-code from TBD, by either cloning it recursively:

.. code-block:: bash

  # Retrieve xNVMe via git-repos
  git clone --recursive https://<TBD>/xnvme.git
  cd xnvme

Or by downloading the source-archive:

.. code-block:: bash

  # Retrieve xNVMe via source-archive
  wget https://<TBD>/xnvme-v0.0.7.tar.gz
  mkdir xnvme-v0.0.7
  tar xzf xnvme-v0.0.7.tar.gz -C xnvme-v0.0.7

Regardless of how you retrieved the source-code, the continue like so:

.. code-block:: bash

  # auto-configures xNVMe, builds dependencies, and builds xNVMe
  make

  # Install xNVMe
  make install

If you want to change the build-configuration, then have a look at the
following section. If you seek to build using a toolchain other than the GNU
Compiler Collection, then have a tool at the toolchain section.

Configuration
=============

A ``configure`` script is provided to configure the build. You can inspect
configuration options by invoking::

  ./configure --help

Or open the configure script with your favorite editor.

The following two subsections show common build-configurations of **xNVMe** for
Linux and FreeBSD.

Linux
-----

If you are on **Linux** with a ``.deb`` compatible package manager, then you
probably want:

.. code-block:: bash

  ./configure \
    --enable-be-linux \
    --enable-be-spdk \
    --enable-cli \
    --enable-tests \
    --enables--examples \
    --enable-debs
  make
  make install-deb

The above will:

* Configure **xNVMe**  to be built with the Linux and SPDK backends, command-line
  tools, tests, examples, and Debian packages
* Build **xNVMe**
* Install **xNVMe** using Debian packages

FreeBSD
-------

If you are on **FreeBSD**:

.. code-block:: bash

  ./configure \
    --enable-be-fbsd \
    --enable-be-spdk \
    --enable-cli \
    --enable-tests \
    --enable-examples
  make
  make install

The above will:

* Configure **xNVMe**  to be built with the FreeBSD and SPDK backends,
  command-line tools, tests, and examples.
* Build **xNVMe**
* Install **xNVMe**

The following section provides some information on using a toolchain other than
the default, which is ``gcc``.

Toolchain
=========

The toolchain used for building **xNVMe**  must support C11 and OpenMP. Recent
versions of ``gcc``, ``icc``, ``clang``, and ``pgi`` are known to satisfy these
requirements well.

The default is to use to ``gcc``, however, if you want to change it, then set
the ``CC`` **environment variable** for the ``./configure`` script, **and**
pass ``CC`` as **argument** to make, like so:

.. code-block:: bash

  CC=icc ./configure <YOUR_OPTIONS_HERE>
  make CC=icc
  make install CC=icc

The **xNVMe** build-system is a portable ``make`` frontend to ``cmake``.
``make`` is used to provide support for the conventional common-case use as
seen above and a shell-script, at ``REPOS/configure``, is used for
configuration, that is, for instrumenting ``cmake``.

Note, that there is caveat for ``clang`` on FreeBSD. On FreeBSD, ``clang`` is
shipped as the system-compiler with OpenMP support disabled, so you would have
to install/enable ``clang`` with explicit OpenMP support.

Note, the ``pgi`` compiler has some issues linking with SPDK due to unstable
ABI for rte it seems.

Note, that ``icc`` works well after you bought a license and installed it
correctly. There is a free option with Intel System Suite 2019.

So, the path of least fiddling around is to just install the following:

On **Freebsd**:

Ensure that you have kernel source in ``/usr/src``, then install the following:

.. code-block:: bash

  # For xNVMe
  pkg install \
    bash \
    cmake \
    gcc \
    git \
    gmake

  # These are for DPDK/SPDK
  pkg install \
    autoconf \
    automake \
    e2fsprogs-libuuid \
    nasm \
    openssl \
    libnuma-dev \
    cunit \
    python

On **Linux** (Debian / Ubuntu):

.. code-block:: bash

  apt install \
    cmake \
    gcc \
    make \
    uuid-dev \
    libnuma-dev

On **Alpine Linux**:

.. code-block:: bash

  apk add \
    build-base \
    numactl-dev \
    util-linux-dev \
    cmake \
    musl-dev \
    make

Then you are good to go.

.. _sec-building-windows:

Windows
-------

Windows is not supported in public domain. However, if you want to roll your
support into **xNVMe**, then you could follow the pointers below.

C11 support is quite poor with most compilers on Windows except for Intel ICC
and the GCC port `TDM-GCC <http://tdm-gcc.tdragon.net/>`_.

A backend implementation for Windows could utilize an IO path similar to that
of the ``XNVME_BE_LINUX`` and ``XNVME_BE_FBSD`` for read/write. And then wrap
all other NVMe commands around an IOCTL interface.

Such as the one provided by the Open-Source NVMe Driver for Windows:

* https://svn.openfabrics.org/svnrepo/nvmewin/

Or use the IOCTL interface of the built-in NVMe driver:

* https://docs.microsoft.com/en-us/windows/win32/fileio/working-with-nvme-devices

A bit of care has been taken in **xNVMe**, e.g. buffer-allocation, to support /
run on Windows. So, you "only" have to worry about the above.

Cross-compiling for ARM on x86
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**This is currently not supported with the SPDK backend**

In case you do not have the build-tools available on your ARM target, then you
can cross-compile g by parsing ``CC`` parameter to make e.g.:

.. code-block:: bash

  CC=aarch64-linux-gnu-gcc-7 ./configure <config_options>
  make CC=aarch64-linux-gnu-gcc-7

Then transfer and unpack ``xnvme0.tar.gz`` from the ``build`` directory to your
ARM machine.

Notes on SPDK library bundling
------------------------------

Overview of SPDK API usage, library dependencies, and bundling

# Memory / buffer management
spdk_env_dpdk:
- spdk_dma_free
- spdk_dma_malloc
- spdk_dma_realloc
- spdk_vtophys

# NVMe driver for controller and namespace management
spdk_nvme:
- spdk_nvme_cpl_is_error
- spdk_nvme_ctrlr_alloc_io_qpair
- spdk_nvme_ctrlr_alloc_io_qpair
- spdk_nvme_ctrlr_cmd_admin_raw
- spdk_nvme_ctrlr_cmd_io_raw_with_md
- spdk_nvme_ctrlr_free_io_qpair
- spdk_nvme_ctrlr_get_data
- spdk_nvme_ctrlr_get_default_io_qpair_opts
- spdk_nvme_ctrlr_get_ns
- spdk_nvme_ctrlr_get_num_ns
- spdk_nvme_ctrlr_process_admin_completions
- spdk_nvme_detach
- spdk_nvme_ns_get_data
- spdk_nvme_ns_is_active
- spdk_nvme_probe
- spdk_nvme_qpair_process_completions
- spdk_nvme_qpair_process_completions
- spdk_nvme_transport_id_compare

# For SPDK environment initialization
spdk_env_dpdk:
- spdk_env_init
- spdk_env_opts_init

# To reduce the chattiness of SPDK/DPDK

spdk_log:
- spdk_log_set_print_level

rte_log:
- rte_log_set_global_level

The API and directly used SPDK libraries in turn has dependencies, so we need to
link against those as well. Which is a alot... transport implementations,
utility libraries, bunch of RTE stuff / DPDK.

------

Reasons to bundle SPDK:

- Avoid DPDK / SPDK linkage with xNVMe derived work, that is, derived work does
  not have to link with all the DPDK / SPdK libraries, then can just link with
  xNVMe
- Support LTO accross DPDK / SPDK for xNVMe and xNVMe derived work
