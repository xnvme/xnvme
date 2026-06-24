(sec-tutorials-transports-linux)=

# Linux Target

The Linux target uses the in-tree `nvmet` driver configured through
`configfs`. `xnvme-driver reset` rebinds the device to the kernel NVMe
driver so its local `/dev/nvmeXn1` is available; `nvmet` and `nvmet_tcp`
are loaded; the subsystem, namespace, and port are created as `configfs`
directories. The `cijoe/scripts/nvme_target_start.py`,
`nvme_target_probe.py`, and `nvme_target_stop.py` scripts drive this with
`--provider linux`. They are wired into the dedicated
`workflows/transports-tcp-linux.yaml` workflow whose three steps
(`nvme_target_start`, `nvme_target_probe`, `nvme_target_stop`) can be
selected individually.

## Dev loop

Run the demo workflow end-to-end:

```bash
cd cijoe
cijoe --config configs/debian-trixie-iommu_enabled.toml \
      --output /tmp/linux-up \
      workflows/transports-tcp-linux.yaml
```

This brings the target up, probes it from the initiator side, and tears
it down. The captured commands and output are what populates the
embedded report below. See the parent page for
[shared configuration](../index.md) and
[initiator usage](../index.md#initiator-usage).

When you want to hold the target up between iterations, the workflow
can be driven a step at a time
(`cijoe ... transports-tcp-linux.yaml nvme_target_start` and `...
nvme_target_stop`), and the same logic is also reachable by invoking
`cijoe/scripts/nvme_target_start.py` directly. Those are cijoe
conveniences over the same underlying script.

## Run report

```{raw} html
<div class="cijoe-report">
  <iframe src="../../../_static/transports/report-linux.html"
          loading="lazy"
          title="cijoe report: NVMe TCP target, Linux kernel provider"></iframe>
</div>
```
