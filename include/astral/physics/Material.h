#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <random>
#include <glm/glm.hpp>
#include "astral/physics/Cell.h"

namespace astral {

// Forward declaration
using MaterialID = uint16_t;

enum class MaterialType {
    EMPTY,
    SOLID,
    POWDER,
    LIQUID,
    GAS,
    FIRE,
    SPECIAL
};

// Struct to define reactions between materials
struct MaterialReaction {
    MaterialID reactantMaterial;
    MaterialID resultMaterial;
    float probability;
    float energyRelease;
};

// Struct to define state changes for materials
struct MaterialStateChange {
    MaterialID targetMaterial;
    float temperatureThreshold;
    float probability;
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
    float density;
    float viscosity;
    float friction;
    float elasticity;
    float dispersion;
    
    // Thermal properties
    float specificHeat;
    float thermalConductivity;
    float meltingPoint;
    float freezingPoint;
    float boilingPoint;
    float ignitionPoint;
    
    // Simulation behavior
    bool movable;
    bool flammable;
    float flammability;
    float burnRate;
    float lifetime;
    bool conductive;
    float conductivity;
    bool dissolves;
    float dissolutionRate;
    
    // Reactions and state changes
    std::vector<MaterialReaction> reactions;
    std::vector<MaterialStateChange> stateChanges;
    
    // Constructors
    MaterialProperties();
    MaterialProperties(MaterialType type, const std::string& name, const glm::vec4& color);
};

/**
 * Registry for material definitions
 */
class MaterialRegistry {
private:
    std::unordered_map<MaterialID, MaterialProperties> materials;
    std::unordered_map<std::string, MaterialID> nameToID;
    MaterialID nextID = 1; // 0 is reserved for EMPTY/AIR
    
public:
    MaterialRegistry();
    ~MaterialRegistry() = default;
    
    // Register a new material
    MaterialID registerMaterial(const MaterialProperties& properties);
    
    // Register basic built-in materials
    void registerBasicMaterials();
    
    // Get material properties
    MaterialProperties getMaterial(MaterialID id) const;
    
    // Get material ID from name
    MaterialID getIDFromName(const std::string& name) const;
    bool hasMaterialName(const std::string& name) const;
    
    // Utility functions for common materials
    MaterialID getDefaultMaterialID() const { return 0; } // Air
    MaterialID getSandID() const;
    MaterialID getWaterID() const;
    MaterialID getStoneID() const;
    MaterialID getOilID() const;
    MaterialID getLavaID() const;
    MaterialID getFireID() const;
    MaterialID getSteamID() const;
    MaterialID getSmokeID() const;
    MaterialID getWoodID() const;
};

/**
 * Helper class to initialize and process cells
 */
class CellProcessor {
private:
    MaterialRegistry* materialRegistry;
    std::mt19937 random;
    
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
    float getRandomFloat(float min, float max);
    int getRandomInt(int min, int max);
    bool rollProbability(float chance);
};

} // namespace astral