#!/bin/sh
rm -rf package

mkdir /p package
copy README.md ./package/
copy LICENSE ./package/
copy bin/release/qmm2.so ./package/
cd package
if [[ -z "${GITHUB_TOKEN}" ]]; then
  tar zcf ../qmm2-latest.tar.gz *
else
  tar zcf ../qmm2-${{ github.ref_name }}.tar.gz *
fi
cd ..
rm -rf package

mkdir /p package
copy README.md ./package/
copy LICENSE ./package/
copy bin/debug/qmm2.so ./package/
cd package
if [[ -z "${GITHUB_TOKEN}" ]]; then
  tar zcf ../qmm2-latest-debug.tar.gz *
else
  tar zcf ../qmm2-${{ github.ref_name }}-debug.tar.gz *
fi
cd ..
rm -rf package
