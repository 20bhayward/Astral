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
    EMPTY,          // Air or void
    
    // Solid materials (static and sturdy)
    SOLID,          // Generic solid materials like stone
    METAL,          // Metals (conducts electricity, high density)
    WOOD,           // Wooden materials (flammable solids)
    GLASS,          // Glass-like materials (breakable, transparent)
    CRYSTAL,        // Crystals (can grow, special properties)
    
    // Granular materials (can pile up or flow)
    POWDER,         // Generic powders like sand
    SOIL,           // Soil, dirt, mud (can support plant growth)
    GRANULAR,       // Specialized granular materials (sugar, salt, etc.)
    
    // Fluid materials
    LIQUID,         // Generic liquids like water
    OIL,            // Oil-based liquids (flammable, float on water)
    ACID,           // Acids (corrode other materials)
    LAVA,           // Molten materials (hot, ignites, damages)
    
    // Gaseous materials
    GAS,            // Generic gas
    STEAM,          // Water vapor (condenses to water)
    SMOKE,          // Smoke particles (from burning)
    
    // Energy and special
    FIRE,           // Fire and flames
    PLASMA,         // Extremely hot ionized material
    ORGANIC,        // Living or organic materials
    SPECIAL         // Special materials with unique properties
};

// Simple category for easy material grouping
enum class MaterialCategory {
    NONE,        // Uncategorized
    STONE,       // Stone, rock, concrete, etc.
    METAL,       // Metals
    DIRT,        // Dirt, mud, soil
    SAND,        // Sand and similar granular materials
    WOOD,        // Wood and plant materials
    WATER,       // Water and similar liquids
    OIL,         // Oil-based liquids
    LAVA,        // Lava and molten materials
    GAS,         // Gases 
    FIRE,        // Fire and heat sources
    SPECIAL      // Special materials
};

// Simplified struct for defining reactions between materials
struct MaterialReaction {
    MaterialID reactantMaterial;   // Material that causes reaction
    MaterialID resultMaterial;     // What this material changes into
    MaterialID byproduct;          // Optional byproduct (e.g., water + lava = stone + steam)
    float probability;             // Chance of reaction occurring (0-1)
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
    MaterialCategory category;       // Category for grouping similar materials
    
    // Visual properties
    glm::vec4 color;
    float colorVariation;
    bool emissive;
    float emissiveStrength;
    
    // Core physical properties
    float density;        // Determines sinking/floating (higher density materials sink in lower density ones)
    float viscosity;      // How thick/sticky the material is (affects flow rate)
    float dispersion;     // How much it spreads horizontally
    float friction;       // Surface friction coefficient
    
    // Basic behavior flags
    bool movable;         // Whether the material can move or is fixed in place
    bool flammable;       // Can this material catch fire?
    float flammability;   // How easily it ignites (0-1)
    
    // Temperature properties
    float meltingPoint;   // Temperature at which solid becomes liquid
    float freezingPoint;  // Temperature at which liquid becomes solid
    float boilingPoint;   // Temperature at which liquid becomes gas
    float ignitionPoint;  // Temperature at which it catches fire
    
    // Simple effects
    float lifetime;       // For temporary materials (like fire, smoke)
    float burnRate;       // How quickly it burns away once ignited
    
    // Simple flags (bit field for efficient storage)
    uint32_t flags;
    enum Flags {
        CORROSIVE      = 1 << 0,  // Damages/dissolves materials
        EXPLOSIVE      = 1 << 1,  // Can explode
        CONDUCTIVE     = 1 << 2,  // Conducts electricity
        HOT            = 1 << 3,  // Naturally hot (like lava)
        STICKY         = 1 << 4,  // Sticks to other materials
        DISAPPEARS     = 1 << 5,  // Disappears over time
        GROWS          = 1 << 6,  // Can grow/expand
        MAGIC          = 1 << 7,  // Has special magical properties
        BREAKABLE      = 1 << 8   // Can break easily
    };
    
    // Reactions and state changes
    std::vector<MaterialReaction> reactions;
    std::vector<MaterialStateChange> stateChanges;
    
    // Constructors
    MaterialProperties();
    MaterialProperties(MaterialType type, const std::string& name, const glm::vec4& color);
    
    // Flag helpers
    bool hasFlag(Flags flag) const { return (flags & flag) != 0; }
    void setFlag(Flags flag) { flags |= flag; }
    void clearFlag(Flags flag) { flags &= ~flag; }
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
    MaterialID getOilFireID() const;
};

// CellProcessor moved to its own header file

} // namespace astral