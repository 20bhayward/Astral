#pragma once

#include <random>
#include "astral/physics/Material.h"
#include "astral/physics/Cell.h"

namespace astral {

/**
 * Helper class to initialize and process cells
 */
class CellProcessor {
private:
    MaterialRegistry* materialRegistry;
    mutable std::mt19937 random;
    
    // Helper to get an adjacent cell safely
    Cell* getAdjacentCell(const Cell& cell, int dx, int dy);
    
public:
    CellProcessor(MaterialRegistry* registry);
    ~CellProcessor() = default;
    
    // Cell initialization
    void initializeCellFromMaterial(Cell& cell, MaterialID materialID) const;
    void applyMaterialProperties(Cell& cell, const MaterialProperties& props) const;
    
    // Cell movement
    bool canCellMove(const Cell& cell, const Cell& target) const;
    bool canDisplace(const Cell& mover, const Cell& target) const;
    bool shouldSwapCells(const Cell& cell1, const Cell& cell2) const;
    
    // Cell interactions
    bool canReact(const Cell& cell1, const Cell& cell2) const;
    bool processPotentialReaction(Cell& cell1, Cell& cell2, float deltaTime);
    void processStateChange(Cell& cell, float deltaTime);
    void transferHeat(Cell& sourceCell, Cell& targetCell, float deltaTime) const;
    bool checkStateChangeByTemperature(Cell& cell);
    
    // Cell effects
    void applyVelocity(Cell& cell, const glm::vec2& direction, float speed);
    void applyPressure(Cell& cell, float amount);
    void damageCell(Cell& cell, float amount);
    void igniteCell(Cell& cell);
    void extinguishCell(Cell& cell);
    void freezeCell(Cell& cell);
    void meltCell(Cell& cell);
    void dissolveCell(Cell& cell, float rate);
    
    // Random utilities
    float getRandomFloat(float min, float max) const;
    int getRandomInt(int min, int max) const;
    bool rollProbability(float chance) const;
};

} // namespace astral