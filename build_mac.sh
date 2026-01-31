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
    EXE_PATH="HexViewer"
elif [ -f Release/HexViewer ]; then
    EXE_PATH="Release/HexViewer"
else
    echo "Error: HexViewer executable not found."
    exit 1
fi

echo "Executable found at: $EXE_PATH"

APP_NAME="HexViewer"
APP_DIR="${APP_NAME}.app"
CONTENTS="${APP_DIR}/Contents"
MACOS="${CONTENTS}/MacOS"
RESOURCES="${CONTENTS}/Resources"

echo "Creating .app bundle..."

rm -rf "$APP_DIR"

mkdir -p "$MACOS"
mkdir -p "$RESOURCES"

cp "$EXE_PATH" "$MACOS/$APP_NAME"
chmod +x "$MACOS/$APP_NAME"

if [ -f "../resources/icons/main.icns" ]; then
    echo "Adding app icon..."
    cp ../resources/icons/main.icns "$RESOURCES/main.icns"
else
    echo "Warning: main.icns not found, app will have no icon."
fi

cat > "${CONTENTS}/Info.plist" <<EOF
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN"
 "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>CFBundleName</key>
    <string>${APP_NAME}</string>

    <key>CFBundleExecutable</key>
    <string>${APP_NAME}</string>

    <key>CFBundleIdentifier</key>
    <string>com.devx.${APP_NAME}</string>

    <key>CFBundleVersion</key>
    <string>1.0</string>

    <key>CFBundlePackageType</key>
    <string>APPL</string>

    <key>LSMinimumSystemVersion</key>
    <string>11.0</string>

    <key>CFBundleIconFile</key>
    <string>main.icns</string>
</dict>
</plist>
EOF

echo "Info.plist created."

# Optional: ad-hoc sign so macOS doesn't complain
echo "Signing app..."
codesign --force --deep --sign - "${APP_DIR}"

echo "Done!"
echo "You can now run:"
echo "  open ${APP_DIR}"