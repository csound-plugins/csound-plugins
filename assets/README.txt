# Csound Plugins

For documentation on the plugins visit <https://csound-plugins.github.io/csound-plugins/>
The source code lives at <https://github.com/csound-plugins/csound-plugins>

The plugins in this collection are released under the LGPLv2

These plugins require Csound 7.

---------------------------------------------------------

## Installation - Windows

Place the .dll files in:

    C:\Users\<User>\AppData\Local\csound\7.0\plugins64

(Create the folder if it does not exist)

----------------------------------------------------------

## Installation - macOS

Place the .dylib files in:

    ~/Library/csound/7.0/plugins64

(Create the folder if it does not exist)

macOS only loads code that has been signed, and since these plugins are
not notarized, browsers additionally mark downloaded files as
quarantined, which makes macOS refuse to load them. The
"macos-codesign" script fixes both: it removes the quarantine attribute,
adds an ad-hoc signature, and verifies the result. Run it in the folder
containing the plugins:

    python macos-codesign.py *.dylib

Note: if your csound build was hardened with library validation enabled,
ad-hoc signed plugins will still be rejected — the csound binary itself
must carry the "com.apple.security.cs.disable-library-validation"
entitlement, or the plugins must be signed with the same Apple Developer
identity as csound.

----------------------------------------------------------

## Installation - Linux

Place the .so files in:

    ~/.local/lib/csound/7.0/plugins64

(Create the folder if it does not exist)
