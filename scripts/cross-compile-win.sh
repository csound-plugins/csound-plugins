#!/bin/bash

# call it from the root directoriy of the project:
# scripts/cross-compile-win.sh

cplugins="poly;else;klib;pathtools"

CC=x86_64-w64-mingw32-gcc
CPP=x86_64-w64-mingw32-c++

DLL_FOLDER=$(realpath buildwin)

rm -fr $DLL_FOLDER
mkdir -p $DLL_FOLDER

for plugin in $(echo $cplugins | tr ';' ' '); do
    pushd .
    PLUGIN_SRC=src/$plugin/src
    cd $PLUGIN_SRC
    ls -l
    echo " --- $plugin ---"
    echo $CC -c -I/usr/local/include/csound $plugin.c
    $CC -c -I/usr/local/include/csound $plugin.c
    echo "--------"
    DLL=lib$plugin.dll

    $CC -O2 -shared -msse2 -o $DLL -std=c99 -Wall -mfpmath=sse \
        -ftree-vectorize -ffast-math -fomit-frame-pointer -mstackrealign -fPIC \
        -DUSE_DOUBLE -DB64BIT -DWIN32\
        -I/usr/local/include -I/usr/local/include/csound $plugin.o
    rm $plugin.o
    cp $DLL $DLL_FOLDER
    rm $DLL
    popd
    # mv $PLUGIN_SRC/*.dll $DLL_FOLDER
done    
    