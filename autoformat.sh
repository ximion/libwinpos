#!/bin/bash

cd "$(dirname "$0")"
set -e

source_files=`find src/ examples/ -type f \( -name '*.c' -o -name '*.h' -o -name '*.cpp' -o -name '*.hpp' \)`

clang-format -i $source_files
