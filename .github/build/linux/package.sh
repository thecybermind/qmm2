#!/bin/sh
rm -rf package

mkdir -p package
cp README.md ./package/
cp LICENSE ./package/
cp bin/release/qmm2.so ./package/
cd package
if [ -z "${GITHUB_TOKEN}" ]; then
  tar zcf ../qmm2-latest.tar.gz *
else
  tar zcf ../qmm2-${{ github.ref_name }}.tar.gz *
fi
cd ..
rm -rf package

mkdir -p package
cp README.md ./package/
cp LICENSE ./package/
cp bin/debug/qmm2-dbg.so ./package/
cd package
if [ -z "${GITHUB_TOKEN}" ]; then
  tar zcf ../qmm2-latest-debug.tar.gz *
else
  tar zcf ../qmm2-${{ github.ref_name }}-debug.tar.gz *
fi
cd ..
rm -rf package
