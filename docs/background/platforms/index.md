(sec-platforms)=

# Platforms

**xNVMe** supports one platform per operating system. Each platform registers its
available backend configs at build time. The sections below cover
system configuration, device identification, and instrumentation for each
supported platform.

**xNVMe** supports only little-endian hosts, such as x86_64 and aarch64. NVMe
data structures are little-endian, and **xNVMe** uses them in host byte order,
so the build stops on a big-endian host.

```{toctree}
:maxdepth: 2
:hidden:

linux/index
freebsd/index
macos/index
windows/index
```
