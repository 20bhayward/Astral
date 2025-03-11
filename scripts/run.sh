#!/bin/bash

# Build configuration
BUILD_TYPE=${1:-Debug}
APP_NAME="astral_app"

# Check if build exists
if [ ! -f "build/${BUILD_TYPE}/bin/${APP_NAME}" ]; then
    echo "Application not built. Building now..."
    ./scripts/build.sh ${BUILD_TYPE}
fi

# Run the application
./build/${BUILD_TYPE}/bin/${APP_NAME}