#!/bin/bash

platform=`sh script/platform.sh`
mkdir lib


# Boost ------------------------------------------------------------------------------------------------------------------

cd external

BOOST=boost_1_65_1
BOOST_TAR=$BOOST.tar.gz

curl -L -O https://dl.bintray.com/boostorg/release/1.65.1/source/$BOOST_TAR
tar -zxf $BOOST_TAR
rm $BOOST_TAR
mv $BOOST boost
echo "/*" > boost/.gitignore

echo "/*" > ../lib/.gitignore

cd boost
./bootstrap.sh

if [ $platform == "macOS" ]; then
	./b2 toolset=clang cxxflags="-stdlib=libc++" linkflags="-stdlib=libc++" variant=release --layout=versioned
else
	./b2 
fi

cp stage/lib/libboost_context*.a ../../lib
cp stage/lib/libboost_system*.a ../../lib
cp stage/lib/libboost_thread*.a ../../lib
cp stage/lib/libboost_timer*.a ../../lib
cp stage/lib/libboost_chrono*.a ../../lib
cp stage/lib/libboost_program_options*.a  ../../lib


# CityHash ----------------------------------------------------------------------------------------------------------------

cd ../cityhash
./configure
make

