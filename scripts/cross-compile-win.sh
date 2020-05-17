#!/bin/bash

# call it from the root directoriy of the project:
# scripts/cross-compile-win.sh

c_plugins="poly;else;klib;pathtools"
cpp_plugins=""

CC=x86_64-w64-mingw32-gcc
CPP=x86_64-w64-mingw32-c++

DLL_FOLDER=$(realpath buildwin)

rm -fr $DLL_FOLDER
mkdir -p $DLL_FOLDER

for plugin in $(echo $c_plugins | tr ';' ' '); do
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

# rory opcodes

echo "----- building rory opcodes -----"
pushd .
cd src/rory/src
ls -l
$CPP -c -I/usr/local/include/csound opcodes.cpp

DLL=librory.dll

$CPP -O2 -shared -msse -msse2 -o $DLL -std=c++11 -Wall -mfpmath=sse \
    -ftree-vectorize -ffast-math -fomit-frame-pointer -mstackrealign -fPIC \
    -DUSE_DOUBLE -DUSE_LIB64 -DB64BIT -DWIN32\
    -I/usr/local/include -I/usr/local/include/csound opcodes.o

cp $DLL $DLL_FOLDER

rm opcodes.o
rm $DLL
    
popd

echo "-------- finished roryopcodes -------"


for plugin in $(echo $cpp_plugins | tr ';' ' '); do
    pushd .
    PLUGIN_SRC=src/$plugin/src
    cd $PLUGIN_SRC
    ls -l
    echo " --- $plugin ---"
    echo $CPP -c -I/usr/local/include/csound $plugin.cpp
    $CPP -c -I/usr/local/include/csound $plugin.cpp
    echo "--------"
    DLL=lib$plugin.dll

    $CPP -O2 -shared -msse -msse2 -o $DLL -std=c++11 -Wall -mfpmath=sse \
        -ftree-vectorize -ffast-math -fomit-frame-pointer -mstackrealign -fPIC \
        -DUSE_DOUBLE -DUSE_LIB64 -DB64BIT -DWIN32\
        -I/usr/local/include -I/usr/local/include/csound $plugin.o
    rm $plugin.o
    cp $DLL $DLL_FOLDER
    rm $DLL
    popd
done
