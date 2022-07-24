# Contributing

To submit an opcode clone this repository and make a pull request

# Organisation

An opcode is normally implemented as part of a library, to allow for different
versions and related opcodes to share functionality.
Each library lives in its own directory.
The tree can be structured as follows:


    mylib/
      CMakeLists.txt
      risset.json
      [ README.md ]
      src/
        mylib.c
      examples/
        foo.csd
        bar.csd
      doc/
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
	cmake --build .
	cmake --install .
    


# Manifest (risset.json)

A manifest is used both to automate documentation of the opcodes (wiki, pdf documentation, etc)
and make the opcode installable via risset. The manifest (named risset.json) is a .json file. 
It should have the minimal form: 


```json

{
  "name": "mylib",
  "version": "1.0.0",
  "opcodes": [
    "opcode1",
    "opcode2",
    ...
  ],
  "short_description": "A short description",
  "long_description": "A longer description",
  "csound_version": "6.17",
  "author": "Name Surname",
  "email": "name.surname@mail.com",
  "license": "LGPL",
  "repository": "https://github.com/csound-plugins/csound-plugins",
}
     
```   
