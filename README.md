# External plugins for csound

This is a repository for plugins for [csound](https://csound.com/). It includes
multiple plugins, where each plugin contains a series of opcodes.

--------------

# Documentation of all plugins

Go to [Documentation](https://csound-plugins.github.io/csound-plugins/)

--------------


# Plugins in this repo

### klib

very efficient hashtables (dictionaries) and other data structures for csound


### poly

Parallel and sequential multiplexing opcodes, they enable the creation and
control of multiple instances of a csound opcode


## beosc

additive synthesis implementing the loris model sine+noise


### else

A miscellaneous collection of effects (distortion, saturation, ring-modula
generators (low freq. noise, chaos attractors, etc), envelope generators, etc.


### jsfx

jsfx support in csound, allows any REAPER's jsfx plugin to be loaded and
controlled inside csound


## pathtools

opcodes to handle paths and filenames in a cross-platform manner


----------------

# Installation

The recommended way to install plugins is via *risset*
(https://github.com/csound-plugins/risset). *Risset* itself
can be installed via `pip install risset`.

Then, to install any plugin:

```bash
risset update
risset install <pluginname>
```

For example, to install `klib` and `poly`:

```bash
risset install klib poly
```

Using *risset* to install plugins also ensures integration
with other tools like `CsoundQt`. *Risset* also can be used
to show manual pages, list opcodes, etc.

----------------

# Note for Mac Users

You will probably have to overcome Apple's security mechanism to use the plugins.
Right-click on each plugin and choose "Open with Terminal". Confirm "Open" in the dialog panel.

To find the plugin location, you can run the command `risset info | grep pluginspath`.

----------------

# Download

Plugins can be manually downloaded from the releases page:

https://github.com/csound-plugins/csound-plugins/releases

The binaries need to be copied to the plugins directory. The
directory needs to be created if it does not exist.

| Platform | Csound Version | Plugins Path                                                   |
|----------|----------------|----------------------------------------------------------------|
| linux    | 6              | `$HOME/.local/lib/csound/6.0/plugins64`                        |
| linux    | 7              | `$HOME/.local/lib/csound/7.0/plugins64`                        |
| windows  | 6              | `C:\\Users\\$USERNAME\\AppData\\Local\\csound\\6.0\\plugins64` |
| windows  | 7              | `C:\\Users\\$USERNAME\\AppData\\Local\\csound\\7.0\\plugins64` |
| macos    | 6              | `$HOME/Library/csound/6.0/plugins64`                           |
| macos    | 7              | `$HOME/Library/csound/7.0/plugins64`                           |

----------------

# Build

```bash
git clone  https://github.com/csound-plugins/csound-plugins
cd csound-plugins
git submodule update --init --recursive
mkdir build
cd build
cmake ..
cmake --build . --parallel
cmake --install .
```
