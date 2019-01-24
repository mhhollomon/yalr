#!/usr/bin/env sh

BUILDTYPE=${1:-dev}
CC=${CC:-clang}
CXX=${CXX:-clang++}

case $BUILDTYPE in
    dev)
        DIR=build
        BTARG="--buildtype debug"
        ;;
    release)
        DIR=build-release
        BTARG="--buildtype release"
        ;;
    *)
        echo "Unsupported build type (dev or release)"
        exit 1
        ;;
esac


mkdir $DIR || exit 1
CC=$CC CXX=$CXX meson $BTARG $DIR . || exit 1

cd $DIR
ninja -v && \
    ninja test
