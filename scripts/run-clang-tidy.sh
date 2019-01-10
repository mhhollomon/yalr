#!/usr/bin/bash

BIN=$1 && shift
BUILD_ROOT=$1 && shift

# Execute in a different directory to ensure we don't mess with the meson config
TIDY_DIR="${BUILD_ROOT}/:clang-tidy"

mkdir -p ${TIDY_DIR}
cp  ${BUILD_ROOT}/compile_commands.json ${TIDY_DIR}

# Replace meson commands clang does not understand
sed -i 's/-pipe//g' ${TIDY_DIR}/compile_commands.json

echo args = -p ${TIDY_DIR} $*

$BIN -p ${TIDY_DIR} $*
