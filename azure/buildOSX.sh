echo "AGENT_WORKFOLDER is $AGENT_WORKFOLDER"
echo "AGENT_WORKFOLDER contents:"
ls -1 $AGENT_WORKFOLDER
echo "AGENT_BUILDDIRECTORY is $AGENT_BUILDDIRECTORY"
echo "AGENT_BUILDDIRECTORY contents:"
ls -1 $AGENT_BUILDDIRECTORY
echo "SYSTEM_DEFAULTWORKINGDIRECTORY is $SYSTEM_DEFAULTWORKINGDIRECTORY"
echo "SYSTEM_DEFAULTWORKINGDIRECTORY contents:"
ls -1 $SYSTEM_DEFAULTWORKINGDIRECTORY
echo "BUILD_ARTIFACTSTAGINGDIRECTORY is $BUILD_ARTIFACTSTAGINGDIRECTORY"
echo "BUILD_ARTIFACTSTAGINGDIRECTORY contents:"
ls -1 $BUILD_ARTIFACTSTAGINGDIRECTORY


#return to main working dir after installing Csound
cd $SYSTEM_DEFAULTWORKINGDIRECTORY

curl -Ls -o nasm-2.14.02.zip 'https://www.nasm.us/pub/nasm/releasebuilds/2.14.02/macosx/nasm-2.14.02-macosx.zip'
ls
unzip nasm-2.14.02.zip
nasmdir=$PWD/nasm-2.14.02
cd $nasmdir
echo "In nasm dir $nasmdir"
ls
sudo cp * /usr/local/bin
cd ..

export PATH="$nasmdir:$PATH"
echo $PATH

nasm --version

brew install libsndfile

# Update submodules
git submodule update --init --recursive --remote
git submodule foreach git pull origin master
git submodule status --recursive

# Now build plugins

mkdir build
cd build
cmake ..
make -j4
ls
mkdir $BUILD_ARTIFACTSTAGINGDIRECTORY/MacOS
cp *.dylib $BUILD_ARTIFACTSTAGINGDIRECTORY/MacOs
