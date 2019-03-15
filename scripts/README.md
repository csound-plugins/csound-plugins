# Dependencies

These are the dependencies needed in order to generate documentation / wiki

    * pandoc
    
## Installation

### Linux

    sudo apt install pandoc
    
### macOS

    brew install pandoc
    
### Windows

    choco install pandoc

# Usage

## fill-manual-pages

Generates a complete markdown file from a .md_ template. Supported tags are:

* `{example}`: substituted with the contents of the .csd file named after the same opcode

## generate-readmes

Generates a `README.md` file for a library based on the `manifest.json` file and on the documentation of each opcode. Each opcode mentioned in the `manifest.json` file needs to have an `{opcode}.md` manual page inside the `docs` folder, with a short description under the title `Abstract` (see provided examples)