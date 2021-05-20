# $env:Path += ";C:\Program Files (x86)\Microsoft Visual Studio\2017\Enterprise\MSBuild\15.0\Bin"

# Dependencies
vcpkg integrate install
vcpkg install libsndfile:x64-windows
vcpkg list

# Build plugins

cd D:/a/1/s/

# Submodules

git submodule update --init --recursive --remote
git submodule foreach git pull origin master
git submodule status --recursive

mkdir build
cd build

cmake -DBUILD_JSUSFX_OPCODES=OFF -DCMAKE_TOOLCHAIN_FILE="${vcpkgpath}\scripts\buildsystems\vcpkg.cmake" -DVCPKG_TARGET_TRIPLET=x64-windows  ..
cmake --build . --config Release --parallel 4
# cmake -G "Visual Studio 15 2017 Win64" -DBUILD_JSUSFX_OPCODES=OFF ..
# msbuild.exe Project.sln /property:Platform=x64 /property:Configuration=Release
mkdir D:/a/1/a/Win64
Copy-Item "D:/a/1/s/build/Release/*" -Destination D:/a/1/a/Win64
