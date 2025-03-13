# Noita-Like Physics Implementation Plan

## Current Status and Challenges

We have implemented the basic cellular automaton system with the following capabilities:
- Material property system with density, viscosity, etc.
- Basic sand, water, oil, and solid physics
- Chunk-based world management
- Simple material interactions

However, to achieve Noita-quality physics, we need to address these limitations:
1. Liquids don't have realistic flow patterns
2. Current density-based movement is too simplistic
3. Lack of stable fluid stacking
4. Missing material transformations and reactions
5. No temperature effects or phase changes
6. Limited gas simulation

## Proposed Improvements

### 1. Enhanced Liquid Physics

#### Pressure Simulation
* Implement a pressure field to model realistic fluid behavior
* Track pressure for each cell as a numerical value
* Propagate pressure waves through connected fluid cells
* Make fluid velocity proportional to pressure gradients

```cpp
// Example implementation
void CellularPhysics::updateLiquidPressure(int x, int y) {
    // Get the target cell
    Cell& cell = chunkManager->getCell(x, y);
    
    // Calculate pressure based on liquid column above
    float columnHeight = calculateLiquidColumnHeight(x, y);
    float pressure = columnHeight * GRAVITY * LIQUID_DENSITY;
    
    // Store pressure in cell
    cell.pressure = pressure;
    
    // Calculate horizontal pressure gradient
    float leftPressure = getFluidPressure(x-1, y);
    float rightPressure = getFluidPressure(x+1, y);
    
    // Move fluid based on pressure gradient
    if (leftPressure > rightPressure + PRESSURE_THRESHOLD) {
        tryMoveFluid(x, y, x-1, y, (leftPressure - rightPressure) * FLOW_FACTOR);
    } else if (rightPressure > leftPressure + PRESSURE_THRESHOLD) {
        tryMoveFluid(x, y, x+1, y, (rightPressure - leftPressure) * FLOW_FACTOR);
    }
}
```

#### Viscosity Improvements
* Different liquids should flow at different rates
* More viscous fluids should form thick droplets
* Less viscous fluids should spread quickly

#### Surface Tension
* Implement simplified surface tension for small fluid amounts
* Allow droplets to form and maintain shape
* Prevent unrealistic single-cell fluid spreading

```cpp
bool CellularPhysics::shouldMaintainSurfaceTension(int x, int y) {
    // Count fluid neighbors to determine if this is an isolated droplet
    int fluidNeighbors = countFluidNeighbors(x, y);
    
    // Small drops (few neighbors) maintain surface tension
    return fluidNeighbors <= SURFACE_TENSION_THRESHOLD;
}
```

### 2. Improved Solid Material Physics

#### Structural Integrity
* Implement support checking for solid materials
* Allow unsupported solids to fall (like sand)
* Create cascading collapses for large structures

```cpp
bool CellularPhysics::hasSupport(int x, int y) {
    // A cell has support if:
    // 1. It's directly above a solid
    if (isSolid(x, y+1)) return true;
    
    // 2. It has at least two diagonal supports
    int diagonalSupports = 0;
    if (isSolid(x-1, y+1)) diagonalSupports++;
    if (isSolid(x+1, y+1)) diagonalSupports++;
    if (isSolid(x-1, y)) diagonalSupports++;
    if (isSolid(x+1, y)) diagonalSupports++;
    
    return diagonalSupports >= 2;
}
```

#### Material Strength
* Different materials should have different strengths
* Stronger materials can support more weight
* Add cracks and gradual weakening under stress

### 3. Advanced Material Interactions

#### Chemical Reactions
* Implement reaction rules between different materials
* Track reaction products and energy release
* Create chain reactions and emergent behaviors

```cpp
void CellularPhysics::processReactions(int x, int y) {
    Cell& cell = chunkManager->getCell(x, y);
    
    // Check reaction with neighbors
    for (int dy = -1; dy <= 1; dy++) {
        for (int dx = -1; dx <= 1; dx++) {
            if (dx == 0 && dy == 0) continue;
            
            int nx = x + dx;
            int ny = y + dy;
            
            Cell& neighbor = chunkManager->getCell(nx, ny);
            MaterialID neighborMaterial = neighbor.material;
            
            // Check for reaction between materials
            ReactionResult result = materialRegistry->getReactionResult(
                cell.material, neighborMaterial);
                
            if (result.hasReaction) {
                applyReaction(x, y, nx, ny, result);
                
                // Reaction may create heat or other effects
                if (result.heatReleased > 0) {
                    applyHeatToArea(x, y, result.heatReleased, 2.0f);
                }
            }
        }
    }
}
```

#### Temperature System
* Implement heat transfer between cells
* Track temperature as a numerical value
* Create phase changes (freezing, melting, evaporation)
* Implement thermal expansion

#### Electrical Conductivity
* Add properties for electrical conductivity
* Implement charge propagation through conductive materials
* Create effects like lightning and electrical devices

### 4. Gas Simulation

#### Buoyancy and Diffusion
* Implement rising behavior for gases
* Allow gases to diffuse and spread
* Create proper gas mixing and displacement

#### Smoke and Vapor
* Add particle system for more realistic gas visuals
* Implement convection currents and heat-based flow
* Create condensation effects for steam and vapor

### 5. Special Effects and Phenomena

#### Explosions
* Create pressure waves and material displacement
* Add temperature spikes and material conversion
* Implement realistic debris and particle effects

```cpp
void CellularPhysics::createExplosion(int x, int y, float power, float radius) {
    // Apply force to nearby cells
    for (int dy = -radius; dy <= radius; dy++) {
        for (int dx = -radius; dx <= radius; dx++) {
            float distance = std::sqrt(dx*dx + dy*dy);
            if (distance > radius) continue;
            
            int cx = x + dx;
            int cy = y + dy;
            
            // Calculate force based on distance (inverse square law)
            float forceMagnitude = power * (1.0f - distance/radius) * (1.0f - distance/radius);
            
            // Direction from explosion center
            glm::vec2 direction(dx, dy);
            if (glm::length(direction) > 0.0f) {
                direction = glm::normalize(direction);
            }
            
            // Apply force
            applyForce(cx, cy, direction * forceMagnitude);
            
            // Apply heat
            applyHeat(cx, cy, power * 100.0f * (1.0f - distance/radius));
            
            // Convert materials based on explosion strength
            if (distance < radius * 0.3f) {
                convertMaterial(cx, cy, MaterialType::GAS, "Smoke");
            } else if (distance < radius * 0.5f && rand() % 100 < 50) {
                convertMaterial(cx, cy, MaterialType::FIRE, "Fire");
            }
        }
    }
}
```

#### Fire and Combustion
* Implement burning behavior for flammable materials
* Create heat generation and fuel consumption
* Add smoke and ash production

#### Electricity and Lightning
* Create lightning arcs between charged objects
* Implement electrical damage and fire starting
* Add electromagnetic effects

### 6. Performance Optimization

#### Multi-threaded Updates
* Implement thread-safe chunk updates
* Create a worker thread pool for parallel processing
* Optimize for cache coherence

```cpp
void ChunkManager::updateChunksParallel(float deltaTime) {
    // Get active chunks
    const auto activeChunks = getActiveChunks();
    
    // Process chunks in parallel
    std::vector<std::future<void>> futures;
    for (const auto& chunkCoord : activeChunks) {
        futures.push_back(threadPool.enqueue([this, chunkCoord, deltaTime]() {
            Chunk* chunk = getChunk(chunkCoord);
            if (chunk) {
                chunk->update(deltaTime);
            }
        }));
    }
    
    // Wait for all updates to complete
    for (auto& future : futures) {
        future.wait();
    }
}
```

#### GPU Acceleration
* Implement shader-based physics computation
* Use compute shaders for large-scale simulations
* Create hybrid CPU/GPU approach for different materials

#### Spatial Optimizations
* Implement cell activity tracking
* Only update cells that need updating
* Skip sleeping/static regions

### 7. Stability and Edge Cases

#### Boundary Handling
* Improve edge case handling
* Prevent leaking at chunk boundaries
* Implement proper world edges

#### Numerical Stability
* Address issues with fast-moving particles
* Implement sub-step simulation for critical areas
* Add maximum velocity caps

## Implementation Timeline

### Phase 1: Core Physics Enhancements (2 weeks)
- Implement pressure-based fluid simulation
- Add viscosity and surface tension improvements
- Create structural integrity for solids
- Implement basic temperature effects

### Phase 2: Advanced Material Interactions (2 weeks)
- Create chemical reaction system
- Implement proper phase changes
- Add electrical conductivity
- Develop gas simulation with buoyancy

### Phase 3: Special Effects (1 week)
- Implement explosions with pressure waves
- Create fire and combustion system
- Add smoke and particle effects
- Implement lightning and electricity

### Phase 4: Optimization and Stabilization (2 weeks)
- Implement multi-threaded chunk updates
- Add GPU acceleration for physics
- Optimize memory usage and cache coherence
- Fix edge cases and stability issues

## References

### Noita Physics Techniques
1. Falling sand algorithm with cellular automaton
2. Pressure-based liquid simulation
3. Material interaction rules and chemical reactions
4. Rigid body physics for larger objects

### Academic Papers
1. "Cellular Automata for Physical Modelling" - Techniques for realistic material simulation
2. "Large Scale Fluid Simulation using Cellular Automata" - Optimization techniques
3. "Fast Hydraulic Erosion Simulation and Visualization on GPU" - Fluid dynamics approaches

### Implementation Sources
1. Powder Toy - Open source falling sand game with extensive material interactions
2. Noita GDC Talks - Technical discussions of the game's physics system
3. Jos Stam's "Stable Fluids" - Foundational fluid dynamics algorithm