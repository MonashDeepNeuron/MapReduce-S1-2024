#!/bin/bash

# ENV
CMAKE_EXPORT_COMPILE_COMMANDS=1

cmake -S . -B ./build
cmake --build build
cp ./build/standalone/compile_commands.json ./
