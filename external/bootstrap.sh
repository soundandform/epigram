#!/bin/bash

BOOST=boost_1_65_1.tar.gz
curl -L -O https://dl.bintray.com/boostorg/release/1.65.1/source/$BOOST
tar -zxvf $BOOST
mv $BOOST boost
rm $BOOST
echo "/*" > boost/.gitignore

mkdir ../lib 
echo "/*" > ../lib/.gitignore


cd boost
./bootstrap.sh
./b2 toolset=clang cxxflags="-stdlib=libc++" linkflags="-stdlib=libc++" variant=release --layout=versioned

cp stage/lib/libboost_context-clang-darwin42-mt-1_65_1.a ../../lib

cd ..

cd cityhash
./configure
make






