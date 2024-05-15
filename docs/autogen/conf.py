# -*- coding: utf-8 -*-
#
# xNVMe documentation build configuration file, created by sphinx-quickstart
import os

from xnvme_ver import xnvme_ver

extensions = [
    "sphinx.ext.todo",
    "sphinx.ext.imgmath",
    #    'sphinxcontrib.bibtex',
    "sphinxcontrib.jquery",
    "breathe",
]

exclude_patterns = ["autogen"]

source_suffix = ".rst"
master_doc = "index"
project = "xNVMe"
copyright = "2024, xNVMe"
version = xnvme_ver(os.path.join("..", "..", "meson.build"))
release = xnvme_ver(os.path.join("..", "..", "meson.build"))

pygments_style = "sphinx"

html_css_files = ["theme_overrides.css"]
html_static_path = [os.path.join("..", "_static")]

# Output file base name for HTML help builder.
htmlhelp_basename = "xnvmedoc"

breathe_projects = {project: os.path.join("builddir", "doxy", "xml")}
breathe_default_project = project
breathe_domain_by_extension = {"h": "c"}

html_logo = "../_static/xnvme.svg"

html_theme_options = {"analytics_id": "UA-159785887-1"}

html_theme = "pydata_sphinx_theme"

html_theme_options = {
    "navigation_depth": 4,
    "navbar_align": "left",
    "header_links_before_dropdown": 7,
    "secondary_sidebar_items": {
        "material/index": [],
    },
    "show_prev_next": False,
    "icon_links": [
        {
            "name": "GitHub",
            "url": "https://github.com/xnvme/xnvme",
            "icon": "fa-brands fa-square-github",
            "type": "fontawesome",
        },
        {
            "name": "Discord",
            "url": "https://discord.gg/XCbBX9DmKf",
            "icon": "fa-brands fa-discord",
        },
    ],
}

html_sidebars = {
    "material/index": [],
}

html_context = {
    "default_mode": "light",
}

html_show_sourcelink = False
