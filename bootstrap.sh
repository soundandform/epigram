#!/bin/bash

VERSION=$1

if [ $# == 0 ]; then
	VERSION=74
fi

BOOST_ARCH=combined

if [ $VERSION -ge 86 ]; then
	BOOST_ARCH="arm+x86"
fi

platform=`sh script/platform.sh`

echo "----------------------------------------------"
echo "building boost version: 1.$VERSION for $platform"
echo "----------------------------------------------"

mkdir lib


# Boost ------------------------------------------------------------------------------------------------------------------

cd external

BOOST_VERSION=1.$VERSION.0
BOOST=boost_1_"$VERSION"_0
BOOST_TAR=$BOOST.tar.gz

curl -L -O https://boostorg.jfrog.io/artifactory/main/release/$BOOST_VERSION/source/$BOOST_TAR

tar -zxf $BOOST_TAR
rm $BOOST_TAR
mv $BOOST boost
echo "/*" > boost/.gitignore

echo "/*" > ../lib/.gitignore

cd boost
./bootstrap.sh --with-libraries=chrono,timer,filesystem,system,date_time,program_options,context

if [ $platform == "macOS" ]; then
	./b2 toolset=clang cflags=-mmacosx-version-min=10.8 mflags=-mmacosx-version-min=10.8 mmflags=-mmacosx-version-min=10.8 cxxflags="-stdlib=libc++ -mmacosx-version-min=10.8" linkflags="-stdlib=libc++ -mmacosx-version-min=10.8" variant=release --layout=versioned
	./b2 architecture=$BOOST_ARCH toolset=clang cflags=-mmacosx-version-min=10.8 mflags=-mmacosx-version-min=10.8 mmflags=-mmacosx-version-min=10.8 cxxflags="-arch x86_64 -arch arm64 -stdlib=libc++ -mmacosx-version-min=10.8" linkflags="-stdlib=libc++ -mmacosx-version-min=10.8" variant=release
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
env CXXFLAGS="-arch x86_64 -arch arm64" ./configure
make

