#!/bin/bash

echo "=== HexViewer Build Script ==="

if [ "$1" == "clean" ]; then
    rm -rf build
    echo "Clean complete."
    exit 0
fi

check_and_install() {
    local pkg_name="$1"
    local check_cmd="$2"
    local apt_pkg="$3"

    if { [ -n "$check_cmd" ] && ! command -v "$check_cmd" >/dev/null 2>&1; } || \
       { [ -n "$apt_pkg" ] && ! dpkg -s "$apt_pkg" >/dev/null 2>&1; }; then
        echo "-> Missing dependency: $pkg_name. Attempting to install..."
        if sudo apt update && sudo apt install -y "$apt_pkg"; then
            echo "-> Successfully installed $pkg_name."
        else
            echo "ERROR: Failed to install $pkg_name. Please install it manually."
            exit 1
        fi
    fi
}

check_and_install "CMake" "cmake" "cmake"
check_and_install "Ninja" "ninja" "ninja-build"
check_and_install "C++ Compiler" "g++" "build-essential"
check_and_install "X11 Development Library" "" "libx11-dev"
check_and_install "Libcurl Development Library" "" "libcurl4-openssl-dev"

mkdir -p build
cd build

echo "Configuring project with CMake..."
cmake -G "Ninja" -DCMAKE_BUILD_TYPE=Release .. || exit 1

echo "Building project..."
ninja -j"$(nproc)" || exit 1

echo "----------------------------------------"
echo "Build completed successfully!"
echo "Output: build/HexViewer"
