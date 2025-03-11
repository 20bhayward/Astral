# Getting Started with Astral Development

This guide outlines the initial steps to set up the Astral engine project and start development.

## Step 1: Set Up Basic Project Structure

Create the basic directory structure:

```bash
# Create core directories
mkdir -p src/{core,physics,rendering,tools,utils}
mkdir -p include/astral/{core,physics,rendering,tools,utils}
mkdir -p tests/{unit,integration}
mkdir -p examples
mkdir -p resources
mkdir -p scripts
mkdir -p thirdparty
```

## Step 2: Initialize Git Repository

```bash
# Initialize git repository
git init
git add .
git commit -m "Initial project structure"
```

Create a `.gitignore` file:

```
# Build directories
build/
bin/
lib/
out/

# IDE files
.vscode/
.idea/
*.swp
*.swo

# CMake generated files
CMakeCache.txt
CMakeFiles/
*.cmake
Makefile

# Compiled files
*.o
*.obj
*.exe
*.dll
*.so
*.dylib
*.a
*.lib

# Generated documentation
docs/generated/

# Dependency directories
thirdparty/build/

# OS specific files
.DS_Store
Thumbs.db
```

## Step 3: Set Up CMake Build System

Create the main `CMakeLists.txt`:

```cmake
cmake_minimum_required(VERSION 3.15)
project(Astral VERSION 0.1.0 LANGUAGES CXX)

# C++17 standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build options
option(ASTRAL_BUILD_TESTS "Build tests" ON)
option(ASTRAL_BUILD_EXAMPLES "Build examples" ON)
option(ASTRAL_ENABLE_PROFILING "Enable profiling" ON)

# Output directories
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Include directories
include_directories(include)

# Dependencies
find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)
find_package(glm REQUIRED)
find_package(spdlog REQUIRED)
find_package(nlohmann_json REQUIRED)

# Add subdirectories
add_subdirectory(src)

if(ASTRAL_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

if(ASTRAL_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()
```

Create `src/CMakeLists.txt`:

```cmake
# Core library
add_library(astral_core
    core/Engine.cpp
    core/Timer.cpp
    core/Config.cpp
    core/Logger.cpp
)

target_include_directories(astral_core PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(astral_core
    PUBLIC
    spdlog::spdlog
    nlohmann_json::nlohmann_json
)

# Physics library
add_library(astral_physics
    physics/Material.cpp
    physics/Cell.cpp
    physics/ChunkManager.cpp
    physics/CellularPhysics.cpp
)

target_include_directories(astral_physics PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(astral_physics
    PUBLIC
    astral_core
    glm::glm
)

# Rendering library
add_library(astral_rendering
    rendering/Renderer.cpp
    rendering/ShaderProgram.cpp
    rendering/Camera.cpp
    rendering/GridRenderer.cpp
)

target_include_directories(astral_rendering PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(astral_rendering
    PUBLIC
    astral_core
    astral_physics
    ${OPENGL_LIBRARIES}
    glfw
    glm::glm
)

# Main application
add_executable(astral_app
    main.cpp
)

target_link_libraries(astral_app
    PRIVATE
    astral_core
    astral_physics
    astral_rendering
)
```

## Step 4: Create Initial Core Files

### Engine Header (include/astral/core/Engine.h)

```cpp
#pragma once

#include <memory>
#include <string>

namespace astral {

class Logger;
class Config;
class Timer;
class PhysicsSystem;
class RenderingSystem;

class Engine {
public:
    Engine();
    ~Engine();

    bool initialize(const std::string& configFile = "config.json");
    void shutdown();

    void run();
    void stop();

    double getDeltaTime() const;
    double getTime() const;

private:
    bool running;
    std::unique_ptr<Logger> logger;
    std::unique_ptr<Config> config;
    std::unique_ptr<Timer> timer;
    std::unique_ptr<PhysicsSystem> physics;
    std::unique_ptr<RenderingSystem> renderer;

    double deltaTime;
    double time;

    void update();
    void render();
};

} // namespace astral
```

### Main Entry Point (src/main.cpp)

```cpp
#include "astral/core/Engine.h"
#include <iostream>

int main(int argc, char** argv) {
    try {
        astral::Engine engine;
        
        if (!engine.initialize()) {
            std::cerr << "Failed to initialize engine" << std::endl;
            return 1;
        }
        
        engine.run();
        engine.shutdown();
        
        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }
}
```

## Step 5: Create Initial Physics Files

### Material Header (include/astral/physics/Material.h)

```cpp
#pragma once

#include <string>
#include <vector>
#include <glm/glm.hpp>

namespace astral {

enum class MaterialType {
    EMPTY,
    SOLID,
    POWDER,
    LIQUID,
    GAS,
    FIRE,
    SPECIAL
};

struct MaterialProperties {
    MaterialType type;
    std::string name;
    
    // Visual properties
    glm::vec4 color;
    float colorVariation;
    bool emissive;
    float emissiveStrength;
    
    // Physical properties
    float density;
    float viscosity;
    float friction;
    float elasticity;
    float dispersion;
    
    // Simulation behavior
    bool movable;
    bool flammable;
    float flammability;
    
    // Constructors
    MaterialProperties();
    MaterialProperties(MaterialType type, const std::string& name, const glm::vec4& color);
};

} // namespace astral
```

### Cell Header (include/astral/physics/Cell.h)

```cpp
#pragma once

#include <cstdint>
#include <glm/glm.hpp>

namespace astral {

using MaterialID = uint16_t;

struct Cell {
    MaterialID material;
    float temperature;
    glm::vec2 velocity;
    uint8_t metadata;
    
    Cell();
    Cell(MaterialID material);
    
    bool operator==(const Cell& other) const;
    bool operator!=(const Cell& other) const;
};

} // namespace astral
```

## Step 6: Create Initial Rendering Files

### Renderer Header (include/astral/rendering/Renderer.h)

```cpp
#pragma once

#include <string>
#include <memory>
#include <glm/glm.hpp>

namespace astral {

class Camera;
class ShaderProgram;
class GridRenderer;
class ChunkManager;
class MaterialRegistry;

class Renderer {
public:
    Renderer();
    virtual ~Renderer();
    
    bool initialize(int width, int height, const std::string& title);
    void shutdown();
    
    void beginFrame();
    void endFrame();
    
    void setViewport(int x, int y, int width, int height);
    void setClearColor(float r, float g, float b, float a);
    
    void renderWorld(const ChunkManager* chunkManager, const MaterialRegistry* materials);
    
    bool shouldClose() const;
    
    int getWidth() const { return width; }
    int getHeight() const { return height; }
    
private:
    int width;
    int height;
    std::string title;
    
    std::unique_ptr<Camera> camera;
    std::unique_ptr<ShaderProgram> gridShader;
    std::unique_ptr<GridRenderer> gridRenderer;
    
    void* window; // GLFW window, void* to avoid including GLFW here
};

} // namespace astral
```

## Step 7: Create Build and Run Scripts

### Build Script (scripts/build.sh)

```bash
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
```

### Run Script (scripts/run.sh)

```bash
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
```

Make the scripts executable:

```bash
chmod +x scripts/build.sh scripts/run.sh
```

## Step 8: Create Basic Test Setup

### Test CMakeLists.txt (tests/CMakeLists.txt)

```cmake
# Google Test
find_package(GTest REQUIRED)

# Physics tests
add_executable(physics_tests
    unit/physics/MaterialTests.cpp
    unit/physics/CellTests.cpp
)

target_link_libraries(physics_tests
    PRIVATE
    astral_physics
    GTest::GTest
    GTest::Main
)

add_test(NAME physics_tests COMMAND physics_tests)

# Core tests
add_executable(core_tests
    unit/core/TimerTests.cpp
    unit/core/ConfigTests.cpp
)

target_link_libraries(core_tests
    PRIVATE
    astral_core
    GTest::GTest
    GTest::Main
)

add_test(NAME core_tests COMMAND core_tests)
```

### Basic Material Test (tests/unit/physics/MaterialTests.cpp)

```cpp
#include "astral/physics/Material.h"
#include <gtest/gtest.h>

namespace astral {
namespace test {

TEST(MaterialTest, DefaultConstructor) {
    MaterialProperties props;
    EXPECT_EQ(props.type, MaterialType::EMPTY);
    EXPECT_EQ(props.name, "");
    EXPECT_FALSE(props.movable);
}

TEST(MaterialTest, ParameterizedConstructor) {
    MaterialProperties props(MaterialType::SOLID, "Stone", glm::vec4(0.5f, 0.5f, 0.5f, 1.0f));
    EXPECT_EQ(props.type, MaterialType::SOLID);
    EXPECT_EQ(props.name, "Stone");
    EXPECT_EQ(props.color, glm::vec4(0.5f, 0.5f, 0.5f, 1.0f));
}

} // namespace test
} // namespace astral
```

## Step 9: First Development Milestone

Your first development milestone is to get a basic window displaying with OpenGL. Create a minimal implementation of the required classes to initialize the engine, create a window, and clear the screen with a color.

1. Implement the core Engine class
2. Implement a basic Renderer using GLFW and OpenGL
3. Create a simple main loop that clears the screen
4. Verify the window opens and closes correctly

After completing this milestone, update the Implementation Status document to track your progress.

## Step 10: Next Steps

Once the basic window is working, your next steps will be:

1. Implement the Material system
2. Create the Cell and Chunk data structures
3. Implement basic rendering of a grid
4. Add basic cellular automaton rules for a simple material (like sand)
5. Create a user interface for interacting with the simulation

Continue updating the Implementation Status document as you complete each step.