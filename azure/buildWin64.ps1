$env:Path += ";C:\Program Files (x86)\Microsoft Visual Studio\2017\Enterprise\MSBuild\15.0\Bin"

dir
cd c:/
# Invoke-WebRequest -Uri "https://github.com/csound/csound/releases/download/6.14.0/Csound6.14.0-Windows_x64-binaries.zip" -Outfile winx64-binaries.zip
# 7z.exe x winx64-binaries.zip -o"C:/Program Files/Csound6_x64"
# cd "C:/Program Files/Csound6_x64"
# dir

Write-Debug "Downloading csound installer"

Invoke-WebRequest -Uri "https://github.com/csound/csound/releases/download/6.14.0/Csound6.14.0-Windows_x64-installer.exe" -Outfile csound-installer.exe

Write-Debug "Installing csound installer"

./csound-installer.exe /passive

cd D:/a/1/s/

mkdir build
cd build
cmake -G "Visual Studio 15 2017 Win64" -DBUILD_JSUSFX_OPCODES=OFF ..
msbuild.exe Project.sln /property:Platform=x64 /property:Configuration=Release
mkdir D:/a/1/a/Win64
Copy-Item "D:/a/1/s/build/Release/*" -Destination D:/a/1/a/Win64
