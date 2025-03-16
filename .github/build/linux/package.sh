#!/bin/sh
mkdir -p package
cd package
rm *
cp ../README.md ./
cp ../LICENSE ./
cp ../bin/release/qmm2.so ./
cd ..
