# Csound Plugins

For documentation on the plugins visit <https://csound-plugins.github.io/csound-plugins/>
The source code lives at <https://github.com/csound-plugins/csound-plugins>

The plugins in this collection are released under the LGPLv2

---------------------------------------------------------

## Installation - Windows

In csound 6 the .dll files should be placed in:

    C:\Users\<User>\AppData\Local\csound\6.0\plugins64

(Create the folder if it does not exist)

For csound 7, the plugins folder is:

    C:\Users\<User>\AppData\Local\csound\7.0\plugins64

----------------------------------------------------------

## Installation - macOS

In csound 6 place the .dylib files in in the folder:

    ~/Library/csound/6.0/plugins64

(Create the folder if it does not exist)

For csound 7 substitute "6.0" for "7.0":

    ~/Library/csound/7.0/plugins64

Since macOS 10.14 apple requests all binaries (libraries
and plugins included) to be signed from a paid developer
account. As an alternative, the user can be authorized
by the user in an ad-hoc manner, using the script
"macos-codesign". Run it as "python macos-codesign.py *.dylib"
to generate an entitlements file and ad an ad-hoc signature
to the binaries.

----------------------------------------------------------

## Installation - Linux

For csound 6, place the .so files in in the folder:

    ~/.local/lib/csound/6.0/plugins64

(Create the folder if it does not exist)

For csound 7 the plugins folder is:

    ~/.local/lib/csound/7.0/plugins64
