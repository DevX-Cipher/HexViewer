#!/bin/bash

echo "=== HexViewer Build & Package Script ==="

if [ "$1" == "clean" ]; then
    rm -rf build
    echo "Clean complete."
    exit 0
fi

# Ensure release_version.txt exists
if [ ! -f "release_version.txt" ]; then
    echo "-> release_version.txt not found! Creating default with version 1.0.0"
    echo "1.0.0" > release_version.txt
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
check_and_install "dpkg-dev" "dpkg-shlibdeps" "dpkg-dev"

if ! grep -q "CPACK_GENERATOR" CMakeLists.txt; then
    echo "-> Adding Debian package configuration to CMakeLists.txt..."
    cat << 'EOF' >> CMakeLists.txt

# --- CPack configuration for Debian ---
if (UNIX AND NOT APPLE)
    install(TARGETS HexViewer DESTINATION bin)
    
    # Read version from release_version.txt
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/release_version.txt")
        file(READ "${CMAKE_CURRENT_SOURCE_DIR}/release_version.txt" HEX_VERSION)
        string(STRIP "${HEX_VERSION}" HEX_VERSION)
    else()
        set(HEX_VERSION "1.0.0")
    endif()

    set(CPACK_GENERATOR "DEB")
    set(CPACK_PACKAGE_NAME "hexviewer")
    set(CPACK_PACKAGE_VERSION "${HEX_VERSION}")
    set(CPACK_PACKAGE_CONTACT "Hors (horsicq)")
    set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "A fast and modern hex viewer")
    set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "amd64")
    set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
    include(CPack)
endif()
EOF
fi

mkdir -p build
cd build

echo "Configuring project with CMake..."
cmake -G "Ninja" -DCMAKE_BUILD_TYPE=Release .. || exit 1

echo "Building project..."
ninja -j"$(nproc)" || exit 1

echo "Packaging project into .deb..."
cpack -G DEB || exit 1

echo "----------------------------------------"
echo "Build and Packaging completed successfully!"
echo "Output Binary: build/HexViewer"
echo "Output Package: build/$(ls *.deb 2>/dev/null | head -n 1)"