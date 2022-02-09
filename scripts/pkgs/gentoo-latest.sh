#!/usr/bin/env bash
echo ""
echo ""
echo "build requires linking against ncurses AND tinfo, run the following before compilation:"
echo "export LDFLAGS=\"-ltinfo -lncurses\""

emerge-webrsync
#emerge --sync

# Install packages via emerge
emerge $(< scripts/pkgs/gentoo-latest.txt )
