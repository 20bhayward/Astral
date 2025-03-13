#pragma once

#include <vector>
#include <map>
#include <functional>
#include <random>
#include "astral/physics/ChunkManager.h"
#include "astral/physics/Material.h"

namespace astral {

// Forward declarations
class MaterialRegistry;
class CellProcessor;

/**
 * Handles cellular automaton-based physics simulation.
 */
class CellularPhysics {
private:
    MaterialRegistry* materialRegistry;
    ChunkManager* chunkManager;
    CellProcessor* cellProcessor;
    std::vector<std::vector<bool>> updated; // Tracks which cells updated this frame
    
    // World dimensions for update tracking
    int worldWidth;
    int worldHeight;
    
    // Random number generator
    std::mt19937 random;
    
    // Function map for different material updates
    std::map<MaterialType, std::function<void(CellularPhysics*, int, int, float)>> updateFunctions;
    
    // Helper methods
    bool isValidPosition(int x, int y) const;
    Cell& getCell(int x, int y);
    const Cell& getCell(int x, int y) const;
    MaterialProperties getMaterialProperties(int x, int y) const;
    
    bool canMove(int x, int y, int newX, int newY);
    void swapCells(int x, int y, int newX, int newY);
    void moveCell(int x, int y, int newX, int newY);
    void applyForce(int x, int y, const glm::vec2& force);
    void processMaterialInteraction(int x1, int y1, int x2, int y2, float deltaTime);
    void applyTemperature(int x, int y, float deltaTime);
    
    // Initialization methods
    void initialize();
    void setupUpdateFunctions();
    void resetUpdateTracker();
    
    // Special effects processing
    void processActiveEffects(float deltaTime);
    bool isCellUpdated(int x, int y) const;
    void visualizePropertyField(const std::string& propertyName);
    
public:
    CellularPhysics(MaterialRegistry* registry, ChunkManager* chunkManager);
    ~CellularPhysics();
    
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
    
    // Special effects and interactions
    void createExplosion(int x, int y, float radius, float power);
    void createHeatSource(int x, int y, float temperature, float radius);
    void applyForceField(int x, int y, const glm::vec2& direction, float strength, float radius);
    
    // Debug methods
    void dumpPerformanceStats() const;
};

} // namespace astral