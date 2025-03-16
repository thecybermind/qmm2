#!/bin/sh
mkdir -p package
cd package
rm *
cp ../README.md ./
cp ../LICENSE ./
cp ../qmm2.json ./
cp ../bin/release/qmm2.so ./
cd ..
