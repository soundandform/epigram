#!/bin/bash

VERSION=$1

if [ $# == 0 ]; then
	VERSION=74
fi

echo "building boost version: 1.$VERSION"
echo "----------------------------------------------"

platform=`sh script/platform.sh`
mkdir lib


# Boost ------------------------------------------------------------------------------------------------------------------

cd external

BOOST_VERSION=1.$VERSION.0
BOOST=boost_1_"$VERSION"_0
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
	./b2 toolset=clang cflags=-mmacosx-version-min=10.8 mflags=-mmacosx-version-min=10.8 mmflags=-mmacosx-version-min=10.8 cxxflags="-stdlib=libc++ -mmacosx-version-min=10.8" linkflags="-stdlib=libc++ -mmacosx-version-min=10.8" variant=release
else
	./b2 
fi

mv stage/lib/libboost_*.a ../../lib

# back to external
cd .. 

# put it side-by-side
mv boost ../..

# CityHash ----------------------------------------------------------------------------------------------------------------

cd cityhash
./configure
make

