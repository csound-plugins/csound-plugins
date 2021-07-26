# Repository of plugins

This is a repository for plugins for [csound](https://csound.com/). 

# Plugins in this repo

### jsfx

jsfx support in csound, allows any REAPER's jsfx plugin to be loaded and controlled inside csound

### klib

very efficient hashtables (dictionaries) and other data structures for csound

### poly

Parallel and sequential multiplexing opcodes, they enable the creation and control of multiple 
instances of a csound opcode

### else

A miscellaneous collection of effects (distortion, saturation, ring-modulation), noise 
generators (low freq. noise, chaos attractors, etc), envelope generators, etc.

## pathtools

opcodes to handle paths and filenames in a cross-platform manner

----------------

# Documentation of all plugins

Go to [Documentation](https://csound-plugins.github.io/csound-plugins/)


# Download

Binaries can be installed via [risset](https://github.com/csound-plugins/risset) or downloaded
from https://github.com/csound-plugins/csound-plugins/releases


# Installation from source

## Dependencies

* `csound` >= 6.14
* `nasm`: an assembler compiler needed by `jsusfx`

### Linux

#### Install dependencies

```bash
sudo apt install nasm cmake
```

#### Build and install

```bash
mkdir build && cd build
cmake ..
make
sudo make install
```

### macOS

#### Dependencies

##### cmake 

Install it from https://cmake.org/install/

##### nasm

The `nasm` version provided by XCode is too old. The needed version is nasm >= 2.14. To solve that, it is necessary to install a recent version
of `nasm`. This can be easily done via `homebrew`:

    brew install nasm

If you don't have homebrew installed, the latest version can be downloaded from 
https://www.nasm.us/pub/nasm/releasebuilds/2.15.05/macosx/nasm-2.15.05-macosx.zip .
To install it, unpack the zip and cp the binary files to `/usr/local/bin`. 

#### Build and install

```bash
mkdir build && cd build
cmake ..
make
sudo make install
```

### Windows

NB: the `jsfx` opcodes are not supported on windows at the moment and are by default excluded from the build process.
    
```bash
mkdir build && cd build
cmake ..
make
make install
```

