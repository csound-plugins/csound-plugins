#!/bin/bash
VERSION=$(cat ../../VERSION)

DLLFOLDER=dlls

echo "Csound Plugins version: $VERSION"

BUILD_FOLDER=build
rm -fr $BUILD_FOLDER
mkdir $BUILD_FOLDER
cp $DLLFOLDER/*.dll $BUILD_FOLDER
cp README.txt $BUILD_FOLDER
cd $BUILD_FOLDER
apack csound-plugins-win10-64bit-$VERSION.zip *
cd ..
