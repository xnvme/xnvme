#!/usr/bin/env python3
# SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
#
# SPDX-License-Identifier: BSD-3-Clause
"""
Check that no libxnvme_*.h includes the umbrella libxnvme.h, which includes
them all and has them read as one set
"""

import re
import sys
from pathlib import Path


def main(include_dir):
    offenders = [
        path.name
        for path in sorted(Path(include_dir).glob("libxnvme_*.h"))
        if re.search(r'^#include [<"]libxnvme\.h[>"]', path.read_text(), re.M)
    ]
    if offenders:
        print(f"include the umbrella libxnvme.h: {offenders}")
        return 1

    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1]))
