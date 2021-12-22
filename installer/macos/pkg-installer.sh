#!/bin/bash

# info taken from http://bomutils.dyndns.org/tutorial.html

rm -fr ./build

mkdir build && cd build

PROJNAME="Csound6-Plugins"
OWVERSION=$(cat ../../VERSION)

IDENTIFIER="com.csound.csound6Environment.csoundPlugins"
OPCODES64="/Library/Frameworks/CsoundLib64.framework/Versions/6.0/Resources/Opcodes64"
DEST_DYLIBS="$OPCODES64/Extra"
DYLIBS_DIR="../dylibs"

# -----------------------------------------------------------------

PKG_DYLIBS_DIR=root"$DEST_DYLIBS"
PKGREFID=$IDENTIFIER.base.pkg
PKG_NAME="$PROJNAME.pkg"
DMG_NAME="$PROJNAME.dmg"

mkdir -p flat/base.pkg flat/Resources/en.lproj
mkdir -p "$PKG_DYLIBS_DIR"

cp "$DYLIBS_DIR"/*.dylib "$PKG_DYLIBS_DIR"

# ---------------------------------------

NUMBER_OF_FILES=$(find root | wc -l)
# KBYTES=$(du -b -s root)
KBYTES=$(du --apparent-size -s root | cut -f1)

( cd root && find . | cpio -o --format odc --owner 0:80 | gzip -c ) > flat/base.pkg/Payload

# --------------------------------------

ls
cat > PackageInfo <<EOF
<?xml version="1.0" encoding="utf-8"?>
<pkg-info overwrite-permissions="true" relocatable="false" identifier="$IDENTIFIER" postinstall-action="none" version="1" format-version="2" generator-version="InstallCmds-714 (19B88)" auth="root">
    <payload numberOfFiles="$NUMBER_OF_FILES" installKBytes="$KBYTES"/>
    <scripts>
        <postinstall file="./postinstall"/>
    </scripts>
    <bundle path="./Library/Frameworks/CsoundLib64.framework" id="" CFBundleShortVersionString="" CFBundleVersion=""/>
    <bundle-version>
        <bundle id=""/>
    </bundle-version>
    <upgrade-bundle>
        <bundle id=""/>
    </upgrade-bundle>
    <update-bundle/>
    <atomic-update-bundle/>
    <strict-identifier/>
    <relocate/>

</pkg-info>

EOF


# ---------------------------------------

cat > Distribution <<EOF
<?xml version="1.0" encoding="utf-8"?>
<installer-script minSpecVersion="1.000000" authoringTool="com.apple.PackageMaker" authoringToolVersion="3.0.3" authoringToolBuild="174">
    <title>Csound6 Plugins $OWNVERSION</title>
    <options customize="never" allow-external-scripts="no"/>
    <domains enable_anywhere="true"/>
    <installation-check script="pm_install_check();"/>
    <script>function pm_install_check() {
  if(!(system.compareVersions(system.version.ProductVersion,'10.14') >= 0)) {
    my.result.title = 'Failure';
    my.result.message = 'You need at least Mac OS X 10.14';
    my.result.type = 'Fatal';
    return false;
  }
  return true;
}
</script>
    <choices-outline>
        <line choice="choice1"/>
    </choices-outline>
    <choice id="choice1" title="Csound Plugins">
        <pkg-ref id="$PKGREFID"/>
    </choice>
    <pkg-ref id="$PKGREFID" installKBytes="$KBYTES" version="$OWNVERSION" auth="Root">#base.pkg</pkg-ref>
</installer-script>
EOF

# -----------------------------------------

cat > postinstall <<EOF
#!/bin/sh
/bin/ln -sF $DEST_DYLIBS/*.dylib $OPCODES64

EOF

# -----------------------------------------

cp PackageInfo flat/base.pkg
cp Distribution flat
cp postinstall flat/base.pkg

mkbom -u 0 -g 80 root flat/base.pkg/Bom

( cd flat && xar --compression none -cf "../$PKG_NAME" * )

# Put installer into a dmg disk image

DMG_DIR=/tmp/csound6plugins-disk
DMG_VOLUMEID="$PROJNAME-$OWNVERSION"
rm -fr "$DMG_DIR"
mkdir "$DMG_DIR"
cp "$PKG_NAME" "$DMG_DIR"
mkisofs -o "$DMG_NAME" -r -l -ldots -V "$DMG_VOLUMEID" "$DMG_DIR"
