.. _sec-c-apis:

########
 C APIs
########

A brief overview of the APIs provided by xNVMe follows.

**xnvme**

The :ref:`sec-c-apis-xnvme` API is the foundational API of **xNVMe**, providing
device enumeration, memory handling, asynchronous primitives and the
fundamental command-interface for communicating with devices.

The :ref:`sec-c-apis-xnvme-headers` is available for inspection in its raw form.

**lblk**

Building on top of the functionality of :ref:`sec-c-apis-xnvme` then the
:ref:`sec-c-apis-lblk` API provides helpers and structures specific to the NVMe
command-set provided by NVM/Logical/Conventional Namespaces. That is the usual
read/write, and optional commands such as the Simple-Copy-Command (SCC).

The :ref:`sec-c-apis-lblk-headers` is available for inspection in its raw form.

**znd**

Building on top of the functionality of :ref:`sec-c-apis-xnvme` then the
**znd** API provides helpers and structures specific to NVMe command-set
provided by Zoned Namespaces. It will be available as soon as the NVMe
specification is made public.

**xnvmec**

The :ref:`sec-c-apis-xnvmec` API provides functionality to create
command-line-interfaces for **xNVMe** related applications. The **xNVMe**
examples, tools, and tests use the library to provide a coherent command-line
interface with bash-completion-scripts and man-pages.

The :ref:`sec-c-apis-xnvmec-headers` is available for inspection in its raw
form.

.. toctree::
   :hidden:

   xnvme
   xnvme_headers
   lblk
   lblk_headers
   znd
   znd_headers
   xnvmec
   xnvmec_headers
