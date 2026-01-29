#!/bin/bash

set -e

cd "$(dirname "$0")"

if [ ! -d build ]; then
    mkdir build
fi

cd build

echo "Configuring project..."
cmake .. -DCMAKE_BUILD_TYPE=Release

echo "Building HexViewer..."
cmake --build . --config Release

echo "Build complete."

if [ -f HexViewer ]; then
    echo "Executable: build/HexViewer"
elif [ -f Release/HexViewer ]; then
    echo "Executable: build/Release/HexViewer"
fi
