# Physics System Design

## Overview

The Physics System is the heart of Astral, handling the simulation of various materials and their interactions. Inspired by Noita's material-based physics, our system simulates:

- Rigid bodies (solids)
- Granular materials (sand, powder)
- Fluids (water, oil, lava)
- Gases (smoke, steam)
- Material transformations and reactions

The physics system employs a cellular automaton approach, where the world is divided into a grid of cells, each containing a specific material with unique properties and behaviors.

## Material System

### Material Properties and Types

Each material in the simulation has properties that determine its behavior:

```cpp
enum class MaterialType {
    EMPTY,          // Air/vacuum
    SOLID,          // Rigid, immovable (stone, metal)
    POWDER,         // Granular, falls (sand, dirt)
    LIQUID,         // Fluid, flows (water, oil, lava)
    GAS,            // Rises, diffuses (smoke, steam)
    FIRE,           // Special material with transformation rules
    SPECIAL         // Custom behavior materials
};

struct MaterialProperties {
    MaterialType type;
    std::string name;
    
    // Visual properties
    glm::vec4 color;
    float colorVariation;
    bool emissive;
    float emissiveStrength;
    
    // Physical properties
    float density;          // kg/m³, determines if fluids float or sink
    float viscosity;        // For liquids, higher = slower flow
    float friction;         // Resistance to sliding
    float elasticity;       // Bounciness
    float cohesion;         // Tendency to stick to itself
    float adhesion;         // Tendency to stick to other materials
    float dispersion;       // How quickly it spreads (gases/liquids)
    float temperatureMax;   // Maximum temperature before changing state
    float temperatureMin;   // Minimum temperature before changing state
    
    // Simulation behavior
    bool movable;           // Can be moved by physics
    bool flammable;         // Can burn
    float flammability;     // How easily it catches fire (0-1)
    float burnRate;         // How quickly it burns
    
    // Interaction rules
    struct StateChangeRule {
        float temperatureThreshold;
        MaterialID targetMaterial;
        float probability;
    };
    std::vector<StateChangeRule> stateChanges;
    
    struct ReactionRule {
        MaterialID reactantMaterial;
        MaterialID resultMaterial;
        float probability;
    };
    std::vector<ReactionRule> reactions;
};

// Material registry to store and access all materials
class MaterialRegistry {
private:
    std::unordered_map<MaterialID, MaterialProperties> materials;
    std::unordered_map<std::string, MaterialID> nameToID;
    MaterialID nextID = 1; // 0 is reserved for EMPTY/AIR
    
public:
    MaterialRegistry();
    
    // Register a new material
    MaterialID registerMaterial(const MaterialProperties& properties);
    
    // Get material properties
    const MaterialProperties& getMaterial(MaterialID id) const;
    const MaterialProperties& getMaterialByName(const std::string& name) const;
    
    // Check if material exists
    bool hasMaterial(MaterialID id) const;
    bool hasMaterialName(const std::string& name) const;
    
    // Get material ID from name
    MaterialID getIDFromName(const std::string& name) const;
    
    // Load materials from file
    bool loadMaterialsFromFile(const std::string& filepath);
    
    // Default materials
    MaterialID air() const { return 0; }
    MaterialID stone() const { return getIDFromName("Stone"); }
    MaterialID sand() const { return getIDFromName("Sand"); }
    MaterialID water() const { return getIDFromName("Water"); }
    MaterialID lava() const { return getIDFromName("Lava"); }
    // ... etc for commonly used materials
};
```

### Default Materials

The engine comes with a set of default materials with carefully tuned properties:

```cpp
void MaterialRegistry::registerDefaultMaterials() {
    // Air/Empty
    MaterialProperties air;
    air.name = "Air";
    air.type = MaterialType::EMPTY;
    air.density = 1.0f;
    air.color = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);
    air.movable = true;
    registerMaterial(air);
    
    // Stone
    MaterialProperties stone;
    stone.name = "Stone";
    stone.type = MaterialType::SOLID;
    stone.density = 2600.0f;
    stone.color = glm::vec4(0.5f, 0.5f, 0.5f, 1.0f);
    stone.colorVariation = 0.1f;
    stone.movable = false;
    stone.friction = 0.8f;
    registerMaterial(stone);
    
    // Sand
    MaterialProperties sand;
    sand.name = "Sand";
    sand.type = MaterialType::POWDER;
    sand.density = 1600.0f;
    sand.color = glm::vec4(0.76f, 0.7f, 0.5f, 1.0f);
    sand.colorVariation = 0.1f;
    sand.movable = true;
    sand.friction = 0.5f;
    registerMaterial(sand);
    
    // Water
    MaterialProperties water;
    water.name = "Water";
    water.type = MaterialType::LIQUID;
    water.density = 1000.0f;
    water.color = glm::vec4(0.0f, 0.2f, 0.8f, 0.8f);
    water.colorVariation = 0.05f;
    water.movable = true;
    water.viscosity = 0.3f;
    water.dispersion = 0.8f;
    // Water transitions to steam at high temp
    water.stateChanges.push_back({100.0f, getIDFromName("Steam"), 0.1f});
    // Water transitions to ice at low temp
    water.stateChanges.push_back({0.0f, getIDFromName("Ice"), 0.1f});
    registerMaterial(water);
    
    // Oil
    MaterialProperties oil;
    oil.name = "Oil";
    oil.type = MaterialType::LIQUID;
    oil.density = 900.0f;  // Lighter than water
    oil.color = glm::vec4(0.4f, 0.2f, 0.0f, 0.8f);
    oil.movable = true;
    oil.viscosity = 0.6f;  // More viscous than water
    oil.dispersion = 0.6f;
    oil.flammable = true;
    oil.flammability = 0.8f;
    oil.burnRate = 0.05f;
    registerMaterial(oil);
    
    // Lava
    MaterialProperties lava;
    lava.name = "Lava";
    lava.type = MaterialType::LIQUID;
    lava.density = 3100.0f;
    lava.color = glm::vec4(0.8f, 0.3f, 0.0f, 1.0f);
    lava.colorVariation = 0.1f;
    lava.emissive = true;
    lava.emissiveStrength = 0.5f;
    lava.movable = true;
    lava.viscosity = 0.8f;  // Very viscous
    lava.dispersion = 0.3f;
    lava.temperatureMax = 1200.0f;
    // Lava turns to stone when cool
    lava.stateChanges.push_back({700.0f, getIDFromName("Stone"), 0.05f});
    registerMaterial(lava);
    
    // Steam
    MaterialProperties steam;
    steam.name = "Steam";
    steam.type = MaterialType::GAS;
    steam.density = 0.6f;
    steam.color = glm::vec4(0.8f, 0.8f, 0.8f, 0.3f);
    steam.movable = true;
    steam.dispersion = 0.9f;
    // Steam condenses back to water when cool
    steam.stateChanges.push_back({99.0f, getIDFromName("Water"), 0.01f});
    registerMaterial(steam);
    
    // Smoke
    MaterialProperties smoke;
    smoke.name = "Smoke";
    smoke.type = MaterialType::GAS;
    smoke.density = 0.5f;
    smoke.color = glm::vec4(0.2f, 0.2f, 0.2f, 0.5f);
    smoke.colorVariation = 0.1f;
    smoke.movable = true;
    smoke.dispersion = 0.95f;
    // Smoke gradually disappears
    smoke.stateChanges.push_back({-999.0f, getIDFromName("Air"), 0.001f});
    registerMaterial(smoke);
    
    // Fire
    MaterialProperties fire;
    fire.name = "Fire";
    fire.type = MaterialType::FIRE;
    fire.density = 0.3f;
    fire.color = glm::vec4(0.9f, 0.4f, 0.1f, 0.9f);
    fire.colorVariation = 0.2f;
    fire.emissive = true;
    fire.emissiveStrength = 0.8f;
    fire.movable = true;
    fire.dispersion = 0.7f;
    // Fire burns out naturally
    fire.stateChanges.push_back({-999.0f, getIDFromName("Smoke"), 0.05f});
    registerMaterial(fire);
    
    // Wood
    MaterialProperties wood;
    wood.name = "Wood";
    wood.type = MaterialType::SOLID;
    wood.density = 700.0f;
    wood.color = glm::vec4(0.6f, 0.4f, 0.2f, 1.0f);
    wood.colorVariation = 0.1f;
    wood.movable = false;
    wood.flammable = true;
    wood.flammability = 0.4f;
    wood.burnRate = 0.01f;
    registerMaterial(wood);
    
    // Many more materials would be defined here...
}
```

## Cell Physics Update

The core of our cellular automaton approach is the update logic for each cell:

```cpp
class CellularPhysics {
private:
    MaterialRegistry* materialRegistry;
    ChunkManager* chunkManager;
    std::vector<std::vector<bool>> updated; // Tracks which cells updated this frame
    
    // Helper methods
    bool canMove(int x, int y, int newX, int newY);
    void swapCells(int x, int y, int newX, int newY);
    void moveCell(int x, int y, int newX, int newY);
    void processMaterialInteraction(int x1, int y1, int x2, int y2);
    void applyTemperature(int x, int y, float deltaTime);
    
public:
    CellularPhysics(MaterialRegistry* registry, ChunkManager* chunkManager);
    
    // Update a specific chunk
    void updateChunk(Chunk* chunk, float deltaTime);
    
    // Update methods for different material types
    void updateEmpty(int x, int y, float deltaTime);
    void updateSolid(int x, int y, float deltaTime);
    void updatePowder(int x, int y, float deltaTime);
    void updateLiquid(int x, int y, float deltaTime);
    void updateGas(int x, int y, float deltaTime);
    void updateFire(int x, int y, float deltaTime);
    void updateSpecial(int x, int y, float deltaTime);
    
    // Main update method
    void update(float deltaTime);
};
```

### Update Methods for Different Materials

Different material types have specialized update logic:

```cpp
void CellularPhysics::updatePowder(int x, int y, float deltaTime) {
    Cell& cell = chunkManager->getCell(x, y);
    const MaterialProperties& props = materialRegistry->getMaterial(cell.material);
    
    if (updated[y][x] || !props.movable) return;
    
    updated[y][x] = true;
    
    // Try to fall directly down
    if (canMove(x, y, x, y + 1)) {
        moveCell(x, y, x, y + 1);
        return;
    }
    
    // Random direction preference to avoid bias
    bool tryLeftFirst = rand() % 2 == 0;
    
    // Try to fall diagonally
    if (tryLeftFirst) {
        if (canMove(x, y, x - 1, y + 1)) {
            moveCell(x, y, x - 1, y + 1);
            return;
        }
        if (canMove(x, y, x + 1, y + 1)) {
            moveCell(x, y, x + 1, y + 1);
            return;
        }
    } else {
        if (canMove(x, y, x + 1, y + 1)) {
            moveCell(x, y, x + 1, y + 1);
            return;
        }
        if (canMove(x, y, x - 1, y + 1)) {
            moveCell(x, y, x - 1, y + 1);
            return;
        }
    }
    
    // Apply powder compression mechanics (optional)
    // If powder is under pressure (certain amount of powder above it),
    // it could have a chance to compress into a solid
}

void CellularPhysics::updateLiquid(int x, int y, float deltaTime) {
    Cell& cell = chunkManager->getCell(x, y);
    const MaterialProperties& props = materialRegistry->getMaterial(cell.material);
    
    if (updated[y][x] || !props.movable) return;
    
    updated[y][x] = true;
    
    // Viscosity affects update frequency
    if (rand() % 100 < props.viscosity * 100) return;
    
    // Try to fall down
    if (canMove(x, y, x, y + 1)) {
        moveCell(x, y, x, y + 1);
        return;
    }
    
    // Random direction preference to avoid bias
    bool tryLeftFirst = rand() % 2 == 0;
    
    // Try to fall diagonally
    if (tryLeftFirst) {
        if (canMove(x, y, x - 1, y + 1)) {
            moveCell(x, y, x - 1, y + 1);
            return;
        }
        if (canMove(x, y, x + 1, y + 1)) {
            moveCell(x, y, x + 1, y + 1);
            return;
        }
    } else {
        if (canMove(x, y, x + 1, y + 1)) {
            moveCell(x, y, x + 1, y + 1);
            return;
        }
        if (canMove(x, y, x - 1, y + 1)) {
            moveCell(x, y, x - 1, y + 1);
            return;
        }
    }
    
    // Try to flow horizontally if can't fall
    // Dispersion rate affects how far it spreads
    int dispersionDistance = props.dispersion * 5;
    
    // Try left or right with random preference
    if (tryLeftFirst) {
        for (int i = 1; i <= dispersionDistance; i++) {
            if (canMove(x, y, x - i, y)) {
                moveCell(x, y, x - i, y);
                return;
            }
            
            if (i > 1) break; // Only check immediate neighbors first
        }
        
        for (int i = 1; i <= dispersionDistance; i++) {
            if (canMove(x, y, x + i, y)) {
                moveCell(x, y, x + i, y);
                return;
            }
            
            if (i > 1) break; // Only check immediate neighbors first
        }
    } else {
        for (int i = 1; i <= dispersionDistance; i++) {
            if (canMove(x, y, x + i, y)) {
                moveCell(x, y, x + i, y);
                return;
            }
            
            if (i > 1) break; // Only check immediate neighbors first
        }
        
        for (int i = 1; i <= dispersionDistance; i++) {
            if (canMove(x, y, x - i, y)) {
                moveCell(x, y, x - i, y);
                return;
            }
            
            if (i > 1) break; // Only check immediate neighbors first
        }
    }
}

void CellularPhysics::updateGas(int x, int y, float deltaTime) {
    Cell& cell = chunkManager->getCell(x, y);
    const MaterialProperties& props = materialRegistry->getMaterial(cell.material);
    
    if (updated[y][x] || !props.movable) return;
    
    updated[y][x] = true;
    
    // Gases have a chance to disappear or transform
    if (props.stateChanges.size() > 0) {
        for (const auto& change : props.stateChanges) {
            if (rand() % 1000 < change.probability * 1000) {
                cell.material = change.targetMaterial;
                return;
            }
        }
    }
    
    // Try to rise up (opposite of liquids/powders)
    if (canMove(x, y, x, y - 1)) {
        moveCell(x, y, x, y - 1);
        return;
    }
    
    // Random direction for diagonal rise
    bool tryLeftFirst = rand() % 2 == 0;
    
    // Try to rise diagonally
    if (tryLeftFirst) {
        if (canMove(x, y, x - 1, y - 1)) {
            moveCell(x, y, x - 1, y - 1);
            return;
        }
        if (canMove(x, y, x + 1, y - 1)) {
            moveCell(x, y, x + 1, y - 1);
            return;
        }
    } else {
        if (canMove(x, y, x + 1, y - 1)) {
            moveCell(x, y, x + 1, y - 1);
            return;
        }
        if (canMove(x, y, x - 1, y - 1)) {
            moveCell(x, y, x - 1, y - 1);
            return;
        }
    }
    
    // Horizontal dispersion (similar to liquid but with higher dispersion)
    int dispersionDistance = props.dispersion * 7;
    
    if (tryLeftFirst) {
        for (int i = 1; i <= dispersionDistance; i++) {
            if (canMove(x, y, x - i, y)) {
                moveCell(x, y, x - i, y);
                return;
            }
        }
        
        for (int i = 1; i <= dispersionDistance; i++) {
            if (canMove(x, y, x + i, y)) {
                moveCell(x, y, x + i, y);
                return;
            }
        }
    } else {
        for (int i = 1; i <= dispersionDistance; i++) {
            if (canMove(x, y, x + i, y)) {
                moveCell(x, y, x + i, y);
                return;
            }
        }
        
        for (int i = 1; i <= dispersionDistance; i++) {
            if (canMove(x, y, x - i, y)) {
                moveCell(x, y, x - i, y);
                return;
            }
        }
    }
}

void CellularPhysics::updateFire(int x, int y, float deltaTime) {
    Cell& cell = chunkManager->getCell(x, y);
    const MaterialProperties& props = materialRegistry->getMaterial(cell.material);
    
    if (updated[y][x]) return;
    
    updated[y][x] = true;
    
    // Fire has a chance to spread to flammable neighbors
    for (int dy = -1; dy <= 1; dy++) {
        for (int dx = -1; dx <= 1; dx++) {
            if (dx == 0 && dy == 0) continue;
            
            int nx = x + dx;
            int ny = y + dy;
            
            Cell& neighbor = chunkManager->getCell(nx, ny);
            const MaterialProperties& neighborProps = materialRegistry->getMaterial(neighbor.material);
            
            if (neighborProps.flammable) {
                float ignitionChance = neighborProps.flammability * props.temperatureMax / 1000.0f;
                if (rand() % 1000 < ignitionChance * 1000) {
                    neighbor.material = materialRegistry->getIDFromName("Fire");
                }
            }
        }
    }
    
    // Fire rises like gas
    if (canMove(x, y, x, y - 1)) {
        moveCell(x, y, x, y - 1);
        return;
    }
    
    // Fire has a chance to burn out and turn to smoke
    if (rand() % 100 < 5) {
        cell.material = materialRegistry->getIDFromName("Smoke");
        return;
    }
    
    // Fire can also rise diagonally
    bool tryLeftFirst = rand() % 2 == 0;
    
    if (tryLeftFirst) {
        if (canMove(x, y, x - 1, y - 1)) {
            moveCell(x, y, x - 1, y - 1);
            return;
        }
        if (canMove(x, y, x + 1, y - 1)) {
            moveCell(x, y, x + 1, y - 1);
            return;
        }
    } else {
        if (canMove(x, y, x + 1, y - 1)) {
            moveCell(x, y, x + 1, y - 1);
            return;
        }
        if (canMove(x, y, x - 1, y - 1)) {
            moveCell(x, y, x - 1, y - 1);
            return;
        }
    }
}
```

### Material Interaction Logic

When materials come into contact, they can interact:

```cpp
void CellularPhysics::processMaterialInteraction(int x1, int y1, int x2, int y2) {
    Cell& cell1 = chunkManager->getCell(x1, y1);
    Cell& cell2 = chunkManager->getCell(x2, y2);
    
    const MaterialProperties& props1 = materialRegistry->getMaterial(cell1.material);
    const MaterialProperties& props2 = materialRegistry->getMaterial(cell2.material);
    
    // Temperature transfer
    float tempDiff = cell1.temperature - cell2.temperature;
    float transferAmount = tempDiff * 0.1f;
    cell1.temperature -= transferAmount;
    cell2.temperature += transferAmount;
    
    // Reactions based on material properties
    for (const auto& reaction : props1.reactions) {
        if (reaction.reactantMaterial == cell2.material) {
            if (rand() % 1000 < reaction.probability * 1000) {
                cell1.material = reaction.resultMaterial;
                // Maybe also change cell2 based on the reaction
                break;
            }
        }
    }
    
    // Special material interactions
    
    // Water + Lava = Stone + Steam
    if ((cell1.material == materialRegistry->getIDFromName("Water") && 
         cell2.material == materialRegistry->getIDFromName("Lava")) ||
        (cell2.material == materialRegistry->getIDFromName("Water") && 
         cell1.material == materialRegistry->getIDFromName("Lava"))) {
        
        cell1.material = materialRegistry->getIDFromName("Stone");
        cell2.material = materialRegistry->getIDFromName("Steam");
        cell1.temperature = 100.0f;
        cell2.temperature = 150.0f;
        return;
    }
    
    // Water extinguishes fire
    if (cell1.material == materialRegistry->getIDFromName("Fire") && 
        cell2.material == materialRegistry->getIDFromName("Water")) {
        cell1.material = materialRegistry->getIDFromName("Steam");
        return;
    }
    if (cell2.material == materialRegistry->getIDFromName("Fire") && 
        cell1.material == materialRegistry->getIDFromName("Water")) {
        cell2.material = materialRegistry->getIDFromName("Steam");
        return;
    }
    
    // Many more specific interactions...
}
```

## Temperature System

Temperature adds a layer of complexity and realism:

```cpp
void CellularPhysics::applyTemperature(int x, int y, float deltaTime) {
    Cell& cell = chunkManager->getCell(x, y);
    const MaterialProperties& props = materialRegistry->getMaterial(cell.material);
    
    // Temperature affects state changes
    for (const auto& change : props.stateChanges) {
        if ((change.temperatureThreshold > 0 && cell.temperature > change.temperatureThreshold) ||
            (change.temperatureThreshold < 0 && cell.temperature < -change.temperatureThreshold)) {
            
            if (rand() % 1000 < change.probability * 1000) {
                cell.material = change.targetMaterial;
                return;
            }
        }
    }
    
    // Temperature spreads to neighbors
    for (int dy = -1; dy <= 1; dy++) {
        for (int dx = -1; dx <= 1; dx++) {
            if (dx == 0 && dy == 0) continue;
            
            int nx = x + dx;
            int ny = y + dy;
            
            if (nx < 0 || nx >= worldWidth || ny < 0 || ny >= worldHeight) continue;
            
            Cell& neighbor = chunkManager->getCell(nx, ny);
            
            // Heat transfer
            float tempDiff = cell.temperature - neighbor.temperature;
            float transferAmount = tempDiff * 0.1f * deltaTime;
            
            cell.temperature -= transferAmount;
            neighbor.temperature += transferAmount;
        }
    }
    
    // Temperature naturally decays over time (heat dissipation)
    float ambientTemp = 20.0f; // Room temperature
    cell.temperature = cell.temperature + (ambientTemp - cell.temperature) * 0.01f * deltaTime;
}
```

## Optimizations and Parallelism

To handle large worlds efficiently, we optimize the physics simulation:

```cpp
class ChunkBasedPhysics {
private:
    MaterialRegistry* materialRegistry;
    ChunkManager* chunkManager;
    ThreadPool* threadPool;
    
    // Checkerboard update pattern for parallelism
    bool evenFrame;
    
public:
    ChunkBasedPhysics(MaterialRegistry* registry, ChunkManager* chunkManager, ThreadPool* threadPool);
    
    void update(float deltaTime) {
        // Checkerboard pattern for parallel updates
        evenFrame = !evenFrame;
        
        // Get active chunks (those with moving materials)
        const auto& activeChunks = chunkManager->getActiveChunks();
        
        // Divide chunks into two sets for checkerboard pattern
        std::vector<Chunk*> evenChunks;
        std::vector<Chunk*> oddChunks;
        
        for (Chunk* chunk : activeChunks) {
            ChunkCoord coord = chunk->getCoord();
            if ((coord.x + coord.y) % 2 == 0) {
                evenChunks.push_back(chunk);
            } else {
                oddChunks.push_back(chunk);
            }
        }
        
        // Update first set in parallel
        std::vector<Chunk*>& firstSet = evenFrame ? evenChunks : oddChunks;
        std::vector<std::future<void>> futures;
        
        for (Chunk* chunk : firstSet) {
            futures.push_back(threadPool->enqueue([=]() {
                updateChunk(chunk, deltaTime);
            }));
        }
        
        // Wait for first set to complete
        for (auto& future : futures) {
            future.wait();
        }
        futures.clear();
        
        // Update second set in parallel
        std::vector<Chunk*>& secondSet = evenFrame ? oddChunks : evenChunks;
        
        for (Chunk* chunk : secondSet) {
            futures.push_back(threadPool->enqueue([=]() {
                updateChunk(chunk, deltaTime);
            }));
        }
        
        // Wait for second set to complete
        for (auto& future : futures) {
            future.wait();
        }
    }
    
    void updateChunk(Chunk* chunk, float deltaTime) {
        // Skip chunks with no active cells
        if (!chunk->hasActiveCells()) return;
        
        // Bottom-up update for falling materials (powders, liquids)
        for (int y = CHUNK_SIZE - 1; y >= 0; y--) {
            for (int x = 0; x < CHUNK_SIZE; x++) {
                WorldCoord worldCoord = chunkManager->chunkToWorldCoord(chunk->getCoord(), {x, y});
                
                // Get cell and material
                Cell& cell = chunk->getCell(x, y);
                const MaterialProperties& props = materialRegistry->getMaterial(cell.material);
                
                switch (props.type) {
                    case MaterialType::POWDER:
                        updatePowder(worldCoord.x, worldCoord.y, deltaTime);
                        break;
                    case MaterialType::LIQUID:
                        updateLiquid(worldCoord.x, worldCoord.y, deltaTime);
                        break;
                    // Other types handled in different passes
                }
            }
        }
        
        // Top-down update for rising materials (gases)
        for (int y = 0; y < CHUNK_SIZE; y++) {
            for (int x = 0; x < CHUNK_SIZE; x++) {
                WorldCoord worldCoord = chunkManager->chunkToWorldCoord(chunk->getCoord(), {x, y});
                
                Cell& cell = chunk->getCell(x, y);
                const MaterialProperties& props = materialRegistry->getMaterial(cell.material);
                
                if (props.type == MaterialType::GAS) {
                    updateGas(worldCoord.x, worldCoord.y, deltaTime);
                }
            }
        }
        
        // Fire and special materials update
        for (int y = 0; y < CHUNK_SIZE; y++) {
            for (int x = 0; x < CHUNK_SIZE; x++) {
                WorldCoord worldCoord = chunkManager->chunkToWorldCoord(chunk->getCoord(), {x, y});
                
                Cell& cell = chunk->getCell(x, y);
                const MaterialProperties& props = materialRegistry->getMaterial(cell.material);
                
                if (props.type == MaterialType::FIRE) {
                    updateFire(worldCoord.x, worldCoord.y, deltaTime);
                } else if (props.type == MaterialType::SPECIAL) {
                    updateSpecial(worldCoord.x, worldCoord.y, deltaTime);
                }
            }
        }
        
        // Apply temperature effects
        for (int y = 0; y < CHUNK_SIZE; y++) {
            for (int x = 0; x < CHUNK_SIZE; x++) {
                WorldCoord worldCoord = chunkManager->chunkToWorldCoord(chunk->getCoord(), {x, y});
                applyTemperature(worldCoord.x, worldCoord.y, deltaTime);
            }
        }
        
        // After update, check if chunk is still active
        chunk->updateActiveState();
    }
    
    // Individual material update methods would be defined here...
};
```

## Chunk Activity Tracking

Chunks are only simulated if they contain active materials:

```cpp
class Chunk {
    // ... other members ...
    
    std::vector<std::vector<bool>> activeCells;
    bool hasActive;
    
    void updateActiveState() {
        hasActive = false;
        
        for (int y = 0; y < CHUNK_SIZE; y++) {
            for (int x = 0; x < CHUNK_SIZE; x++) {
                Cell& cell = getCell(x, y);
                const MaterialProperties& props = materialRegistry->getMaterial(cell.material);
                
                // A cell is active if it's a material that can move or react
                bool isActive = props.movable || 
                              props.type == MaterialType::FIRE || 
                              cell.temperature > 50.0f || // Temperature can cause activity
                              props.type == MaterialType::SPECIAL;
                
                // Also check if it has potential to move (e.g., not blocked)
                if (isActive) {
                    switch (props.type) {
                        case MaterialType::POWDER:
                        case MaterialType::LIQUID:
                            // Look down
                            if (y < CHUNK_SIZE - 1) {
                                Cell& below = getCell(x, y + 1);
                                if (below.material == materialRegistry->air() || 
                                    (materialRegistry->getMaterial(below.material).density < props.density)) {
                                    activeCells[y][x] = true;
                                    hasActive = true;
                                    continue;
                                }
                            }
                            break;
                            
                        case MaterialType::GAS:
                            // Look up
                            if (y > 0) {
                                Cell& above = getCell(x, y - 1);
                                if (above.material == materialRegistry->air() || 
                                    (materialRegistry->getMaterial(above.material).density > props.density)) {
                                    activeCells[y][x] = true;
                                    hasActive = true;
                                    continue;
                                }
                            }
                            break;
                            
                        case MaterialType::FIRE:
                        case MaterialType::SPECIAL:
                            activeCells[y][x] = true;
                            hasActive = true;
                            continue;
                    }
                    
                    // Check for active neighbors that might affect this cell
                    bool hasActiveNeighbor = false;
                    for (int dy = -1; dy <= 1 && !hasActiveNeighbor; dy++) {
                        for (int dx = -1; dx <= 1; dx++) {
                            if (dx == 0 && dy == 0) continue;
                            
                            int nx = x + dx;
                            int ny = y + dy;
                            
                            if (nx < 0 || nx >= CHUNK_SIZE || ny < 0 || ny >= CHUNK_SIZE) continue;
                            
                            if (activeCells[ny][nx]) {
                                hasActiveNeighbor = true;
                                break;
                            }
                        }
                    }
                    
                    if (hasActiveNeighbor) {
                        activeCells[y][x] = true;
                        hasActive = true;
                    } else {
                        activeCells[y][x] = false;
                    }
                } else {
                    activeCells[y][x] = false;
                }
            }
        }
    }
    
    bool hasActiveCells() const {
        return hasActive;
    }
};
```

## Fluid Pressure Simulation

For more realistic fluids, we add pressure calculations:

```cpp
void CellularPhysics::updateLiquidWithPressure(int x, int y, float deltaTime) {
    Cell& cell = chunkManager->getCell(x, y);
    const MaterialProperties& props = materialRegistry->getMaterial(cell.material);
    
    if (updated[y][x] || !props.movable) return;
    
    updated[y][x] = true;
    
    // Calculate pressure (how much liquid is stacked above)
    float pressure = 0.0f;
    int stackHeight = 0;
    
    for (int checkY = y - 1; checkY >= 0; checkY--) {
        Cell& above = chunkManager->getCell(x, checkY);
        const MaterialProperties& aboveProps = materialRegistry->getMaterial(above.material);
        
        if (aboveProps.type == MaterialType::LIQUID && aboveProps.density >= props.density) {
            stackHeight++;
            pressure += aboveProps.density / 1000.0f;
        } else {
            break;
        }
    }
    
    // Higher pressure means more horizontal flow
    int horizontalRange = 1 + (int)(pressure * 5); // More pressure = further flow
    int dispersionChance = 100 - (int)(props.viscosity * 100); // Less viscous = more flow
    
    // Try to fall down
    if (canMove(x, y, x, y + 1)) {
        moveCell(x, y, x, y + 1);
        return;
    }
    
    // Try diagonal downs
    bool tryLeftFirst = rand() % 2 == 0;
    
    if (tryLeftFirst) {
        if (canMove(x, y, x - 1, y + 1)) {
            moveCell(x, y, x - 1, y + 1);
            return;
        }
        if (canMove(x, y, x + 1, y + 1)) {
            moveCell(x, y, x + 1, y + 1);
            return;
        }
    } else {
        if (canMove(x, y, x + 1, y + 1)) {
            moveCell(x, y, x + 1, y + 1);
            return;
        }
        if (canMove(x, y, x - 1, y + 1)) {
            moveCell(x, y, x - 1, y + 1);
            return;
        }
    }
    
    // Horizontal flow based on pressure
    for (int dist = 1; dist <= horizontalRange; dist++) {
        // Chances decrease with distance
        if (rand() % 100 >= dispersionChance) continue;
        
        if (tryLeftFirst) {
            if (canMove(x, y, x - dist, y)) {
                moveCell(x, y, x - dist, y);
                return;
            }
            if (canMove(x, y, x + dist, y)) {
                moveCell(x, y, x + dist, y);
                return;
            }
        } else {
            if (canMove(x, y, x + dist, y)) {
                moveCell(x, y, x + dist, y);
                return;
            }
            if (canMove(x, y, x - dist, y)) {
                moveCell(x, y, x - dist, y);
                return;
            }
        }
    }
    
    // High pressure can force liquid upward (against gravity)
    if (pressure > 2.0f) {
        for (int upDist = 1; upDist <= (int)(pressure / 2); upDist++) {
            // Chance decreases with height
            if (rand() % 100 > (pressure * 25) / upDist) continue;
            
            if (canMove(x, y, x, y - upDist)) {
                moveCell(x, y, x, y - upDist);
                return;
            }
        }
    }
}
```

## Physics Integration with Rigid Body Systems

For more complex physics objects that span multiple cells, we integrate with a traditional rigid body physics system:

```cpp
class RigidBodyPhysics {
private:
    b2World* world;
    ChunkManager* chunkManager;
    MaterialRegistry* materialRegistry;
    
    struct RigidBody {
        b2Body* body;
        std::vector<WorldCoord> cells;
        MaterialID material;
    };
    
    std::vector<RigidBody> rigidBodies;
    
public:
    RigidBodyPhysics(ChunkManager* chunkManager, MaterialRegistry* materialRegistry);
    ~RigidBodyPhysics();
    
    void createRigidBodyFromCells(const std::vector<WorldCoord>& cells, MaterialID material);
    void updateRigidBodies(float deltaTime);
    void handleRigidBodyCollisions();
    void synchronizeRigidBodiesWithGrid();
};

RigidBodyPhysics::RigidBodyPhysics(ChunkManager* chunkManager, MaterialRegistry* materialRegistry) 
    : chunkManager(chunkManager), materialRegistry(materialRegistry) {
    
    // Create Box2D world
    b2Vec2 gravity(0.0f, 10.0f);
    world = new b2World(gravity);
}

RigidBodyPhysics::~RigidBodyPhysics() {
    delete world;
}

void RigidBodyPhysics::createRigidBodyFromCells(const std::vector<WorldCoord>& cells, MaterialID material) {
    if (cells.empty()) return;
    
    // Create body definition
    b2BodyDef bodyDef;
    bodyDef.type = b2_dynamicBody;
    
    // Find center of mass
    b2Vec2 centerOfMass(0.0f, 0.0f);
    for (const auto& cell : cells) {
        centerOfMass.x += cell.x;
        centerOfMass.y += cell.y;
    }
    centerOfMass.x /= cells.size();
    centerOfMass.y /= cells.size();
    
    bodyDef.position.Set(centerOfMass.x, centerOfMass.y);
    
    // Create body
    b2Body* body = world->CreateBody(&bodyDef);
    
    // Add fixtures for each cell (simplified as boxes)
    const MaterialProperties& props = materialRegistry->getMaterial(material);
    float density = props.density / 1000.0f;
    
    for (const auto& cell : cells) {
        b2PolygonShape shape;
        shape.SetAsBox(0.5f, 0.5f, b2Vec2(cell.x - centerOfMass.x, cell.y - centerOfMass.y), 0.0f);
        
        b2FixtureDef fixtureDef;
        fixtureDef.shape = &shape;
        fixtureDef.density = density;
        fixtureDef.friction = props.friction;
        fixtureDef.restitution = props.elasticity;
        
        body->CreateFixture(&fixtureDef);
    }
    
    // Add to list
    RigidBody rigidBody;
    rigidBody.body = body;
    rigidBody.cells = cells;
    rigidBody.material = material;
    rigidBodies.push_back(rigidBody);
    
    // Remove cells from grid
    for (const auto& cell : cells) {
        Cell& gridCell = chunkManager->getCell(cell.x, cell.y);
        gridCell.material = materialRegistry->air();
    }
}

void RigidBodyPhysics::updateRigidBodies(float deltaTime) {
    // Step the physics world
    const int velocityIterations = 6;
    const int positionIterations = 2;
    world->Step(deltaTime, velocityIterations, positionIterations);
}

void RigidBodyPhysics::synchronizeRigidBodiesWithGrid() {
    // For each rigid body
    for (auto it = rigidBodies.begin(); it != rigidBodies.end();) {
        RigidBody& rb = *it;
        
        // Get body transform
        b2Transform transform = rb.body->GetTransform();
        
        // Check if the body should break apart
        bool shouldBreak = false;
        
        // Calculate new cell positions
        std::vector<WorldCoord> newCells;
        for (const auto& cell : rb.cells) {
            b2Vec2 localPos(cell.x - rb.body->GetPosition().x, cell.y - rb.body->GetPosition().y);
            b2Vec2 newPos = b2Mul(transform, localPos);
            
            WorldCoord newCell{(int)round(newPos.x + rb.body->GetPosition().x), 
                              (int)round(newPos.y + rb.body->GetPosition().y)};
            
            // Check if new position is valid
            if (chunkManager->isValidCoord(newCell)) {
                const Cell& gridCell = chunkManager->getCell(newCell.x, newCell.y);
                if (gridCell.material != materialRegistry->air()) {
                    // Collision with existing cell - might break apart
                    shouldBreak = true;
                }
                
                newCells.push_back(newCell);
            } else {
                shouldBreak = true;
            }
        }
        
        if (shouldBreak) {
            // Break apart the rigid body
            world->DestroyBody(rb.body);
            
            // Add individual cells back to the grid
            for (const auto& cell : rb.cells) {
                if (chunkManager->isValidCoord(cell)) {
                    Cell& gridCell = chunkManager->getCell(cell.x, cell.y);
                    gridCell.material = rb.material;
                }
            }
            
            // Remove from list
            it = rigidBodies.erase(it);
        } else {
            // Update grid
            for (const auto& oldCell : rb.cells) {
                if (chunkManager->isValidCoord(oldCell)) {
                    Cell& gridCell = chunkManager->getCell(oldCell.x, oldCell.y);
                    if (gridCell.material == materialRegistry->air()) {
                        // Only clear if it's still empty (wasn't filled by something else)
                        gridCell.material = materialRegistry->air();
                    }
                }
            }
            
            for (const auto& newCell : newCells) {
                Cell& gridCell = chunkManager->getCell(newCell.x, newCell.y);
                gridCell.material = rb.material;
            }
            
            // Update cells list
            rb.cells = newCells;
            
            ++it;
        }
    }
}
```

## Physics Debugging Tools

Debugging tools for visualizing the physics simulation:

```cpp
class PhysicsDebugRenderer {
private:
    ChunkManager* chunkManager;
    MaterialRegistry* materialRegistry;
    RenderingSystem* renderer;
    
    bool showVelocity;
    bool showPressure;
    bool showTemperature;
    bool showActive;
    
public:
    PhysicsDebugRenderer(ChunkManager* chunkManager, MaterialRegistry* materialRegistry, RenderingSystem* renderer);
    
    void setShowVelocity(bool show) { showVelocity = show; }
    void setShowPressure(bool show) { showPressure = show; }
    void setShowTemperature(bool show) { showTemperature = show; }
    void setShowActive(bool show) { showActive = show; }
    
    void renderDebugInfo();
    
    void renderVelocityOverlay();
    void renderPressureOverlay();
    void renderTemperatureOverlay();
    void renderActiveOverlay();
};

void PhysicsDebugRenderer::renderDebugInfo() {
    if (showVelocity) renderVelocityOverlay();
    if (showPressure) renderPressureOverlay();
    if (showTemperature) renderTemperatureOverlay();
    if (showActive) renderActiveOverlay();
}

void PhysicsDebugRenderer::renderTemperatureOverlay() {
    // Get visible area
    WorldRect visibleArea = renderer->getVisibleWorldRect();
    
    for (int y = visibleArea.y; y < visibleArea.y + visibleArea.height; y++) {
        for (int x = visibleArea.x; x < visibleArea.x + visibleArea.width; x++) {
            if (!chunkManager->isValidCoord({x, y})) continue;
            
            const Cell& cell = chunkManager->getCell(x, y);
            
            if (cell.material == materialRegistry->air()) continue;
            
            // Temperature gradient: blue (cold) to red (hot)
            float normalized = (cell.temperature - 0.0f) / 1000.0f;
            normalized = std::max(0.0f, std::min(1.0f, normalized));
            
            glm::vec4 color;
            if (normalized < 0.5f) {
                // Blue to white
                float t = normalized * 2.0f;
                color = glm::vec4(t, t, 1.0f, 0.5f);
            } else {
                // White to red
                float t = (normalized - 0.5f) * 2.0f;
                color = glm::vec4(1.0f, 1.0f - t, 1.0f - t, 0.5f);
            }
            
            renderer->drawRect(glm::vec2(x, y), glm::vec2(1.0f, 1.0f), color);
        }
    }
}

void PhysicsDebugRenderer::renderActiveOverlay() {
    // Get visible area
    WorldRect visibleArea = renderer->getVisibleWorldRect();
    
    for (int y = visibleArea.y; y < visibleArea.y + visibleArea.height; y++) {
        for (int x = visibleArea.x; x < visibleArea.x + visibleArea.width; x++) {
            if (!chunkManager->isValidCoord({x, y})) continue;
            
            WorldCoord coord{x, y};
            ChunkCoord chunkCoord = chunkManager->worldToChunkCoord(coord);
            LocalCoord localCoord = chunkManager->worldToLocalCoord(coord);
            
            Chunk* chunk = chunkManager->getChunk(chunkCoord);
            if (!chunk) continue;
            
            if (chunk->isCellActive(localCoord.x, localCoord.y)) {
                renderer->drawRect(glm::vec2(x, y), glm::vec2(1.0f, 1.0f), glm::vec4(0.0f, 1.0f, 0.0f, 0.3f));
            }
        }
    }
}
```

## Conclusion

The physics system forms the core of the Astral engine, providing realistic simulations of various materials and their interactions. Key features include:

1. **Cellular Automaton Approach**: Each cell follows simple rules, but the emergent behavior creates complex simulations
2. **Material System**: Flexible material properties enable a wide variety of behaviors
3. **Temperature and Reactions**: Materials can change state or react based on temperature and proximity
4. **Optimized Performance**: Chunk-based updates, activity tracking, and parallelization
5. **Rigid Body Integration**: Combining cellular simulation with traditional physics for larger objects
6. **Debug Tools**: Visualizing the physics simulation for development and debugging

The physics system is designed to be extended with new materials, behaviors, and interactions, allowing for a rich and dynamic game world.