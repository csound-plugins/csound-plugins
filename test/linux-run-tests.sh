#!/bin/bash
mkdir -p artifacts
          
# beosc
pushd .

cd src/beosc/examples
csound -o beadsynt.wav beadsynt.csd
ls *.wav
cp beadsynt.wav ../../../artifacts

cd ../../else/examples
csound -o perlin3.flac perlin3-test.csd
cp perlin3.flac ../../../artifacts
csound --opcode-lib=../../../build/libelse.so bisect-interp1d-test.csd

popd

# Finished
ls artifacts/*
