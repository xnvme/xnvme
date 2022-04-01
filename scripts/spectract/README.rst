Spectract
=========

Spectract consists of python scripts to extract and parse tables
from NVMe specification documents to yaml.

The entrypoint is the script::

  spectract.py
  
This supports the command::

  spectract.py extract [-h] file

Where ``file`` is a yaml file formatted like the example below::

   - input: nvme-document.pdf
     pages: 103-104
     tables: 0-1
     name: xnvme_spec_cmd_cdw0
   - input: nvme-document.pdf
     pages: 104-106
     tables: 1-3
     name: xnvme_spec_cmd_common
   - input: nvme-document.pdf
     pages: 106
     tables: 1
     name: xnvme_spec_cmd_common_admin

Below is the definition of the keys: 

input
  The file to extract the table from

  string
  
pages
  The pages to extract the table from

  string with format 1 or 1-3

tables
  The indices of the desired tables in the page range

  string with format 0 or 0-3

name
  The name of the generated struct, enumerator, etc.

  string with no spaces
