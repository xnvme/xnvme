<!--
SPDX-FileCopyrightText: Samsung Electronics Co., Ltd

SPDX-License-Identifier: BSD-3-Clause
-->

====================
 xNVMe Code Emitter
====================

The **xNVMe** Code emitter produces the following:

* The C API

  - Definitions
  - Pretty-printers (JSON + YAML)
  - Command-Construction Functions aka Preppers

* Language Bindings

  - Access to the C API via FFI
  - Python
  - Rust

The emitter is responsible for generating a lot of code that would otherwise
require manual maintenance. Additionally, then a technique of **artisinal**
code generation is utilized providing increasingly idiomatic language bindings
compare to the other binding generators.

NVMe Data Model
===============

A major part of the **xNVMe** API consists of definitions and helper functions
for concepts defined in the NVMe specification documents. These concepts are
represented in an NVMe Data Model, providing a comprehensive representation of
all NVMe entities.

This data model is produced by the ``senfd`` tool. Creating the model requires
all NVMe specification documents in ``.docx`` format. Since these documents are
only available to NVMe working group members and are strictly **not** public
domain, **xNVMe** provides a version of the data model in the repository at:

  ``xnvme/emitter/datamodel/nvme.model.document.json.tgz``

The document can be verified using a JSON schema available in the ``senfd``
repository.

API and FFI
===========

Initially, the use of a custom interface definition language (IDL) was
considered as the foundation for API and FFI generation. However, this approach
was discarded for practical reasons:

Requiring developers to learn another "language" for API definition and
documentation felt counterintuitive. Instead, it made more sense to rely on the
C language, which developers already know.

To streamline API/FFI generation, a C parser is necessary to simplify the
creation of bindings. The goal is to ensure that existing or new APIs comply
with FFI constraints. Any parts of a C API that do not meet these constraints
are excluded from FFI generation.

A C API can include definitions that are intended exclusively for C users—one
might even call these "idiomatic," much like certain patterns are considered
"Pythonic" for Python bindings or "idiomatic" for Rust bindings. As long as
the API provides alternative means to access specific functionality across FFI
boundaries, this approach remains valid.