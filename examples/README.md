# Astral Examples

This directory contains examples showing how to use the Astral engine.

## Physics Sandbox Example

`physics_sandbox_example.cpp` - A simple sandbox for testing the cellular automaton physics system.

### Controls

- **Left Mouse Button**: Draw with the selected material
- **Right Mouse Button**: Erase (replace with air)
- **Number Keys (1-9)**: Select different materials
  - 1: Sand
  - 2: Water
  - 3: Stone
  - 4: Wood
  - 5: Oil
  - 6: Lava
  - 7: Fire
  - 8: Steam
  - 9: Smoke
- **Mouse Wheel**: Change brush size
- **Shift + Mouse Wheel**: Zoom in/out
- **WASD Keys**: Pan the camera
- **Space**: Pause/resume the simulation
- **R**: Reset the world (create new sandbox)
- **E**: Create an explosion at the cursor position
- **H**: Create a heat source at the cursor position
- **Escape**: Quit the application

### Features Demonstrated

- Basic cellular automaton physics system
- Material interactions (water, sand, fire, etc.)
- Temperature effects
- Special effects (explosions, heat sources)
- Simple rendering of cells

### Building and Running

```bash
# From the build directory
cmake --build . --target physics_sandbox_example
./bin/physics_sandbox_example
```