#!/bin/bash
mkdir -p artifacts
          
# beosc
pushd .

cd src/beosc/examples
csound -o beadsynt.wav beadsynt.csd
ls *.wav
cp beadsynt.wav ../../../artifacts

popd

# Finished
ls artifacts/*
