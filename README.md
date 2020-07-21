# Repository of plugins

This is a repository for plugins for [csound](https://csound.com/). 

# Plugins in this repo

### jsfx

jsfx support in csound, allows any REAPER's jsfx plugin to be loaded and controlled inside csound

### klib

very efficient hashtables for csound

### poly

Parallel and sequential multiplexing opcodes, they enable the creation and control of multiple 
instances of a csound opcode

### else

A miscellaneous collection of effects (distortion, saturation, ring-modulation), noise 
generators (low freq. noise, chaos attractors, etc), envelope generators, etc.

## pathtools

opcodes to handle paths and filenames in a cross-platform manner

# Download

Binaries can be installed via [risset](https://github.com/csound-plugins/risset)

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

The `nasm` version provided by XCode is too old. To solve that, it is necessary to install a recent version
of `nasm`. This can be easily done via `homebrew`:

    brew install nasm

If you have a default installation of homebrew, the build process will detect this version (`/usr/local/bin/nasm`) and 
will use that instead of the older one.

#### Build and install

```bash
mkdir build && cd build
cmake ..
make
sudo make install
```

### Windows

NB: the `jsfx` opcodes are not supported on windows at the moment
    
```bash
mkdir build && cd build
cmake ..
make
make install
```

# Documentation of all plugins

Go to [Documentation](https://csound-plugins.github.io/csound-plugins/)