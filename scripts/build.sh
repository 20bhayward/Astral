#!/bin/bash

# Build configurations
BUILD_TYPE=${1:-Debug}
BUILD_DIR="build/${BUILD_TYPE}"

# Create build directory
mkdir -p ${BUILD_DIR}
cd ${BUILD_DIR}

# Configure
cmake -DCMAKE_BUILD_TYPE=${BUILD_TYPE} ../..

# Build
cmake --build . -- -j$(nproc)

# Return to root
cd ../..

echo "Build completed: ${BUILD_TYPE}"