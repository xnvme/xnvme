Performance report
==================

The files contained here are utilized to create the SPDK / xNVMe performance
report. The files are:

* cover.jinja2.tmpl

  - Jinja2 template for cover page for the report
  - The pdf-generator (``rst2pdf``) requires a separate file to avoid
    headers/footers on the frontmatter

* bench.jinja2.rst
  
  - Jinja2 template for the body the report

* xnvme.png

  - Logo for the xNVMe project

These files can then be utilized to create a report, however, first the data
for the report needs to be acquired to populate the Jinja2 template above.
