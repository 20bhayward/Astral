#!/bin/bash

# Script to set up vcpkg and install required dependencies for Astral

# Configuration
VCPKG_DIR="thirdparty/vcpkg"
VCPKG_TRIPLET="x64-linux"  # Change as needed: x64-windows, x64-osx

# Set up vcpkg if it doesn't exist
if [ ! -d "$VCPKG_DIR" ]; then
    echo "Setting up vcpkg..."
    
    # Create thirdparty directory if it doesn't exist
    mkdir -p thirdparty
    
    # Clone vcpkg repository
    git clone https://github.com/microsoft/vcpkg.git "$VCPKG_DIR"
    
    # Build vcpkg
    pushd "$VCPKG_DIR"
    ./bootstrap-vcpkg.sh
    popd
else
    echo "vcpkg already exists, updating..."
    pushd "$VCPKG_DIR"
    git pull
    popd
fi

# Install dependencies
echo "Installing dependencies..."
"$VCPKG_DIR/vcpkg" install --triplet="$VCPKG_TRIPLET" \
    glfw3 \
    glm \
    spdlog \
    nlohmann-json \
    gtest \
    imgui

echo "Creating vcpkg CMake toolchain file..."
# Create a wrapper CMake toolchain file
cat > "vcpkg-toolchain.cmake" << EOF
# Astral vcpkg toolchain wrapper
set(CMAKE_TOOLCHAIN_FILE "\${CMAKE_CURRENT_LIST_DIR}/$VCPKG_DIR/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain file")
EOF

echo "Setup complete! To build with vcpkg, use:"
echo "  mkdir build && cd build"
echo "  cmake -DCMAKE_TOOLCHAIN_FILE=../vcpkg-toolchain.cmake .."
echo "  cmake --build ."