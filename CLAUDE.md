# Astral Project Guide for Claude

This document helps Claude track project information, preferences, and commands for the Astral physics engine project.

## Project Overview

Astral is a 2D physics-based voxel simulation engine inspired by Noita, focusing on realistic particle physics, fluid dynamics, and material interactions. The goal is to create a high-performance engine capable of simulating thousands to millions of particles.

## Build Commands

```bash
# Configure and build project (Debug)
mkdir -p build/debug
cd build/debug
cmake -DCMAKE_BUILD_TYPE=Debug ../..
cmake --build .

# Configure and build project (Release)
mkdir -p build/release
cd build/release
cmake -DCMAKE_BUILD_TYPE=Release ../..
cmake --build .

# Run tests
cd build/debug
ctest

# Run main application
cd build/debug
./astral_app

# Generate documentation
cd docs
doxygen Doxyfile
```

## Coding Style Guidelines

- Use camelCase for variables and functions
- Use PascalCase for classes and structs
- Use ALL_CAPS for constants and macros
- Indent with 4 spaces (no tabs)
- Place braces on their own lines
- Maximum line length of 100 characters
- Document all public APIs with doxygen-style comments

```cpp
/**
 * Example function demonstrating style guidelines.
 * 
 * @param inputValue The value to process
 * @return The processed result
 */
int processData(int inputValue)
{
    constexpr int MAX_VALUE = 100;
    
    if (inputValue > MAX_VALUE)
    {
        return MAX_VALUE;
    }
    
    return inputValue * 2;
}
```

## Project Structure

- `src/` - Source code
  - `core/` - Core engine components
  - `physics/` - Physics simulation
  - `rendering/` - Rendering system
  - `tools/` - Development tools
- `include/` - Public headers
- `tests/` - Unit and integration tests
- `examples/` - Example applications
- `docs/` - Documentation
- `scripts/` - Build and utility scripts
- `thirdparty/` - Third-party dependencies
- `resources/` - Game resources

## Development Workflow

1. Check current status in `docs/IMPLEMENTATION_STATUS.md`
2. Work on next pending task
3. Update status file when task is completed
4. Run tests to verify functionality
5. Document code and update documentation as needed

## Implementation Progress Tracking

The file `docs/IMPLEMENTATION_STATUS.md` tracks the current implementation status. After completing tasks:

1. Update the "Completed Tasks" section
2. Move items from "In Progress" to "Completed"
3. Move items from "Up Next" to "In Progress"
4. Update overall phase completion percentages
5. Add an entry to the "Recent Updates" section with date and summary

## Dependencies

- GLFW: Window creation and input handling
- glad: OpenGL loader
- glm: Math library
- stb_image: Image loading
- spdlog: Logging
- EnTT: Entity Component System
- nlohmann/json: Configuration and serialization
- Google Test: Testing framework
- ImGui: Debug UI

## Key Design Principles

1. **Data-oriented design**: Optimize for cache efficiency
2. **Component-based architecture**: Flexible entity composition
3. **Modular systems**: Decoupled subsystems with clear interfaces
4. **Performance-first**: Optimize core simulation loops
5. **Debug-friendly**: Comprehensive tools for debugging and visualization

## Notes for Implementation

- Use thread pool for parallelizing physics updates
- Implement chunk-based world management for efficient memory use
- Use cellular automaton approach for material simulation
- Leverage SIMD for physics calculations where possible
- Consider GPU acceleration for particle systems
- Use instanced rendering for efficient visualization
- Implement active cell tracking to avoid updating static regions

## To-Do List

- Set up basic project structure and build system
- Implement core engine architecture
- Create material system and physics simulation
- Develop rendering system for visualization
- Add development and debugging tools
- Optimize performance for large simulations
- Create demo applications showcasing capabilities