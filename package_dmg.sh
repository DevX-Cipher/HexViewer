#!/bin/bash

echo "=== HexViewer macOS Build & DMG Script ==="

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

# Dependency check for macOS (using Homebrew)
check_and_install_mac() {
    local check_cmd="$1"
    local brew_pkg="$2"

    if ! command -v "$check_cmd" >/dev/null 2>&1; then
        echo "-> Missing dependency: $brew_pkg. Attempting to install via Homebrew..."
        if command -v brew >/dev/null 2>&1; then
            brew install "$brew_pkg"
        else
            echo "ERROR: Homebrew is not installed. Please install Homebrew first (https://brew.sh) or install $brew_pkg manually."
            exit 1
        fi
    fi
}

check_and_install_mac "cmake" "cmake"
check_and_install_mac "ninja" "ninja"

# CPack configuration for macOS (DragNDrop)
if ! grep -q "CPACK_GENERATOR \"DragNDrop\"" CMakeLists.txt; then
    echo "-> Adding macOS DMG package configuration to CMakeLists.txt..."
    cat << 'EOF' >> CMakeLists.txt

# --- CPack configuration for macOS (DMG) ---
if (APPLE)
    install(TARGETS HexViewer DESTINATION bin)
    
    # Read version from release_version.txt
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/release_version.txt")
        file(READ "${CMAKE_CURRENT_SOURCE_DIR}/release_version.txt" HEX_VERSION)
        string(STRIP "${HEX_VERSION}" HEX_VERSION)
    else()
        set(HEX_VERSION "1.0.0")
    endif()

    set(CPACK_GENERATOR "DragNDrop")
    set(CPACK_PACKAGE_NAME "HexViewer")
    set(CPACK_PACKAGE_VERSION "${HEX_VERSION}")
    set(CPACK_DMG_FORMAT "UDBZ") # Use compressed DMG format
    set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${HEX_VERSION}-macOS")
    include(CPack)
endif()
EOF
fi

mkdir -p build
cd build

echo "Configuring project with CMake..."
cmake -G "Ninja" -DCMAKE_BUILD_TYPE=Release .. || exit 1

echo "Building project..."
# macOS uses sysctl to get the number of CPU cores
ninja -j"$(sysctl -n hw.ncpu)" || exit 1

echo "Packaging project into .dmg..."
cpack -G DragNDrop || exit 1

echo "----------------------------------------"
echo "Build and Packaging completed successfully!"
echo "Output Binary: build/HexViewer"
echo "Output Package: build/$(ls *.dmg 2>/dev/null | head -n 1)"