# Installation

## Compiled Binaries

[Download binaries for all platforms](<https://github.com/csound-plugins/csound-plugins/releases>)

Download from a tagged build (`v1.x.y`) for a stable release, or use the development build, which
contains the latest version.

!!! note 

    A csound plugin is a shared library (`.dylib` in macOS, `.so` in Linux, `.dll` in windows) which
    is loaded by csound and contains multiple opcodes.

In csound >= 6.16, place the plugins in (create the folder if it does not exist):

* **macOS**: `~/Library/csound/6.0/plugins64`
* **Linux**: `~/.local/lib/csound/6.0/plugins64`
* **Windows**: `C:\Users\<User>\AppData\Local\csound\6.0\plugins64`

For previous versions, put the plugins along csound's own plugins (this folder should already exist
and contain a multitude of files):

* **macOS**: `/Library/Frameworks/CsoundLib64.framework/Versions/6.0/Resources/Opcodes64` if csound was 
  installed via the official installer, or 
  `~/Library/Frameworks/CsoundLib64.framework/Versions/6.0/Resources/Opcodes64` if installed from source
* **Linux**: `/usr/local/lib/csound/plugins64-6.0` if installed from source, `/usr/lib/csound/plugins64-6.0` if
  installed by the package manager
* **Windows**: `C:\Program Files\Csound6_x64\plugins64`


## From Source

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
make -j4
sudo make install
```

With csound >= 6.16 the last step (`make install`) can be replaced by:

    cp *.so ~/.local/lib/csound/6.0/plugins64

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
make -j4
sudo make install
```

With csound >= 6.16 the last step (`make install`) can be replaced by:

    cp *.dylib ~/Library/csound/6.0/plugins64


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
cp Release\*.dll "C:\Program Files\Csound6_x64\plugins64"
# Or copy to "C:\Users\<User>\AppData\Local\csound\6.0\plugins64" if you want to install
# it only for the current user and csound >= 6.16

```