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
#sudo apt-get install libasound2-dev
#sudo apt-get install libjack-dev
#sudo apt-get install portaudio19-dev
#sudo apt-get install libportmidi-dev
#sudo apt-get install libpulse-dev
#sudo apt-get install swig
#sudo apt-get install liblua5.1-0-dev
#sudo apt-get install python-dev
#sudo apt-get install puredata-dev
#sudo apt-get install default-jdk
#sudo apt-get install libfltk1.1-dev
#sudo apt-get install libfluidsynth-dev
#sudo apt-get install liblo-dev
#sudo apt-get install fluid
#sudo apt-get install ladspa-sdk
#sudo apt-get install libpng-dev
#sudo apt-get install dssi-dev
#sudo apt-get install libstk0-dev
#sudo apt-get install libgmm++-dev
#sudo apt-get install bison
#sudo apt-get install flex
#sudo apt-get install libportsmf-dev
#sudo apt-get install libeigen3-dev
#sudo apt-get install libcunit1-dev
#sudo apt-get install python-tk
sudo apt-get install nasm

# # sudo apt-get install libsndfile1
#git clone https://github.com/csound/csound.git
#cd csound
#mkdir build
#cd build
#cmake ..
#make -j4
#sudo make install
#sudo ldconfig

#return to main working dir after installing Csound
cd $SYSTEM_DEFAULTWORKINGDIRECTORY
ls

git submodue update --init --recursive --remote
git submodule foreach git pull origin master
git submodule status --recursive

mkdir build
cd build

cmake -DSKIP_FAST_MATH=True ..
make -j4
ls
mkdir $BUILD_ARTIFACTSTAGINGDIRECTORY/Linux
cp *.so $BUILD_ARTIFACTSTAGINGDIRECTORY/Linux
