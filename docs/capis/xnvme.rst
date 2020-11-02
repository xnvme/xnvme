.. _sec-c-api-xnvme:

xnvme
########

.. _sec-c-api-xnvme-header:

Header
======

.. literalinclude:: ../../include/libxnvme.h
   :language: c

.. _sec-c-api-xnvme-enum:

Enums
=====


.. _sec-c-api-xnvme-enum-xnvme_async_opts:

xnvme_async_opts
----------------

.. doxygenenum:: xnvme_async_opts


.. _sec-c-api-xnvme-enum-xnvme_cmd_opts:

xnvme_cmd_opts
--------------

.. doxygenenum:: xnvme_cmd_opts


.. _sec-c-api-xnvme-enum-xnvme_pr:

xnvme_pr
--------

.. doxygenenum:: xnvme_pr



.. _sec-c-api-xnvme-struct:

Structs
=======


.. _sec-c-api-xnvme-struct-xnvme_be_attr:

xnvme_be_attr
-------------

.. doxygenstruct:: xnvme_be_attr
   :members:
   :undoc-members:


.. _sec-c-api-xnvme-struct-xnvme_be_attr_list:

xnvme_be_attr_list
------------------

.. doxygenstruct:: xnvme_be_attr_list
   :members:
   :undoc-members:


.. _sec-c-api-xnvme-struct-xnvme_enumeration:

xnvme_enumeration
-----------------

.. doxygenstruct:: xnvme_enumeration
   :members:
   :undoc-members:


.. _sec-c-api-xnvme-struct-xnvme_ident:

xnvme_ident
-----------

.. doxygenstruct:: xnvme_ident
   :members:
   :undoc-members:


.. _sec-c-api-xnvme-struct-xnvme_req:

xnvme_req
---------

.. doxygenstruct:: xnvme_req
   :members:
   :undoc-members:


.. _sec-c-api-xnvme-struct-xnvme_req_pool:

xnvme_req_pool
--------------

.. doxygenstruct:: xnvme_req_pool
   :members:
   :undoc-members:


.. _sec-c-api-xnvme-struct-xnvme_timer:

xnvme_timer
-----------

.. doxygenstruct:: xnvme_timer
   :members:
   :undoc-members:



.. _sec-c-api-xnvme-func:

Functions
=========


.. _sec-c-api-xnvme-func-XNVME_ILOG2:

XNVME_ILOG2
-----------

.. doxygenfunction:: XNVME_ILOG2


.. _sec-c-api-xnvme-func-XNVME_MAX:

XNVME_MAX
---------

.. doxygenfunction:: XNVME_MAX


.. _sec-c-api-xnvme-func-XNVME_MIN:

XNVME_MIN
---------

.. doxygenfunction:: XNVME_MIN


.. _sec-c-api-xnvme-func-xnvme_async_get_depth:

xnvme_async_get_depth
---------------------

.. doxygenfunction:: xnvme_async_get_depth


.. _sec-c-api-xnvme-func-xnvme_async_get_outstanding:

xnvme_async_get_outstanding
---------------------------

.. doxygenfunction:: xnvme_async_get_outstanding


.. _sec-c-api-xnvme-func-xnvme_async_init:

xnvme_async_init
----------------

.. doxygenfunction:: xnvme_async_init


.. _sec-c-api-xnvme-func-xnvme_async_poke:

xnvme_async_poke
----------------

.. doxygenfunction:: xnvme_async_poke


.. _sec-c-api-xnvme-func-xnvme_async_term:

xnvme_async_term
----------------

.. doxygenfunction:: xnvme_async_term


.. _sec-c-api-xnvme-func-xnvme_async_wait:

xnvme_async_wait
----------------

.. doxygenfunction:: xnvme_async_wait


.. _sec-c-api-xnvme-func-xnvme_be_attr_fpr:

xnvme_be_attr_fpr
-----------------

.. doxygenfunction:: xnvme_be_attr_fpr


.. _sec-c-api-xnvme-func-xnvme_be_attr_list:

xnvme_be_attr_list
------------------

.. doxygenfunction:: xnvme_be_attr_list


.. _sec-c-api-xnvme-func-xnvme_be_attr_list_fpr:

xnvme_be_attr_list_fpr
----------------------

.. doxygenfunction:: xnvme_be_attr_list_fpr


.. _sec-c-api-xnvme-func-xnvme_be_attr_list_pr:

xnvme_be_attr_list_pr
---------------------

.. doxygenfunction:: xnvme_be_attr_list_pr


.. _sec-c-api-xnvme-func-xnvme_be_attr_pr:

xnvme_be_attr_pr
----------------

.. doxygenfunction:: xnvme_be_attr_pr


.. _sec-c-api-xnvme-func-xnvme_buf_alloc:

xnvme_buf_alloc
---------------

.. doxygenfunction:: xnvme_buf_alloc


.. _sec-c-api-xnvme-func-xnvme_buf_free:

xnvme_buf_free
--------------

.. doxygenfunction:: xnvme_buf_free


.. _sec-c-api-xnvme-func-xnvme_buf_realloc:

xnvme_buf_realloc
-----------------

.. doxygenfunction:: xnvme_buf_realloc


.. _sec-c-api-xnvme-func-xnvme_buf_virt_alloc:

xnvme_buf_virt_alloc
--------------------

.. doxygenfunction:: xnvme_buf_virt_alloc


.. _sec-c-api-xnvme-func-xnvme_buf_virt_free:

xnvme_buf_virt_free
-------------------

.. doxygenfunction:: xnvme_buf_virt_free


.. _sec-c-api-xnvme-func-xnvme_buf_vtophys:

xnvme_buf_vtophys
-----------------

.. doxygenfunction:: xnvme_buf_vtophys


.. _sec-c-api-xnvme-func-xnvme_cmd_pass:

xnvme_cmd_pass
--------------

.. doxygenfunction:: xnvme_cmd_pass


.. _sec-c-api-xnvme-func-xnvme_cmd_pass_admin:

xnvme_cmd_pass_admin
--------------------

.. doxygenfunction:: xnvme_cmd_pass_admin


.. _sec-c-api-xnvme-func-xnvme_enumerate:

xnvme_enumerate
---------------

.. doxygenfunction:: xnvme_enumerate


.. _sec-c-api-xnvme-func-xnvme_enumeration_fpp:

xnvme_enumeration_fpp
---------------------

.. doxygenfunction:: xnvme_enumeration_fpp


.. _sec-c-api-xnvme-func-xnvme_enumeration_fpr:

xnvme_enumeration_fpr
---------------------

.. doxygenfunction:: xnvme_enumeration_fpr


.. _sec-c-api-xnvme-func-xnvme_enumeration_pp:

xnvme_enumeration_pp
--------------------

.. doxygenfunction:: xnvme_enumeration_pp


.. _sec-c-api-xnvme-func-xnvme_enumeration_pr:

xnvme_enumeration_pr
--------------------

.. doxygenfunction:: xnvme_enumeration_pr


.. _sec-c-api-xnvme-func-xnvme_ident_fpr:

xnvme_ident_fpr
---------------

.. doxygenfunction:: xnvme_ident_fpr


.. _sec-c-api-xnvme-func-xnvme_ident_from_uri:

xnvme_ident_from_uri
--------------------

.. doxygenfunction:: xnvme_ident_from_uri


.. _sec-c-api-xnvme-func-xnvme_ident_pr:

xnvme_ident_pr
--------------

.. doxygenfunction:: xnvme_ident_pr


.. _sec-c-api-xnvme-func-xnvme_is_pow2:

xnvme_is_pow2
-------------

.. doxygenfunction:: xnvme_is_pow2


.. _sec-c-api-xnvme-func-xnvme_lba_fpr:

xnvme_lba_fpr
-------------

.. doxygenfunction:: xnvme_lba_fpr


.. _sec-c-api-xnvme-func-xnvme_lba_fprn:

xnvme_lba_fprn
--------------

.. doxygenfunction:: xnvme_lba_fprn


.. _sec-c-api-xnvme-func-xnvme_lba_pr:

xnvme_lba_pr
------------

.. doxygenfunction:: xnvme_lba_pr


.. _sec-c-api-xnvme-func-xnvme_lba_prn:

xnvme_lba_prn
-------------

.. doxygenfunction:: xnvme_lba_prn


.. _sec-c-api-xnvme-func-xnvme_req_clear:

xnvme_req_clear
---------------

.. doxygenfunction:: xnvme_req_clear


.. _sec-c-api-xnvme-func-xnvme_req_cpl_status:

xnvme_req_cpl_status
--------------------

.. doxygenfunction:: xnvme_req_cpl_status


.. _sec-c-api-xnvme-func-xnvme_req_pool_alloc:

xnvme_req_pool_alloc
--------------------

.. doxygenfunction:: xnvme_req_pool_alloc


.. _sec-c-api-xnvme-func-xnvme_req_pool_free:

xnvme_req_pool_free
-------------------

.. doxygenfunction:: xnvme_req_pool_free


.. _sec-c-api-xnvme-func-xnvme_req_pool_init:

xnvme_req_pool_init
-------------------

.. doxygenfunction:: xnvme_req_pool_init


.. _sec-c-api-xnvme-func-xnvme_req_pr:

xnvme_req_pr
------------

.. doxygenfunction:: xnvme_req_pr


.. _sec-c-api-xnvme-func-xnvme_timer_bw_pr:

xnvme_timer_bw_pr
-----------------

.. doxygenfunction:: xnvme_timer_bw_pr


.. _sec-c-api-xnvme-func-xnvme_timer_elapsed:

xnvme_timer_elapsed
-------------------

.. doxygenfunction:: xnvme_timer_elapsed


.. _sec-c-api-xnvme-func-xnvme_timer_elapsed_msecs:

xnvme_timer_elapsed_msecs
-------------------------

.. doxygenfunction:: xnvme_timer_elapsed_msecs


.. _sec-c-api-xnvme-func-xnvme_timer_elapsed_nsecs:

xnvme_timer_elapsed_nsecs
-------------------------

.. doxygenfunction:: xnvme_timer_elapsed_nsecs


.. _sec-c-api-xnvme-func-xnvme_timer_elapsed_secs:

xnvme_timer_elapsed_secs
------------------------

.. doxygenfunction:: xnvme_timer_elapsed_secs


.. _sec-c-api-xnvme-func-xnvme_timer_elapsed_usecs:

xnvme_timer_elapsed_usecs
-------------------------

.. doxygenfunction:: xnvme_timer_elapsed_usecs


.. _sec-c-api-xnvme-func-xnvme_timer_pr:

xnvme_timer_pr
--------------

.. doxygenfunction:: xnvme_timer_pr


.. _sec-c-api-xnvme-func-xnvme_timer_start:

xnvme_timer_start
-----------------

.. doxygenfunction:: xnvme_timer_start


.. _sec-c-api-xnvme-func-xnvme_timer_stop:

xnvme_timer_stop
----------------

.. doxygenfunction:: xnvme_timer_stop

