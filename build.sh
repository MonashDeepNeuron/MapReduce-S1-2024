#!/bin/bash

# ENV
CMAKE_EXPORT_COMPILE_COMMANDS=1

cmake -S standalone -B build/standalone
cmake --build build/standalone
./build/standalone/MDN-MapReduce --help
cp ./build/standalone/compile_commands.json ./
