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

sudo apt-get install libsndfile1-dev
sudo apt-get install nasm

#return to main working dir after installing Csound
cd $SYSTEM_DEFAULTWORKINGDIRECTORY
ls

git submodule update --init --recursive --remote
git submodule foreach git pull origin master
git submodule status --recursive

mkdir build
cd build

cmake -DSKIP_FAST_MATH=True ..
make -j4
ls
mkdir $BUILD_ARTIFACTSTAGINGDIRECTORY/Linux
cp *.so $BUILD_ARTIFACTSTAGINGDIRECTORY/Linux
