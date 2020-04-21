#!/bin/bash

platform=`sh script/platform.sh`
mkdir lib


# Boost ------------------------------------------------------------------------------------------------------------------

cd external

BOOST_VERSION=1.67.0
BOOST=boost_1_67_0
BOOST_TAR=$BOOST.tar.gz

curl -L -O https://dl.bintray.com/boostorg/release/$BOOST_VERSION/source/$BOOST_TAR
tar -zxf $BOOST_TAR
rm $BOOST_TAR
mv $BOOST boost
echo "/*" > boost/.gitignore

echo "/*" > ../lib/.gitignore

cd boost
./bootstrap.sh

if [ $platform == "macOS" ]; then
	./b2 toolset=clang cflags=-mmacosx-version-min=10.8 mflags=-mmacosx-version-min=10.8 mmflags=-mmacosx-version-min=10.8 cxxflags="-stdlib=libc++ -mmacosx-version-min=10.8" linkflags="-stdlib=libc++ -mmacosx-version-min=10.8" variant=release --layout=versioned
else
	./b2 
fi

mv stage/lib/libboost_*.a ../../lib

cd ..

# put it side-by-side
mv boost ..

# CityHash ----------------------------------------------------------------------------------------------------------------

cd ../cityhash
./configure
make

