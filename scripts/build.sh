#!/usr/bin/env sh

BUILDTYPE=${1:-dev}
shift;
COMPILER=${1:-notgiven}

case $COMPILER in
    gnu)
        CC=gcc
        CXX=g++
        ;;
    clang)
        CC=clang
        CXX=clang++
        ;;
    notgiven)
        CC=${CC:-clang}
        CXX=${CXX:-clang++}
        ;;
    *)
        echo "Unrecognized compiler $COMPILER"
        exit 1
        ;;
esac

ARGS=""

#cmake_ver=`cmake --version | grep version | awk '{ print $3; }'`

case $BUILDTYPE in
    dev)
        DIR=build
        BTARG="-DCMAKE_BUILD_TYPE=Developer"
        ;;
    release)
        DIR=build-release
        BTARG="-DCMAKE_BUILD_TYPE=Release"
        ;;
    ci)
        DIR=build-ci
        BTARG="-DCMAKE_BUILD_TYPE=CI"
        #ARGS="--verbose"
        ;;
    *)
        echo "Unsupported build type (dev or release)"
        exit 1
        ;;
esac


mkdir $DIR || exit 1
CC=$CC CXX=$CXX cmake -S . -B $DIR $BTARG || exit 1

(cd $DIR ; make -j2 ) && ( cd $DIR; ctest $ARGS )
#cmake --build $DIR $ARGS && (cd $DIR; ctest $ARGS )
