.. _sec-api:

.. _sec-api-c:

#####
 API
#####

C 
=

For didactic purposes the **xNVMe** API can be broken into two parts: the :ref:`sec-api-c-core` API and the :ref:`sec-api-c-extended` API.

The :ref:`sec-api-c-core` API should be the starting point for anyone new to **xNVMe** and storage programming in general.
It describes the minimal set of functions, and their respective header-files, you need to get started with **xNVMe**, which essentially boils down to opening a device, creating a queue, and sending a command. 

The :ref:`sec-api-c-extended` API contains everything else you might need once you are ready to dive deeper into **xNVMe**, including, but not limited to, more command-sets, helpers for lba-ranges, and a representation of the NVMe specification. 

In reality, there is no division of the API. Instead, you can mix and match header-files as needed, or include everything through ``libxnvme.h``.
The ``libxnvme.h`` header is available for inspection below:

.. literalinclude:: ../../include/libxnvme.h
   :language: c

.. toctree::
   :maxdepth: 2
   :hidden:

   core/index
   extended/index
   examples/index
   python/index
   rust/index