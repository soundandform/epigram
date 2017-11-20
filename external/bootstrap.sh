#!/bin/bash

BOOST=boost_1_65_1
BOOST_TAR=$BOOST.tar.gz

curl -L -O https://dl.bintray.com/boostorg/release/1.65.1/source/$BOOST_TAR
tar -zxf $BOOST_TAR
rm $BOOST_TAR
mv $BOOST boost
echo "/*" > boost/.gitignore

mkdir ../lib 
echo "/*" > ../lib/.gitignore

cd boost
./bootstrap.sh
./b2 toolset=clang cxxflags="-stdlib=libc++" linkflags="-stdlib=libc++" variant=release --layout=versioned

cp stage/lib/libboost_context-clang-darwin42-mt-1_65_1.a ../../lib
cp stage/lib/libboost_system-clang-darwin42-mt-1_65_1.a ../../lib
cp stage/lib/libboost_thread-clang-darwin42-mt-1_65_1.a ../../lib
cp stage/lib/libboost_timer-clang-darwin42-mt-1_65_1.a ../../lib
cp stage/lib/libboost_chrono-clang-darwin42-mt-1_65_1.a ../../lib

cd ..

cd cityhash
./configure
make






