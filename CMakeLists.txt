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