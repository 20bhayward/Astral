cmake_minimum_required(VERSION 3.15)
project(Astral VERSION 0.1.0 LANGUAGES CXX)

# C++17 standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

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
find_package(OpenGL REQUIRED)

# Try to find dependencies with fallbacks
# GLFW - Use our custom find module
find_package(GLFW3 QUIET)
if(NOT GLFW3_FOUND)
  message(WARNING "GLFW3 not found - rendering components will not build")
endif()

# GLM
find_package(glm QUIET)
if(NOT glm_FOUND)
  message(STATUS "GLM not found via find_package, checking for header-only include")
  # For header-only GLM, we only need to check if the headers exist
  find_path(GLM_INCLUDE_DIR glm/glm.hpp)
  if(GLM_INCLUDE_DIR)
    message(STATUS "Found GLM headers at ${GLM_INCLUDE_DIR}")
    # Create an interface target if not created by find_package
    if(NOT TARGET glm::glm)
      add_library(glm::glm INTERFACE)
      target_include_directories(glm::glm INTERFACE ${GLM_INCLUDE_DIR})
    endif()
  else()
    message(WARNING "GLM not found - some components may not build")
  endif()
endif()

# spdlog
find_package(spdlog QUIET)
if(NOT spdlog_FOUND)
  message(WARNING "spdlog not found - logging functionality will be limited")
endif()

# nlohmann_json
find_package(nlohmann_json QUIET)
if(NOT nlohmann_json_FOUND)
  message(WARNING "nlohmann_json not found - configuration system will be limited")
endif()

# ImGui (optional)
find_package(imgui QUIET) 
if(NOT imgui_FOUND)
  message(STATUS "ImGui not found - debug UI will be disabled")
endif()

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