# Installation

## Binaries

Binaries can be installed via [risset](https://github.com/csound-plugins/risset)

```bash
risset install klib else poly pathtools
```

and releases are found here: https://github.com/csound-plugins/csound-plugins/releases


## From Source


The source lives at <https://github.com/csound-plugins/csound-plugins>


### Dependencies

* a compiler
* cmake
* csound >= 6.14
* nasm (https://www.nasm.us/)
    * Linux: `apt install nasm`
    * Windows: `choco install nasm`
    * macOS: `brew install nasm`

```
git clone https://github.com/csound-plugins/csound-plugins
cd csound-plugins
mkdir build
cd build
cmake ..
make 
sudo make install
```