#!/bin/bash

# This script creates the installer for macos. The created installer
# is a .zip file with the .dylib files and an installer script.
# This installer script, when run, installs the plugins in the correct
# Opcodes64 folder.

PLUGINS_VERSION=$(cat ../../VERSION)
CSOUND_VERSION=6.14
MACOS_VERSION=10.14
VERSION="$CSOUND_VERSION-$PLUGINS_VERSION-macos$MACOS_VERSION"

BUILD_DIR=build
DYLIBS_DIR=../../build
RELEASE_DIR=release
ROOT_OPCODES64=/Library/Frameworks/CsoundLib64.framework/Versions/6.0/Resources/Opcodes64
HOME_OPCODES64='$HOME/Library/Frameworks/CsoundLib64.framework/Versions/6.0/Resources/Opcodes64'

rm -fr $BUILD_DIR
mkdir -p $BUILD_DIR
rm -fr $RELEASE_DIR
mkdir -p $RELEASE_DIR

cp $DYLIBS_DIR/*.dylib $BUILD_DIR

# -------------------------------------------

cat > build/README.txt <<'EOF'

# Csound Plugins (macos)

## Installation

Inside a terminal, run the script csound-plugins-install.sh. It will ask for
administrator rights

    $ ./csound-plugins-install.sh

This will put the plugins inside your Opcodes64 folder, depending on your
installation (/Library/... if installed via the official csound installer, or
~/Library/... if installed from source)

To uninstall the plugins, also inside a terminal:

    $ ./csound-plugins-uninstall.sh

## Manual installation

Copy all .dylib files to
/Library/Frameworks/CsoundLib64.framework/Versions/6.0/Resources/Opcodes64 if
you installed csound via the installer, or to
~/Library/Frameworks/CsoundLib64.framework/Versions/6.0/Resources/Opcodes64 if
csound was installed from source.

EOF

# -------------------------------------------
#                 installer
# -------------------------------------------

cat > build/csound-plugins-install.sh <<EOF

#!/bin/bash

####################################################
#                                                  #
#                  Csound Plugins                  #
#                                                  #
####################################################

# This is the installer script for Csound Plugins $PLUGINS_VERSION
# Csound Version: $CSOUND_VERSION
# Created with macOS $MACOS_VERSION

# check that csound is installed
if [ -z "\$(which csound)" ]; then
    echo "Csound does not seem to be installed. Exiting"
    exit -1
fi

# find installation
if [ -d $ROOT_OPCODES64 ]; then
    DEST=$ROOT_OPCODES64
    SUDO=sudo
elif [ -d $HOME_OPCODES64 ]; then
    DEST=$HOME_OPCODES64
    SUDO=""
else
    echo "Did not find Opcodes64 folder. Exiting"
    exit -1
fi

echo "Found Opcodes64 in \$DEST"

## set -x

\$SUDO cp *.dylib \$DEST

# test if files are there
for f in *.dylib; do
    fullpath=\$DEST/\$f
    if [ ! -e \$fullpath ]; then
       echo "Installation failed, file not found: \$fullpath"
       exit -1
    fi
done

echo "Installation OK!"

EOF

chmod +x build/csound-plugins-install.sh

# -------------------------------------------
#                 uninstaller
# -------------------------------------------

DYLIBS=$(cd $DYLIBS_DIR; ls -1 *.dylib)

cat > build/csound-plugins-uninstall.sh <<EOF
#!/bin/bash

# check that csound is installed
if [ -z "\$(which csound)" ]; then
    echo "Csound does not seem to be installed. Exiting"
    exit -1
fi

# find installation
if [ -d $ROOT_OPCODES64 ]; then
    DEST=$ROOT_OPCODES64
elif [ -d $HOME_OPCODES64 ]; then
    DEST=$HOME_OPCODES64
else
    echo "Did not find Opcodes64 folder. Exiting"
    exit -1
fi

echo "Found Opcodes64 in \$DEST"

for f in $(echo $DYLIBS | tr '\n' ' '); do
    sudo rm -i "\$DEST/\$f"
done

EOF

chmod +x build/csound-plugins-uninstall.sh

# -------------------------------------------
#                 make zip
# -------------------------------------------
cd $BUILD_DIR
ZIP=csound-plugins-$VERSION.zip
zip  $ZIP *
cd ..
mv $BUILD_DIR/$ZIP $RELEASE_DIR

# -------------------------------------------
#                 make dmg
# -------------------------------------------

makedmg() {
    DMG_DIR=/tmp/csound6plugins-nonpkg-disk
    DMG_VOLUMEID="csound-plugins-$PLUGINS_VERSION"
    DMG_NAME="csound-plugins.dmg"
    rm -fr "$DMG_DIR"
    mkdir "$DMG_DIR"
    cd $BUILD_DIR
    cp -R * "$DMG_DIR"
    cd ..
    mkisofs -o "$DMG_NAME" -r -l -ldots -V "$DMG_VOLUMEID" "$DMG_DIR"
    mv $DMG_NAME $RELEASE_DIR      
}

# makedmg