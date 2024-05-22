# -*- coding: utf-8 -*-
#
# xNVMe documentation build configuration file, created by sphinx-quickstart
import os

from xnvme_ver import xnvme_ver

extensions = [
    "sphinx.ext.extlinks",
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

extlinks = {
    "xref-xnvme": ("https://xnvme.io/%s", None),
    "xref-github-xnvme": ("https://github.com/xnvme/xnvme/%s", None),
    "xref-github-xnvme-issues": ("https://github.com/xnvme/xnvme/issues/%s", None),
    "xref-github-xnvme-prs": ("https://github.com/xnvme/xnvme/pulls/%s", None),
    "xref-github-xnvme-discussions": (
        "https://github.com/xnvme/xnvme/discussions%s",
        None,
    ),
    "xref-discord-xnvme": ("https://discord.com/invite/XCbBX9DmKf%s", None),
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
    "xref-dpdk-guide-lock-pages": (
        "https://doc.dpdk.org/guides/windows_gsg/run_apps.html#grant-lock-pages-in-memory-privilege%s",
        None,
    ),
    "xref-dpdk-guide-install-virt2phys": (
        "https://doc.dpdk.org/guides/windows_gsg/run_apps.html#virt2phys%s",
        None,
    ),
    "xref-dpdk-guide-install-netuio": (
        "https://doc.dpdk.org/guides/windows_gsg/run_apps.html#netuio%s",
        None,
    ),
    "xref-dpdk-guide-install-kmods": ("https://git.dpdk.org/dpdk-kmods%s", None),
    "xref-nvme-specs": ("https://nvmexpress.org/specifications/%s", None),
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
    "navigation_depth": 4,
    "navigation_with_keys": False,
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

linkcheck_report_timeouts_as_broken = False
