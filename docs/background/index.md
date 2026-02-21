(sec-background)=

# Background

**xNVMe** abstracts platform-specific NVMe interfaces behind a common API.
Each supported operating system provides a **platform** implementation that
registers a set of **backend configs** — combinations of implementations for
admin, sync, async, memory, and device operations. At runtime, **xNVMe** selects
a config with **caps** matching the target device. If you wish to override the
backend configuration, you can do so via URI options.

For the design rationale and architectural details, see
{doc}`architecture/index`.

```{toctree}
:maxdepth: 2
:hidden:

platforms/index
backends/index
architecture/index
```
