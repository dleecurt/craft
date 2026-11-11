#!/bin/sh
# Regenerate configure and Makefile.in (needs autoconf and automake).
set -e
cd "$(dirname "$0")"
autoreconf --install --force
echo "Now run: ./configure && make"
