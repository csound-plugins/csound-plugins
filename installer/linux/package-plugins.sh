#!/bin/bash
VERSION=$(cat ../../VERSION)

echo "Csound Plugins version: $VERSION"

BUILD_FOLDER=build
rm -fr $BUILD_FOLDER
mkdir $BUILD_FOLDER
cp ../../build/*.so $BUILD_FOLDER
cd $BUILD_FOLDER
apack csound-plugins-linux-$VERSION.zip *
cd ..
