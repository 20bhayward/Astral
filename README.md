# Astral

A Noita-inspired falling sand physics engine with cellular automaton simulation.

## Overview

Astral is a high-performance 2D physics engine focused on simulating materials like sand, water, gas, and other particles using a cellular automaton approach. Inspired by games like Noita, it provides realistic physics for thousands to millions of particles interacting in real-time.

![Astral Engine Demo](docs/images/demo_placeholder.png)

## Features

- **Cellular Physics**: Simulates a wide range of materials including solids, powders, liquids, and gases
- **Material Interactions**: Dynamic interactions between different material types
- **Temperature System**: Heat transfer and state changes based on temperature
- **Optimized Performance**: Chunk-based simulation with multithreading support
- **Realistic Rendering**: Dynamic lighting and particle effects
- **Extensible**: Easy to add new materials and behaviors

## Project Status

Astral is currently in early development. See [Implementation Status](docs/IMPLEMENTATION_STATUS.md) for details on current progress.

## Building

### Prerequisites

- C++17 compatible compiler
- CMake 3.15+
- OpenGL 4.3+

### Dependencies

- GLFW
- glad
- glm
- spdlog
- EnTT
- nlohmann/json
- ImGui

### Build Instructions

```bash
# Clone the repository
git clone https://github.com/yourusername/astral.git
cd astral

# Configure
mkdir build && cd build
cmake ..

# Build
cmake --build .

# Run
./astral_app
```

## Getting Started

Check out the [documentation](docs/) for detailed information on the engine architecture and usage.

Example applications can be found in the `examples/` directory:

- `basic_simulation`: Simple demonstration of material interaction
- `liquid_physics`: Focus on fluid dynamics
- `temperature_demo`: Showcase of the temperature system

## Contributing

Contributions are welcome! Please check the [contribution guidelines](CONTRIBUTING.md) before submitting pull requests.

## License

Astral is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Acknowledgments

- Inspired by [Noita](https://noitagame.com/) by Nolla Games
- Techniques from [Falling Sand](https://en.wikipedia.org/wiki/Falling-sand_game) games
- Research on cellular automaton for physical simulations