#!/bin/sh
mkdir -p package
cd package
rm *
cp ../README.md ./
cp ../LICENSE ./
cp ../qmm2.json ./
cp ../bin/release/x86/qmm2.so ./
cp ../bin/release/x86_64/qmm2_x86_64.so ./
cd ..
