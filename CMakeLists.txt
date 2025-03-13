cmake_minimum_required(VERSION 3.15)
project(Astral VERSION 0.1.0 LANGUAGES C CXX)

# C++17 standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# C11 standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# Include custom CMake modules
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
# Check if Doxygen module exists
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cmake/Doxygen.cmake")
  include(Doxygen)
else()
  message(STATUS "Doxygen.cmake not found - documentation generation disabled")
endif()

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
find_package(OpenGL)
find_package(glfw3)
find_package(glm REQUIRED)
find_package(spdlog REQUIRED)
find_package(nlohmann_json REQUIRED)

# Optional graphics functionality
if(OpenGL_FOUND AND glfw3_FOUND)
    message(STATUS "OpenGL and GLFW found - graphics functionality will be enabled")
    add_compile_definitions(USE_OPENGL)
else()
    message(STATUS "OpenGL or GLFW not found - graphics functionality will be disabled")
endif()

# GLAD - just include it directly
include_directories(${CMAKE_SOURCE_DIR}/include)

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