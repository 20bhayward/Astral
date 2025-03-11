#pragma once

#include <vector>
#include "astral/physics/ChunkManager.h"

namespace astral {

// Forward declarations
class MaterialRegistry;

/**
 * Handles cellular automaton-based physics simulation.
 */
class CellularPhysics {
private:
    MaterialRegistry* materialRegistry;
    ChunkManager* chunkManager;
    std::vector<std::vector<bool>> updated; // Tracks which cells updated this frame
    
    // World dimensions for update tracking
    int worldWidth;
    int worldHeight;
    
    // Helper methods
    bool canMove(int x, int y, int newX, int newY);
    void swapCells(int x, int y, int newX, int newY);
    void moveCell(int x, int y, int newX, int newY);
    void processMaterialInteraction(int x1, int y1, int x2, int y2);
    void applyTemperature(int x, int y, float deltaTime);
    
public:
    CellularPhysics(MaterialRegistry* registry, ChunkManager* chunkManager);
    ~CellularPhysics() = default;
    
    // Set world dimensions for update tracking
    void setWorldDimensions(int width, int height);
    
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

} // namespace astral