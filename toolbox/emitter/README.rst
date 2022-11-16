Code-emitter for spec. definitions
==================================

When running the code-emitter(``./emitter.py``), then a **model** is loaded by
reading all the ``.yaml`` files in the ``model`` directory.

For each **topic** in the **model**, then a corresponding code-emitter-template
is loaded from the ``templates/{topic}.template``, the template is populated
with data from the **model** and stored in ``output/{topic}.h``.

That is all.

Usage
-----

The following assumes that you have recent versions of: Python with
jinja2+yaml, clang-format, doxygen, and Makefile.

Run the emitter::

  python3 emitter.py

See ``--help`` on how to change input/output.

For code-formating, then run ``clang-format``::

  clang-format --style=file:../clang-format-h -i output/*.h

Then take a look at the generated code::

  cat output/*.h | less

Or, instead of the above, then just run ``make``, it does everything above.
