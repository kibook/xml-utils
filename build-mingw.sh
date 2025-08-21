#!/bin/sh

set -e

out=build/$MSYSTEM

mkdir -p "$out"
make -j$(nproc) clean
make -j$(nproc) all
cp utils/xml-*/xml-*.exe "$out"
ldd "$out"/xml-*.exe | awk '{print $3}' | grep '^/mingw' | sort -u | xargs cp -t "$out"
