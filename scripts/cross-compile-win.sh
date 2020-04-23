#!/bin/bash

# call it from the root directoriy of the project:
# scripts/cross-compile-win.sh

plugins="poly;else;klib"


CC=x86_64-w64-mingw32-gcc
DLL_FOLDER=buildwin

rm -fr $DLL_FOLDER
mkdir -p $DLL_FOLDER

for plugin in $(echo $plugins | tr ';' ' '); do
    pushd .
    PLUGIN_SRC=src/$plugin/src
    cd $PLUGIN_SRC
    $CC -c -I/usr/local/include/csound $plugin.c
    $CC -O2 -shared -msse2 -o buildwin/lib$plugin.dll -std=c99 -Wall -mfpmath=sse \
        -ftree-vectorize -ffast-math -fomit-frame-pointer -mstackrealign -fPIC -DUSE_DOUBLE -DB64BIT \
        -I/usr/local/include -I/usr/local/include/csound $plugin.o
    rm *.o    
    popd
    # mv $PLUGIN_SRC/*.dll $DLL_FOLDER
done    
    