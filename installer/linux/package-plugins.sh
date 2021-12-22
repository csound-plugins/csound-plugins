#!/bin/bash
VERSION=$(cat ../../VERSION)

echo "Csound Plugins version: $VERSION"

BUILD_FOLDER=build
rm -fr $BUILD_FOLDER
mkdir $BUILD_FOLDER

cp ../../build/*.so $BUILD_FOLDER

# -------------------------------------------------------
cat > build/README.txt <<'EOF'

# Csound Plugins (linux)

## Installation

copy the .so files to `/usr/local/lib/csound/plugins64-6.0`

    $ sudo cp *.so /usr/local/lib/csound/plugins64-6.0

EOF

cd $BUILD_FOLDER
apack csound-plugins-linux-$VERSION.zip *
cd ..
