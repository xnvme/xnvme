(sec-tutorials-transports-spdk)=

# SPDK Target

The SPDK target runs the user-space `nvmf_tgt` application bundled in
xNVMe's SPDK subproject. The PCIe device is bound to the userspace driver
by `xnvme-driver`, attached to `nvmf_tgt` as `Nvme0` through `rpc.py`, and
published as a TCP subsystem. The `cijoe/scripts/nvme_target_start.py`,
`nvme_target_probe.py`, and `nvme_target_stop.py` scripts drive this with
`--provider spdk`. They are wired into the dedicated
`workflows/transports-tcp-spdk.yaml` workflow whose three steps
(`nvme_target_start`, `nvme_target_probe`, `nvme_target_stop`) can be
selected individually.

## Dev loop

Run the demo workflow end-to-end:

```bash
cd cijoe
cijoe --config configs/debian-trixie-iommu_enabled.toml \
      --output /tmp/spdk-up \
      workflows/transports-tcp-spdk.yaml
```

This brings the target up, probes it from the initiator side, and tears
it down. The captured commands and output are what populates the
embedded report below. See the parent page for
[shared configuration](../index.md) and
[initiator usage](../index.md#initiator-usage).

When you want to hold the target up between iterations, the workflow
can be driven a step at a time
(`cijoe ... transports-tcp-spdk.yaml nvme_target_start` and `...
nvme_target_stop`), and the same logic is also reachable by invoking
`cijoe/scripts/nvme_target_start.py` directly. Those are cijoe
conveniences over the same underlying script.

```{note}
For additional documentation on the SPDK target, consult the SPDK
documentation on {xref-spdk-nvmeof}`SPDK-NVMe-oF<>`.
```

## Run report

```{raw} html
<div class="cijoe-report">
  <iframe src="../../../_static/transports/report-spdk.html"
          loading="lazy"
          title="cijoe report: NVMe TCP target, SPDK provider"></iframe>
</div>
```
