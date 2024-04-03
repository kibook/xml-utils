#!/bin/sh

out=build/$MSYSTEM

make
mkdir -p "$out"
cp utils/xml-*/xml-*.exe "$out"
ldd "$out"/xml-*.exe | awk '{print $3}' | grep '^/mingw64' | sort -u | xargs cp -t "$out"
