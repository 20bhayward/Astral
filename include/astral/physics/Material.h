#pragma once

#include <string>
#include <vector>
#include <glm/glm.hpp>

namespace astral {

enum class MaterialType {
    EMPTY,
    SOLID,
    POWDER,
    LIQUID,
    GAS,
    FIRE,
    SPECIAL
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
    
    // Simulation behavior
    bool movable;
    bool flammable;
    float flammability;
    
    // Constructors
    MaterialProperties();
    MaterialProperties(MaterialType type, const std::string& name, const glm::vec4& color);
};

} // namespace astral