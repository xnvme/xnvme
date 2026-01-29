# xNVMe Documentation Tooling

This directory contains the build tooling for xNVMe documentation.

## System Requirements

The following command-line tools must be installed on your system:

| Tool | Purpose | Installation |
|------|---------|--------------|
| `doxygen` | Parse C headers for API docs | `apt install doxygen` / `brew install doxygen` |
| `universal-ctags` | Generate symbol tags | `apt install universal-ctags` / `brew install universal-ctags` |
| `graphviz` | Generate diagrams | `apt install graphviz` / `brew install graphviz` |

On Debian/Ubuntu:
```bash
sudo apt install doxygen universal-ctags graphviz
```

On Fedora:
```bash
sudo dnf install doxygen ctags graphviz
```

On macOS:
```bash
brew install doxygen universal-ctags graphviz
```

## Installation

Install the documentation build environment using `pipx`:

```bash
cd /path/to/xnvme
pipx install docs/tooling
```

Or with pip in a virtual environment:

```bash
cd /path/to/xnvme
python -m venv .venv
source .venv/bin/activate
pip install docs/tooling
```

## CLI Commands

After installation, the following commands are available:

### Build HTML Documentation

```bash
xnvme-docs-build-html
```

Options:
- `--docs-root PATH` - Path to docs directory (auto-detected if not specified)
- `--output PATH` - Output directory for built docs (default: `docs/builddir/html`)
- `--fresh` - Force fresh build (equivalent to sphinx `-E` flag)

### Serve with Live Reload

```bash
xnvme-docs-serve
```

Options:
- `--docs-root PATH` - Path to docs directory (auto-detected if not specified)
- `--port PORT` - Port to serve on (default: 8000)
- `--host HOST` - Host to bind to (default: 127.0.0.1)
- `--open-browser` - Open browser automatically

### Clean Build Artifacts

```bash
xnvme-docs-clean
```

Options:
- `--docs-root PATH` - Path to docs directory (auto-detected if not specified)
- `--all` - Also clean generated API docs

### Generate API Documentation

```bash
xnvme-docs-apigen --tags /path/to/tags --output docs/api
```

Options:
- `--tags PATH` - Path to ctags file (required)
- `--output PATH` - Output directory (default: current directory)
- `--format {rst,md}` - Output format (default: md)
- `--log-level LEVEL` - Logging level

### Generate Toolchain Documentation

```bash
xnvme-docs-toolchain
```

Options:
- `--format {rst,md}` - Output format (default: md)
- `--output PATH` - Output directory (default: docs/toolchain)

### Deploy to GitHub Pages

```bash
xnvme-docs-dest --docs docs/builddir/html --site /path/to/gh-pages --ref refs/tags/v1.2.3
```

Options:
- `--docs PATH` - Path to built documentation
- `--site PATH` - Path to GitHub Pages repository
- `--ref REF` - Git reference (e.g., `refs/heads/main` or `refs/tags/v1.2.3`)
- `--url URL` - Base URL for versions.json (default: https://xnvme.io)

## Full Build Workflow

A typical documentation build workflow:

```bash
# 1. Install tooling
pipx install docs/tooling

# 2. Generate ctags (from xNVMe root)
make tags-xnvme-public

# 3. Run Doxygen
cd docs/tooling
doxygen doxy.cfg

# 4. Generate API docs
xnvme-docs-apigen --tags ../../tags --output ../api

# 5. Generate toolchain docs
xnvme-docs-toolchain

# 6. Build HTML
xnvme-docs-build-html

# 7. Serve locally for preview
xnvme-docs-serve --open-browser
```

## Directory Structure

```
docs/
├── tooling/
│   ├── pyproject.toml          # Package + CLI entry points
│   ├── doxy.cfg                # Doxygen configuration
│   └── xnvme_docs/             # Python package
│       ├── __init__.py
│       ├── xnvme_docs.py       # Core build tooling
│       ├── apigen.py           # API doc generation
│       ├── toolchain.py        # Toolchain doc generation
│       ├── dest.py             # GitHub Pages deployment
│       └── templates/
│           ├── api_section.md.jinja
│           ├── api_section.rst.jinja
│           ├── toolchain.md.jinja
│           └── toolchain.rst.jinja
├── conf.py                     # Sphinx configuration
├── index.rst                   # Main index
├── _static/                    # Static assets
├── api/                        # API documentation
├── builddir/                   # Generated output
└── ...
```

## Migration Notes

This tooling supports both RST and Markdown (MyST) formats:

- Existing RST files continue to work
- New generated files use Markdown by default
- Use `--format rst` with apigen/toolchain commands for RST output
