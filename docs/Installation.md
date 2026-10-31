# Installation

## Compiled Binaries

[Download binaries for all platforms](<https://github.com/csound-plugins/csound-plugins/releases>)

Download from a tagged build (`v1.x.y`) for a stable release, or use the development build ("latest"), which
contains the latest version.

!!! note 

    A csound plugin is a shared library (`.dylib` in macOS, `.so` in Linux, `.dll` in windows) which
    is loaded by csound and contains multiple opcodes.

Place the plugins in the user plugins folder (create the folder if it does not exist):

* **macOS**: `~/Library/csound/7.0/plugins64`
* **Linux**: `~/.local/lib/csound/7.0/plugins64`
* **Windows**: `C:\Users\<User>\AppData\Local\csound\7.0\plugins64`


## Installation from source

The source lives at <https://github.com/csound-plugins/csound-plugins>

-----

### Linux

```
sudo apt-get install libsndfile1-dev nasm
git clone https://github.com/csound-plugins/csound-plugins
cd csound-plugins
git submodule update --init --recursive --remote
git submodule foreach git pull origin master
mkdir build && cd build
cmake ..
cmake --build .
cmake --install .
```

> Csound 7 is required. Csound 6 is no longer supported.

--------

### MacOS

```
brew install libsndfile
curl -Ls -o nasm-2.14.02.zip 'https://www.nasm.us/pub/nasm/releasebuilds/2.14.02/macosx/nasm-2.14.02-macosx.zip'
unzip nasm-2.14.02.zip
export PATH="$(realpath nasm-2.14.02):$PATH"
git clone https://github.com/csound-plugins/csound-plugins
cd csound-plugins
git submodule update --init --recursive --remote
git submodule foreach git pull origin master
mkdir build && cd build
cmake ..
cmake --build .
cmake --install .
```

------

### Windows


```
git clone https://github.com/microsoft/vcpkg
.\vcpkg\bootstrap-vcpkg.bat
.\vcpkg\vcpkg integrate install
.\vcpkg\vcpkg install libsndfile:x64-windows

git submodule update --init --recursive --remote
git submodule foreach git pull origin master
mkdir build
cd build
cmake -A x64 -DBUILD_JSUSFX_OPCODES=OFF ..
cmake --build . --config Release
cmake --install .

# This last step simply copies Release\*.dll to "C:\Users\<User>\AppData\Local\csound\7.0\plugins64" 

```
