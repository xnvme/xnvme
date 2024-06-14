.. _sec-backends-freebsd-sync-ioctl:

NVMe Driver IOCTL
-----------------

The FreeBSD NVMe Driver IOCTL provides a rich interface for submitting commands
similar to that of the Linux NVMe Driver IOCTL. However, for completion-errors,
the FreeBSD NVMe Driver IOCTL provides the entire NVMe Completion result, where
the Linux IOCTL maps the completion-result to an ``errno`` value and returns
that instead.

In this case, the backend will transform such errors into the NVMe equivalent.
See the documentation on
:ref:`xnvme_cmd_ctx <sec-api-c-xnvme_cmd-struct-xnvme_cmd_ctx>`.