Rust xNVMe Bindings
===================

* The ``xnvme-sys`` crate contains unsafe/direct/raw bindings to the xNVMe C Library

  * This is intended to provide data-structure-definitons and function-callables
  * STATUS: available as bindgen emitted wrapping of libxnvme

* The ``xnvme`` crate contains idiomatic xNVMe bindings

  * Built on top of 'xnvme-sys'
  * Provide a safe interface for consuming xNVMe
  * Envelop the ``xnvme-sys`` primitives in Rust idomatic constructs
  * STATUS: not implemented
