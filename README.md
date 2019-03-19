# Repository of plugins

This is a repository for plugins for [csound](https://csound.com/). 

# Plugins in this repo

* `dict` plugins: very efficient hashtables for csound
* `poly` plugins: parallel and sequential multiplexing opcodes, 
         they enable to create and control multiple instances of a csound opcode
* `sched` plugins: schedule an instrument at specific events

# Documentation of all plugins

See the [wiki](https://github.com/csound-plugins/csound-plugins/wiki)


# Installation


    git clone https://github.com/gesellkammer/csound-plugins
    cd csound-plugins
    mkdir build & cd build
    cmake ..
    make
    sudo make install

# Generate the documentation

    ./scripts/generate-docs

# Contributing 

See [Contributing](https://github.com/csound-plugins/csound-plugins/wiki/contributing)
