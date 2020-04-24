# -*- coding: utf-8 -*-
#
# xNVMe documentation build configuration file, created by sphinx-quickstart
import sys
import os
from xnvme_ver import xnvme_ver

on_rtd = os.environ.get('READTHEDOCS', None) == 'True'

extensions = [
    'sphinx.ext.todo',
    'sphinx.ext.imgmath',
    'sphinxcontrib.bibtex',
    'breathe',
]

exclude_patterns = ['autogen']

templates_path = [os.sep.join(["_templates"])]
source_suffix = '.rst'
master_doc = 'index'
project = u'xNVMe'
copyright = u'2020, xNVMe'
version = xnvme_ver()
release = xnvme_ver()

pygments_style = 'sphinx'

if not on_rtd:
    import sphinx_rtd_theme
    html_theme = "sphinx_rtd_theme"
    html_theme_path = [sphinx_rtd_theme.get_html_theme_path()]

html_static_path = [os.sep.join(["..", "_static"])]

html_context = {
    "css_files": [
        "_static/theme_overrides.css",
    ]
}

# Output file base name for HTML help builder.
htmlhelp_basename = 'xnvmedoc'

breathe_projects = {
    project: os.path.abspath("build/doxy/xml/")
}
breathe_default_project = project
breathe_domain_by_extension = {
    "h" : "c"
}
#breathe_default_members = ('members', 'undoc-members')

