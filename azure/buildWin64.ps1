$env:Path += ";C:\Program Files (x86)\Microsoft Visual Studio\2017\Enterprise\MSBuild\15.0\Bin"

cd D:/a/1/s/

git submodule update --init --recursive --remote
git submodule foreach git pull origin master
git submodule status --recursive

mkdir build
cd build

cmake -DBUILD_JSUSFX_OPCODES=OFF ..
cmake --build . --config Release --parallel 4
# cmake -G "Visual Studio 15 2017 Win64" -DBUILD_JSUSFX_OPCODES=OFF ..
# msbuild.exe Project.sln /property:Platform=x64 /property:Configuration=Release
mkdir D:/a/1/a/Win64
Copy-Item "D:/a/1/s/build/Release/*" -Destination D:/a/1/a/Win64
