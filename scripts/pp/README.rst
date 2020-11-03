Code-generator for pretty-printer functions
===========================================

To run the code-generator then ensure that you have ``Python3``, ``jinja2``,
and ``ctags`` installed.

Usage Example
-------------

Run it like so:

  ./scripts/pp/generator.py \
    --hdr-file include/libxnvme_spec.h \
    --namespaces "xnvme_spec" \
    --templates scripts/pp/ \
    --hdr-output include \
    --src-output src

The above command will generate the files:

  include/libxnvme_spec_pp.h
  src/xnvme_spec_pp.c

Containing pretty-printer functions for the enums, and structs in
``libxnvme_spec.h`` with C-Namespace ``xnvme_spec``. That is, it will generate
for symbols prefix by 'xnvme_spec', ignoring anything else defined in the
header.

TODO
----

1) Currently the code-generator only emits ``*_str(enum ...)`` functions, however,
once completed then it will also emit ``*_fpr(stream, struct *)`` and
``_pr(struct *)`` functions as well.
