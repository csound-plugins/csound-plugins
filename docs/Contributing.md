# Contributing

To submit an opcode clone this repository and make a pull request

# Organisation

An opcode is normally implemented as part of a library, to allow for different
versions and related opcodes to share functionality.
Each library lives in its own directory.
The tree can be structured as follows:


    mylib/
      CMakeLists.txt
      manifest.json
      [ README.md ]
      src/
        mylib.c
      examples/
        foo.csd
        bar.csd
      docs/
        foo.md
        bar.md
        

For each opcode defined in mylib.c there should be an example `opcode.csd` 
and a manual page `opcode.md`. Optionally it is possible to include a README.md
where a short description of the opcodes in this library is given 

Put your tree under `src` and you should be able to build your plugin.

# Build

We use cmake as a build tool. For simple opcodes with no extra dependencies, 
a simple CMakeLists.txt would suffice:

    make_plugin(mylib src/mylib.c)


# Installation

At the root folder of this repository, do


    mkdir build
    cd build
    cmake ..
    make
    sudo make install



# Manifest

A manifest is used to automate documentation of the opcodes (wiki, pdf documentation, etc).
The manifest is a .json file. It should have the minimal form: 


```json
{
    "name": "mylib",
    "opcodes": ["foo", "bar"],
    "author": "name",
    "author_email": "name@email.com",
    "license": "LGPL",
    "description": "Description of this package",
    "url": "http://github.com/..."
}
        
```

In this case, three opcodes are defined, and these names should correspond to
the .csd example living inside of the `examples` folder, and a .md file 
living inside the `docs` folder.


