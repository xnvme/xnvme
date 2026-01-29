(sec-toolchain-linux)=
# Linux

## Alpine Linux (latest)

Install the required toolchain and libraries by running the package installation
script provided with the **xNVMe** repository, as shown below. Ensure that you
have sufficient system privileges when doing so (e.g., run as `root` or with
`sudo`):

```bash
sudo ./xnvme/toolbox/pkgs/alpine-latest.sh
```

Or, run the commands contained within the script manually:

```{literalinclude} ../../../toolbox/pkgs/alpine-latest.sh
:language: bash
:lines: 8-
```

:::{note}
A Docker-image is provided via `ghcr.io`, specifically
`ghcr.io/xnvme/xnvme-deps-alpine-latest:next`. This Docker-image contains
all the software described above.
:::

Then go ahead and configure, build and install using `meson`:

```{literalinclude} ../../../toolbox/pkgs/alpine-latest-build.sh
:language: bash
:lines: 2-
```


:::{note}
There are issues with SPDK/DPDK due to incompatibilities with the standard
library provided by ``musl libc``. Additionally, the ``libexecinfo-dev``
package is no longer available on Alpine.
Additionally, libvfn also relies on ``libexecinfo-dev`` which is currently
not available for Alpine Linux. Thus, it is also disabled.
Pull-request fixing these issues are most welcome, until then, disable
libvfn and spdk on Alpine.

:::


## Arch Linux (latest)

Install the required toolchain and libraries by running the package installation
script provided with the **xNVMe** repository, as shown below. Ensure that you
have sufficient system privileges when doing so (e.g., run as `root` or with
`sudo`):

```bash
sudo ./xnvme/toolbox/pkgs/archlinux-latest.sh
```

Or, run the commands contained within the script manually:

```{literalinclude} ../../../toolbox/pkgs/archlinux-latest.sh
:language: bash
:lines: 8-
```

:::{note}
A Docker-image is provided via `ghcr.io`, specifically
`ghcr.io/xnvme/xnvme-deps-archlinux-latest:next`. This Docker-image contains
all the software described above.
:::

Then go ahead and configure, build and install using `meson`:

```{literalinclude} ../../../toolbox/pkgs/archlinux-latest-build.sh
:language: bash
:lines: 2-
```


:::{note}
The build is configured to install with ``--prefix=/usr`` this is
intentional such the the ``pkg-config`` files end up in the default search
path on the system. If you do not want this, then remove ``--prefix=/usr``
and adjust your ``$PKG_CONFIG_PATH`` accordingly.

:::


## Oracle Linux 10 (10)

Install the required toolchain and libraries by running the package installation
script provided with the **xNVMe** repository, as shown below. Ensure that you
have sufficient system privileges when doing so (e.g., run as `root` or with
`sudo`):

```bash
sudo ./xnvme/toolbox/pkgs/oraclelinux-10.sh
```

Or, run the commands contained within the script manually:

```{literalinclude} ../../../toolbox/pkgs/oraclelinux-10.sh
:language: bash
:lines: 8-
```

:::{note}
A Docker-image is provided via `ghcr.io`, specifically
`ghcr.io/xnvme/xnvme-deps-oraclelinux-10:next`. This Docker-image contains
all the software described above.
:::

Then go ahead and configure, build and install using `meson`:

```{literalinclude} ../../../toolbox/pkgs/oraclelinux-10-build.sh
:language: bash
:lines: 2-
```



## Oracle Linux 9 (9)

Install the required toolchain and libraries by running the package installation
script provided with the **xNVMe** repository, as shown below. Ensure that you
have sufficient system privileges when doing so (e.g., run as `root` or with
`sudo`):

```bash
sudo ./xnvme/toolbox/pkgs/oraclelinux-9.sh
```

Or, run the commands contained within the script manually:

```{literalinclude} ../../../toolbox/pkgs/oraclelinux-9.sh
:language: bash
:lines: 8-
```

:::{note}
A Docker-image is provided via `ghcr.io`, specifically
`ghcr.io/xnvme/xnvme-deps-oraclelinux-9:next`. This Docker-image contains
all the software described above.
:::

Then go ahead and configure, build and install using `meson`:

```{literalinclude} ../../../toolbox/pkgs/oraclelinux-9-build.sh
:language: bash
:lines: 2-
```



## Rocky Linux 9 (9.7)

Install the required toolchain and libraries by running the package installation
script provided with the **xNVMe** repository, as shown below. Ensure that you
have sufficient system privileges when doing so (e.g., run as `root` or with
`sudo`):

```bash
sudo ./xnvme/toolbox/pkgs/rockylinux-9.7.sh
```

Or, run the commands contained within the script manually:

```{literalinclude} ../../../toolbox/pkgs/rockylinux-9.7.sh
:language: bash
:lines: 8-
```

:::{note}
A Docker-image is provided via `ghcr.io`, specifically
`ghcr.io/xnvme/xnvme-deps-rockylinux-9.7:next`. This Docker-image contains
all the software described above.
:::

Then go ahead and configure, build and install using `meson`:

```{literalinclude} ../../../toolbox/pkgs/rockylinux-9.7-build.sh
:language: bash
:lines: 2-
```



## Rocky Linux 10 (10.1)

Install the required toolchain and libraries by running the package installation
script provided with the **xNVMe** repository, as shown below. Ensure that you
have sufficient system privileges when doing so (e.g., run as `root` or with
`sudo`):

```bash
sudo ./xnvme/toolbox/pkgs/rockylinux-10.1.sh
```

Or, run the commands contained within the script manually:

```{literalinclude} ../../../toolbox/pkgs/rockylinux-10.1.sh
:language: bash
:lines: 8-
```

:::{note}
A Docker-image is provided via `ghcr.io`, specifically
`ghcr.io/xnvme/xnvme-deps-rockylinux-10.1:next`. This Docker-image contains
all the software described above.
:::

Then go ahead and configure, build and install using `meson`:

```{literalinclude} ../../../toolbox/pkgs/rockylinux-10.1-build.sh
:language: bash
:lines: 2-
```



## CentOS Stream 10 (stream10)

Install the required toolchain and libraries by running the package installation
script provided with the **xNVMe** repository, as shown below. Ensure that you
have sufficient system privileges when doing so (e.g., run as `root` or with
`sudo`):

```bash
sudo ./xnvme/toolbox/pkgs/centos-stream10.sh
```

Or, run the commands contained within the script manually:

```{literalinclude} ../../../toolbox/pkgs/centos-stream10.sh
:language: bash
:lines: 8-
```

:::{note}
A Docker-image is provided via `ghcr.io`, specifically
`ghcr.io/xnvme/xnvme-deps-centos-stream10:next`. This Docker-image contains
all the software described above.
:::

Then go ahead and configure, build and install using `meson`:

```{literalinclude} ../../../toolbox/pkgs/centos-stream10-build.sh
:language: bash
:lines: 2-
```



## CentOS Stream 9 (stream9)

Install the required toolchain and libraries by running the package installation
script provided with the **xNVMe** repository, as shown below. Ensure that you
have sufficient system privileges when doing so (e.g., run as `root` or with
`sudo`):

```bash
sudo ./xnvme/toolbox/pkgs/centos-stream9.sh
```

Or, run the commands contained within the script manually:

```{literalinclude} ../../../toolbox/pkgs/centos-stream9.sh
:language: bash
:lines: 8-
```

:::{note}
A Docker-image is provided via `ghcr.io`, specifically
`ghcr.io/xnvme/xnvme-deps-centos-stream9:next`. This Docker-image contains
all the software described above.
:::

Then go ahead and configure, build and install using `meson`:

```{literalinclude} ../../../toolbox/pkgs/centos-stream9-build.sh
:language: bash
:lines: 2-
```



## Debian Stable (trixie)

Install the required toolchain and libraries by running the package installation
script provided with the **xNVMe** repository, as shown below. Ensure that you
have sufficient system privileges when doing so (e.g., run as `root` or with
`sudo`):

```bash
sudo ./xnvme/toolbox/pkgs/debian-trixie.sh
```

Or, run the commands contained within the script manually:

```{literalinclude} ../../../toolbox/pkgs/debian-trixie.sh
:language: bash
:lines: 8-
```

:::{note}
A Docker-image is provided via `ghcr.io`, specifically
`ghcr.io/xnvme/xnvme-deps-debian-trixie:next`. This Docker-image contains
all the software described above.
:::

Then go ahead and configure, build and install using `meson`:

```{literalinclude} ../../../toolbox/pkgs/default-build.sh
:language: bash
:lines: 2-
```



## Debian Oldstable (bookworm)

Install the required toolchain and libraries by running the package installation
script provided with the **xNVMe** repository, as shown below. Ensure that you
have sufficient system privileges when doing so (e.g., run as `root` or with
`sudo`):

```bash
sudo ./xnvme/toolbox/pkgs/debian-bookworm.sh
```

Or, run the commands contained within the script manually:

```{literalinclude} ../../../toolbox/pkgs/debian-bookworm.sh
:language: bash
:lines: 8-
```

:::{note}
A Docker-image is provided via `ghcr.io`, specifically
`ghcr.io/xnvme/xnvme-deps-debian-bookworm:next`. This Docker-image contains
all the software described above.
:::

Then go ahead and configure, build and install using `meson`:

```{literalinclude} ../../../toolbox/pkgs/default-build.sh
:language: bash
:lines: 2-
```



## Debian Oldoldstable (bullseye)

Install the required toolchain and libraries by running the package installation
script provided with the **xNVMe** repository, as shown below. Ensure that you
have sufficient system privileges when doing so (e.g., run as `root` or with
`sudo`):

```bash
sudo ./xnvme/toolbox/pkgs/debian-bullseye.sh
```

Or, run the commands contained within the script manually:

```{literalinclude} ../../../toolbox/pkgs/debian-bullseye.sh
:language: bash
:lines: 8-
```

:::{note}
A Docker-image is provided via `ghcr.io`, specifically
`ghcr.io/xnvme/xnvme-deps-debian-bullseye:next`. This Docker-image contains
all the software described above.
:::

Then go ahead and configure, build and install using `meson`:

```{literalinclude} ../../../toolbox/pkgs/default-build.sh
:language: bash
:lines: 2-
```



## Debian Testing (forky)

Install the required toolchain and libraries by running the package installation
script provided with the **xNVMe** repository, as shown below. Ensure that you
have sufficient system privileges when doing so (e.g., run as `root` or with
`sudo`):

```bash
sudo ./xnvme/toolbox/pkgs/debian-forky.sh
```

Or, run the commands contained within the script manually:

```{literalinclude} ../../../toolbox/pkgs/debian-forky.sh
:language: bash
:lines: 8-
```

:::{note}
A Docker-image is provided via `ghcr.io`, specifically
`ghcr.io/xnvme/xnvme-deps-debian-forky:next`. This Docker-image contains
all the software described above.
:::

Then go ahead and configure, build and install using `meson`:

```{literalinclude} ../../../toolbox/pkgs/default-build.sh
:language: bash
:lines: 2-
```



## Fedora (43)

Install the required toolchain and libraries by running the package installation
script provided with the **xNVMe** repository, as shown below. Ensure that you
have sufficient system privileges when doing so (e.g., run as `root` or with
`sudo`):

```bash
sudo ./xnvme/toolbox/pkgs/fedora-43.sh
```

Or, run the commands contained within the script manually:

```{literalinclude} ../../../toolbox/pkgs/fedora-43.sh
:language: bash
:lines: 8-
```

:::{note}
A Docker-image is provided via `ghcr.io`, specifically
`ghcr.io/xnvme/xnvme-deps-fedora-43:next`. This Docker-image contains
all the software described above.
:::

Then go ahead and configure, build and install using `meson`:

```{literalinclude} ../../../toolbox/pkgs/fedora-43-build.sh
:language: bash
:lines: 2-
```



## Fedora (42)

Install the required toolchain and libraries by running the package installation
script provided with the **xNVMe** repository, as shown below. Ensure that you
have sufficient system privileges when doing so (e.g., run as `root` or with
`sudo`):

```bash
sudo ./xnvme/toolbox/pkgs/fedora-42.sh
```

Or, run the commands contained within the script manually:

```{literalinclude} ../../../toolbox/pkgs/fedora-42.sh
:language: bash
:lines: 8-
```

:::{note}
A Docker-image is provided via `ghcr.io`, specifically
`ghcr.io/xnvme/xnvme-deps-fedora-42:next`. This Docker-image contains
all the software described above.
:::

Then go ahead and configure, build and install using `meson`:

```{literalinclude} ../../../toolbox/pkgs/fedora-42-build.sh
:language: bash
:lines: 2-
```



## Ubuntu Latest (questing)

Install the required toolchain and libraries by running the package installation
script provided with the **xNVMe** repository, as shown below. Ensure that you
have sufficient system privileges when doing so (e.g., run as `root` or with
`sudo`):

```bash
sudo ./xnvme/toolbox/pkgs/ubuntu-questing.sh
```

Or, run the commands contained within the script manually:

```{literalinclude} ../../../toolbox/pkgs/ubuntu-questing.sh
:language: bash
:lines: 8-
```

:::{note}
A Docker-image is provided via `ghcr.io`, specifically
`ghcr.io/xnvme/xnvme-deps-ubuntu-questing:next`. This Docker-image contains
all the software described above.
:::

Then go ahead and configure, build and install using `meson`:

```{literalinclude} ../../../toolbox/pkgs/default-build.sh
:language: bash
:lines: 2-
```


:::{note}
All tools and libraries are available via system package-manager.
:::


## Ubuntu LTS (noble)

Install the required toolchain and libraries by running the package installation
script provided with the **xNVMe** repository, as shown below. Ensure that you
have sufficient system privileges when doing so (e.g., run as `root` or with
`sudo`):

```bash
sudo ./xnvme/toolbox/pkgs/ubuntu-noble.sh
```

Or, run the commands contained within the script manually:

```{literalinclude} ../../../toolbox/pkgs/ubuntu-noble.sh
:language: bash
:lines: 8-
```

:::{note}
A Docker-image is provided via `ghcr.io`, specifically
`ghcr.io/xnvme/xnvme-deps-ubuntu-noble:next`. This Docker-image contains
all the software described above.
:::

Then go ahead and configure, build and install using `meson`:

```{literalinclude} ../../../toolbox/pkgs/default-build.sh
:language: bash
:lines: 2-
```


:::{note}
All tools and libraries are available via system package-manager.
:::


## Ubuntu LTS (Old) (jammy)

Install the required toolchain and libraries by running the package installation
script provided with the **xNVMe** repository, as shown below. Ensure that you
have sufficient system privileges when doing so (e.g., run as `root` or with
`sudo`):

```bash
sudo ./xnvme/toolbox/pkgs/ubuntu-jammy.sh
```

Or, run the commands contained within the script manually:

```{literalinclude} ../../../toolbox/pkgs/ubuntu-jammy.sh
:language: bash
:lines: 8-
```

:::{note}
A Docker-image is provided via `ghcr.io`, specifically
`ghcr.io/xnvme/xnvme-deps-ubuntu-jammy:next`. This Docker-image contains
all the software described above.
:::

Then go ahead and configure, build and install using `meson`:

```{literalinclude} ../../../toolbox/pkgs/default-build.sh
:language: bash
:lines: 2-
```


:::{note}
Installing liburing from source and meson + ninja via pip
:::


## Gentoo (latest)

Install the required toolchain and libraries by running the package installation
script provided with the **xNVMe** repository, as shown below. Ensure that you
have sufficient system privileges when doing so (e.g., run as `root` or with
`sudo`):

```bash
sudo ./xnvme/toolbox/pkgs/gentoo-latest.sh
```

Or, run the commands contained within the script manually:

```{literalinclude} ../../../toolbox/pkgs/gentoo-latest.sh
:language: bash
:lines: 8-
```

:::{note}
A Docker-image is provided via `ghcr.io`, specifically
`ghcr.io/xnvme/xnvme-deps-gentoo-latest:next`. This Docker-image contains
all the software described above.
:::

Then go ahead and configure, build and install using `meson`:

```{literalinclude} ../../../toolbox/pkgs/gentoo-latest-build.sh
:language: bash
:lines: 2-
```


:::{note}
In case you get ``error adding symbols: DSO missing from command line``,
during compilation, then add ``-ltinfo -lnurces`` to ``LDFLAGS`` as it is
done in the commands above.
The build is configured to install with ``--prefix=/usr`` this is
intentional such the the ``pkg-config`` files end up in the default search
path on the system. If you do not want this, then remove ``--prefix=/usr``
and adjust your ``$PKG_CONFIG_PATH`` accordingly.

:::


## openSUSE (tumbleweed-latest)

Install the required toolchain and libraries by running the package installation
script provided with the **xNVMe** repository, as shown below. Ensure that you
have sufficient system privileges when doing so (e.g., run as `root` or with
`sudo`):

```bash
sudo ./xnvme/toolbox/pkgs/opensuse-tumbleweed-latest.sh
```

Or, run the commands contained within the script manually:

```{literalinclude} ../../../toolbox/pkgs/opensuse-tumbleweed-latest.sh
:language: bash
:lines: 8-
```

:::{note}
A Docker-image is provided via `ghcr.io`, specifically
`ghcr.io/xnvme/xnvme-deps-opensuse-tumbleweed-latest:next`. This Docker-image contains
all the software described above.
:::

Then go ahead and configure, build and install using `meson`:

```{literalinclude} ../../../toolbox/pkgs/default-build.sh
:language: bash
:lines: 2-
```


:::{note}
All tools and libraries are available via system package-manager.
:::


