# Repository of plugins

This is a repository for plugins for [csound](https://csound.com/). 

**Version**: 6.14.1

### Note about versions

The version number (major.minor.revision) indicates the compatibility with the core csound project. 
The major and minor components correspond to the csound version, the revision number indicates the
version of the plugins themselves. 

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


# Installation from source

## Dependencies

* `csound` >= 6.14
* `nasm`: an assembler compiler needed by `jsusfx`

#### macOS

The `nasm` version provided by XCode is too old. To solve that, it is necessary to install a recent version
of `nasm`. This can be easily done via `homebrew`:

    brew install nasm

If you have a default installation of homebrew, the build process will detect this version (`/usr/local/bin/nasm`) and 
will use that instead of the older one.

### Windows

    choco install nasm

### Linux

    sudo apt install nasm

## After installing dependencies

    mkdir build & cd build
    cmake ..
    make
    sudo make install


# Documentation of all plugins

Go to [Documentation](https://csound-plugins.github.io/csound-plugins/)


## Offline Documentation

Static documentation is included and can be browsed by opening `site/index.html` in your browser. 
To be able to use the search function you need to:

    # install [mkdocs](https://www.mkdocs.org/):
    pip3 install mkdocs --user

    # generate the documentation and serve it
    mkdocs build && mkdocs serve

The documentation can then be browsed at http://127.0.0.1:8080
