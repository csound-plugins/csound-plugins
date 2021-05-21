# Csound Plugins

For documentation on the plugins visit <https://csound-plugins.github.io/csound-plugins/>
The source code lives at <https://github.com/csound-plugins/csound-plugins>

The plugins in this collection are released under the LGPLv2 

---------------------------------------------------------

## Installation - Windows

In csound >= 6.16, the .dll files should be placed in:

    C:\Users\<User>\AppData\Local\csound\6.0\plugins64

(Create the folder if it does not exist)

For previous versions, copy the .dll files to csound's opcodes folder. If csound was 
installed via the official installer, this folder is normally set to:

    C:\Program Files\Csound6_x64\plugins64

----------------------------------------------------------

## Installation - macOS

In csound >= 6.16, place the .dylib files in in the folder:

    ~/Library/csound/6.0/plugins64

(Create the folder if it does not exist)

For previous versions, copy the .dylib files to csound's opcodes folder. If csound was 
installed via the official installer, this folder is normally set to:

    /Library/Frameworks/CsoundLib64.framework/Versions/6.0/Resources/Opcodes64

If csound was installed from source, the opcodes folder would normally be:

    ~/Library/Frameworks/CsoundLib64.framework/Versions/6.0/Resources/Opcodes64

----------------------------------------------------------

## Installation - Linux

In csound >= 6.16, place the .so files in in the folder:

    ~/.local/lib/csound/6.0/plugins64
    
(Create the folder if it does not exist)

For previous versions, copy the .so files to csound's opcodes folder. If csound was 
installed via a package manager, this folder is normally set to:

    /usr/lib/csound/plugins64-6.0

If csound was installed from source, the opcodes folder is normally placed in:

    /usr/local/lib/csound/plugins64-6.0