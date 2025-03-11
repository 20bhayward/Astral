# Cell System Design

This document describes the design and implementation of the Cell system in the Astral Engine.

## Overview

The Cell system forms the foundation of the physics simulation in Astral. It uses a cellular automaton approach to model various materials and their interactions, similar to games like Noita. The system is designed to be efficient, flexible, and capable of simulating a wide variety of materials and behaviors.

## Key Components

### Cell

The `Cell` struct is the fundamental unit of simulation. Each cell represents a small area in the world and contains:

- **Material ID**: Reference to the material type in the Material Registry
- **Physical Properties**: Temperature, pressure, velocity, etc.
- **State Flags**: Bitflags for various states (burning, wet, frozen, etc.)
- **Metadata**: Additional material-specific information

Key features:
- Compact memory layout for cache efficiency
- State flag system to track multiple simultaneous states
- Helper methods for common operations

### CellProcessor

The `CellProcessor` handles material-specific cell interactions:

- **Material Initialization**: Applies appropriate properties to cells based on their material
- **Physics Interactions**: Determines movement, displacement, and swapping rules
- **Chemical Reactions**: Handles interactions between different materials
- **State Changes**: Manages transitions like freezing, melting, burning

### CellularPhysics

The `CellularPhysics` class implements the core simulation rules:

- **Material-Specific Rules**: Different update methods for solids, liquids, powders, gases, etc.
- **Multi-Phase Updates**: Different update patterns to avoid simulation artifacts
- **Special Effects**: Explosions, heat sources, force fields
- **Optimization**: Active cell tracking, chunk-based simulation

### CellularAutomaton

The `CellularAutomaton` serves as the high-level controller:

- **World Management**: Generation, loading, saving
- **Painting Tools**: Methods for modifying the world (drawing cells, lines, shapes)
- **Simulation Control**: Pause, resume, speed control
- **Material Registry**: Access to all defined materials
- **Statistics**: Tracking simulation performance and state

## Material System Integration

Cells interact closely with the Material system:

- Each cell references a material by ID
- Materials define physical properties and behaviors
- Material reactions specify how different materials interact
- State changes define how materials transform under different conditions

## Chunk System Integration

For efficient memory management and simulation, cells are organized into chunks:

- World is divided into fixed-size chunks (e.g., 64x64 cells)
- Only active chunks are loaded and simulated
- Chunk boundaries handle special cases to ensure seamless simulation
- Cross-chunk cell interactions are handled correctly

## Update Process

The cell simulation follows a multi-step update process:

1. **Reset Update Tracker**: Clear the bookkeeping for this frame
2. **Bottom-up Update**: Update materials that fall (powders, liquids)
3. **Top-down Update**: Update materials that rise (gases, fire)
4. **Solid Update**: Process static and solid materials
5. **Interaction Update**: Handle interactions between adjacent cells
6. **Finalization**: Reset cell update flags for the next frame

## Optimization Strategies

Several strategies are employed to optimize performance:

- **Chunk-Based Processing**: Only active chunks are simulated
- **Material-Specific Updates**: Only relevant physics are applied to each material type
- **Update Tracking**: Avoid processing cells multiple times per frame
- **Multi-Phase Updates**: Process different material types in optimal order
- **Memory Layout**: Cell data is organized for cache efficiency

## Future Improvements

Planned enhancements to the Cell system:

- **GPU Acceleration**: Moving cell processing to the GPU for massive parallelism
- **Advanced Fluid Dynamics**: More realistic liquid and gas behavior
- **Better Thermal Simulation**: More accurate heat transfer and temperature effects
- **Electrical Conductivity**: Simulating electrical flow through conductive materials
- **Pressure Wave Propagation**: More realistic explosions and compression effects
- **Structural Analysis**: Simulating structural integrity and collapse
- **Active Cell Tracking**: More efficient detection of which cells need updating

## Usage Examples

### Creating and Modifying Cells

```cpp
// Create a water cell
MaterialID waterId = automaton.getMaterialIDByName("Water");
automaton.setCell(x, y, waterId);

// Check cell temperature
float temp = automaton.getCell(x, y).temperature;

// Apply heat to a region
automaton.createHeatSource(x, y, 500.0f, 10.0f);

// Create an explosion
automaton.createExplosion(x, y, 15.0f, 10.0f);

// Paint a line of sand
MaterialID sandId = automaton.getMaterialIDByName("Sand");
automaton.paintLine(x1, y1, x2, y2, sandId);
```

### Defining Custom Materials

```cpp
// Create a new material
MaterialProperties lavaProps;
lavaProps.name = "Lava";
lavaProps.type = MaterialType::LIQUID;
lavaProps.color = {255, 100, 0, 200};
lavaProps.density = 3000.0f;
lavaProps.temperature = 1200.0f;
lavaProps.viscosity = 0.8f;
lavaProps.emissive = true;
lavaProps.emissiveStrength = 0.7f;

// Register the material
MaterialID lavaId = automaton.registerMaterial(lavaProps);
```

## Conclusion

The Cell system provides a flexible foundation for simulating complex material interactions in Astral. Its modular design allows for easy extension with new materials and behaviors, while the optimization strategies ensure good performance even with large numbers of active cells.