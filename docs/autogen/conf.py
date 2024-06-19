# -*- coding: utf-8 -*-
#
# xNVMe documentation build configuration file, created by sphinx-quickstart
import os

from xnvme_ver import xnvme_ver

extensions = [
    "sphinx.ext.extlinks",
    "sphinx.ext.todo",
    "sphinx.ext.imgmath",
    "sphinx_copybutton",
    "sphinx_tabs.tabs",
    "sphinxcontrib.jquery",
    "breathe",
]

exclude_patterns = ["autogen"]

source_suffix = ".rst"
master_doc = "index"
project = "xNVMe"
copyright = "2024, xNVMe"

version = "v" + xnvme_ver(os.path.join("..", "..", "meson.build"))
release = version

extlinks = {
    "xref-pkg-config-site": (
        "https://www.freedesktop.org/wiki/Software/pkg-config/%s",
        None,
    ),
    "xref-xnvme": ("https://xnvme.io/%s", None),
    "xref-xnvme-repository-dir": ("https://github.com/xnvme/xnvme/tree/main/%s", None),
    "xref-fio-repository-file": ("https://github.com/axboe/fio/blob/master/%s", None),
    "xref-spdk-repository-file": ("https://github.com/spdk/spdk/tree/master/%s", None),
    "xref-github-xnvme": ("https://github.com/xnvme/xnvme/%s", None),
    "xref-github-xnvme-issues": ("https://github.com/xnvme/xnvme/issues/%s", None),
    "xref-github-xnvme-prs": ("https://github.com/xnvme/xnvme/pulls/%s", None),
    "xref-github-xnvme-discussions": (
        "https://github.com/xnvme/xnvme/discussions%s",
        None,
    ),
    "xref-discord-xnvme": ("https://discord.com/invite/XCbBX9DmKf%s", None),
    "xref-meson": ("https://mesonbuild.com/%s", None),
    "xref-meson-options-builtin": (
        "https://mesonbuild.com/Builtin-options.html%s",
        None,
    ),
    "xref-linux-vfio": ("https://www.kernel.org/doc/Documentation/vfio.txt%s", None),
    "xref-linux-uio": (
        "https://www.kernel.org/doc/html/v4.14/driver-api/uio-howto.html%s",
        None,
    ),
    "xref-windows-wsl": ("https://github.com/wpdk/wpdk/blob/master/doc/wsl.md%s", None),
    "xref-packaging-chocolatey": ("https://chocolatey.org/%s", None),
    "xref-packaging-brew": ("https://brew.sh/%s", None),
    "xref-compilers-mingw": ("https://www.mingw-w64.org/%s", None),
    "xref-distro-armbian": ("https://www.armbian.com/%s", None),
    "xref-fw-netgate-1100": ("https://shop.netgate.com/products/1100-pfsense%s", None),
    "xref-fw-netgate-1100-manual": (
        "https://docs.netgate.com/pfsense/en/latest/solutions/sg-1100/%s",
        None,
    ),
    "xref-sbc-rockpi4b": ("https://rockpi.org/rockpi4%s", None),
    "xref-sbc-pikvm": ("https://pikvm.org/%s", None),
    "xref-sbc-pikvm-v3-hat": ("https://docs.pikvm.org/v3/%s", None),
    "xref-sbc-pikvm-v3-hat-notes": ("https://safl.dk/notebook/pikvm/%s", None),
    "xref-sbc-rpi4b": (
        "https://www.raspberrypi.com/products/raspberry-pi-4-model-b/%s",
        None,
    ),
    "xref-spdk": ("https://spdk.io/%s", None),
    "xref-spdk-nvmeof": ("https://spdk.io/doc/nvmf.html%s", None),
    "xref-nvme-specs": ("https://nvmexpress.org/specifications/%s", None),
    "xref-conventional-commits": ("https://www.conventionalcommits.org/%s", None),
    "xref-": ("", None),
}

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
    "collapse_navigation": False,
    "navigation_depth": 4,
    "navigation_with_keys": False,
    "navbar_align": "left",
    "navbar_end": ["version-switcher"],
    "header_links_before_dropdown": 8,
    "secondary_sidebar_items": {
        "**": ["page-toc"],
        "index": [],
        "material/index": [],
    },
    "show_prev_next": False,
    "show_nav_level": 2,
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
    "switcher": {
        "json_url": "https://xnvme.io/versions.json",
        "version_match": version,
    },
    "show_version_warning_banner": True,
    "check_switcher": False,
}

html_sidebars = {
    "getting_started/index": [],
    "material/index": [],
}

html_context = {
    "default_mode": "light",
}

html_show_sourcelink = False

linkcheck_report_timeouts_as_broken = False
