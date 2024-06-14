.. _sec-api-rust:

Rust
====

xnvme-sys
---------

The :xref-xnvme-rust-xnvme-sys:`xnvme-sys<>` crate provides low-level bindings
to the xNVMe library, enabling efficient and direct interaction with NVMe
devices in Rust. It facilitates the integration of **xNVMe's** capabilities for
cross-platform NVMe management, focusing on performance and ease of use. The
crate includes interfaces and functions necessary for developers to control NVMe
hardware, supporting the development of high-performance storage applications.

xnvme
-----

While :xref-xnvme-rust-xnvme-sys:`xnvme-sys<>` focuses on raw/direct
bindings, the intent is to also provide idiomatic Rust bindings with the
crate :xref-xnvme-rust-xnvme:`xnvme<>`. However, currently, it is nothing but a
namesquat. Contributions and suggestions for this are welcome.
