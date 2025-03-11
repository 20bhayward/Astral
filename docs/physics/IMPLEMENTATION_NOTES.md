# Implementation Notes: Physics System

This document contains technical notes and implementation details for the Astral physics system.

## Cell System Implementation

### Memory Optimization

The `Cell` struct is designed to be compact while still providing all necessary functionality:
- Basic properties use smaller data types where possible (`uint8_t`, `uint16_t`)
- State flags are stored as bitflags to save space (8 states in a single byte)
- Memory layout is designed to minimize padding

The estimated memory usage per cell is approximately 32 bytes, enabling efficient simulation of large worlds.

### State Flags System

The state flags system uses a bitfield approach to track multiple simultaneous states:
```cpp
static constexpr uint8_t FLAG_BURNING = 0x01;     // 00000001
static constexpr uint8_t FLAG_WET = 0x02;         // 00000010
static constexpr uint8_t FLAG_ELECTRIFIED = 0x04; // 00000100
```

This allows for efficient operations:
```cpp
// Check if a cell is burning
bool isBurning = cell.hasFlag(Cell::FLAG_BURNING);

// Make a cell wet
cell.setFlag(Cell::FLAG_WET);

// Remove the burning flag
cell.clearFlag(Cell::FLAG_BURNING);
```

### Multi-Phase Updates

To avoid simulation artifacts and ensure consistent behavior, the update process is split into multiple phases:

1. **Bottom-up for falling materials**: Update powders and liquids from bottom to top
2. **Top-down for rising materials**: Update gases and fire from top to bottom
3. **General phase for solids**: Update solids and special materials
4. **Interaction phase**: Process interactions between cells

This approach prevents simulation bias and creates more realistic behavior.

## Chunk System Implementation

### Chunk Size Considerations

The chunk size (currently 64x64) was chosen as a balance between:
- Memory efficiency (fitting in CPU cache lines)
- Minimizing cross-chunk operations
- Enabling efficient active region tracking

Experiments with various chunk sizes showed:
- 16x16: Too many cross-chunk operations, high overhead
- 32x32: Good balance but still high overhead
- 64x64: Good performance with reasonable overhead
- 128x128: Too large to fit efficiently in cache, less granular active tracking

### Thread Safety

The chunk system is designed for concurrent access:
- Each chunk has its own mutex for thread safety
- The chunk manager uses a combination of read-write locks
- Cell access follows a consistent locking pattern to prevent deadlocks
- Cross-chunk operations use careful lock ordering to prevent deadlocks

## Material System Implementation

### Material Type Dispatch

The physics system uses a dispatch table to efficiently handle different material types:
```cpp
std::unordered_map<MaterialType, CellUpdateFunction> updateFunctions;
```

This approach offers:
- O(1) dispatch to the correct update function
- Easy extensibility for new material types
- Cleaner code than large switch statements

### State Changes and Reactions

Material state changes and reactions are stored as rules in the material properties:
```cpp
struct StateChangeRule {
    float temperatureThreshold;
    uint16_t targetMaterial;
    float probability;
    std::string condition;
};
```

This data-driven approach allows for:
- Easy definition of complex behaviors
- Runtime modification of reactions
- Saving/loading reaction rules from configuration files

## CellularAutomaton Implementation

### World Template Generation

World templates use a combination of:
- Primitive shape drawing (rectangles, circles)
- Procedural generation
- Material placement rules

The current implementation uses simple algorithms but can be extended with:
- Terrain generation algorithms
- Cave system generation
- Biome-based material distribution

### Painting Tools

The painting system implements:
- Single cell painting
- Line drawing using Bresenham's algorithm
- Circle painting using midpoint circle algorithm
- Rectangle filling

These primitives can be combined to create more complex shapes and patterns.

### Simulation Statistics

Performance metrics are tracked to help optimize the simulation:
- Cell counts by material type
- Active cell percentage
- Update time per frame
- Temperature and pressure averages

## Performance Considerations

### Critical Performance Areas

Profiling has identified these areas as most performance-critical:
1. Cell update functions (especially for liquids and powders)
2. Material interactions and reactions
3. Cross-chunk cell access
4. Temperature and pressure calculations

### Current Optimizations

Several optimizations are already implemented:
- Update tracking to avoid re-processing cells
- Material-specific update skipping (e.g., static solids)
- Chunk-based active region management
- Efficient bit operations for state flags

### Planned Optimizations

Future performance improvements:
- SIMD acceleration for cell updates
- GPU acceleration for parallel processing
- More efficient active cell tracking
- Dynamic chunk loading/unloading based on distance

## Threading Model

The current threading approach uses:
- Thread pool for parallel chunk processing
- Job system with dependency tracking
- Lock-free algorithms where possible
- Task stealing for better load balancing

## Known Limitations and Challenges

Current implementation challenges:
- Complex fluid dynamics are approximated
- Pressure waves don't propagate perfectly
- Temperature diffusion is simplified
- Gas behavior is not physically accurate
- Cross-chunk interactions can create artifacts at boundaries

## Integration with Rendering

The physics system integrates with rendering through:
- Material color properties
- Temperature visualization (hot materials glow)
- Pressure visualization
- Debug visualization options for different cell properties

## Future Research Areas

Areas for future research and improvement:
- Lattice Boltzmann methods for fluids
- SPH (Smoothed Particle Hydrodynamics) for liquids
- Verlet integration for improved object physics
- Continuous cellular automaton techniques
- Neural network-based material behavior models