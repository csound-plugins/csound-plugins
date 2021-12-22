#!/bin/bash
base=poly
pluginsdir=/usr/local/lib/csound/plugins64-6.0
includedir=/usr/local/include/csound
# includeH=/home/em/dev/forks/csound/H
libdir=/usr/local/lib/csound


lib=lib$base.so

mkdir -p build
cd build
cp ../$base.c .
cp ../*.h .

cmd="gcc -O2 -shared -msse2 -o $lib -Wall -std=c99 -fPIC -mfpmath=sse -ffast-math -Wall -Wextra \
 -Wno-missing-field-initializers -DUSE_DOUBLE -DB64BIT -I$includedir $base.c"

echo -e "\n\n\n\n----------------------------------------\n\n\n\n"

echo $cmd
$cmd

cmd="sudo cp $lib $pluginsdir"
echo $cmd
$cmd
