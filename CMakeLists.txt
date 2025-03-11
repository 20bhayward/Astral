cmake_minimum_required(VERSION 3.15)
project(Astral VERSION 0.1.0 LANGUAGES CXX)

# C++17 standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Include custom CMake modules
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(Doxygen)

# Build options
option(ASTRAL_BUILD_TESTS "Build tests" ON)
option(ASTRAL_BUILD_EXAMPLES "Build examples" ON)
option(ASTRAL_ENABLE_PROFILING "Enable profiling" ON)
option(ASTRAL_GENERATE_DOCS "Generate documentation" ON)

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
find_package(imgui QUIET) # Optional ImGui for debug UI

# Add subdirectories
add_subdirectory(src)

if(ASTRAL_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

if(ASTRAL_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# Add package configuration
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/AstralConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

# Installation targets
install(DIRECTORY include/ DESTINATION include)
install(EXPORT AstralTargets
    FILE AstralTargets.cmake
    NAMESPACE Astral::
    DESTINATION lib/cmake/Astral
)

# Create package config file
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/AstralConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/AstralConfig.cmake"
    @ONLY
)

# Install config files
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/AstralConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/AstralConfigVersion.cmake"
    DESTINATION lib/cmake/Astral
)