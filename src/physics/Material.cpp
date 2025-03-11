#include "astral/physics/Material.h"

namespace astral {

MaterialProperties::MaterialProperties()
    : type(MaterialType::EMPTY)
    , name("")
    , color(0.0f, 0.0f, 0.0f, 0.0f)
    , colorVariation(0.0f)
    , emissive(false)
    , emissiveStrength(0.0f)
    , density(0.0f)
    , viscosity(0.0f)
    , friction(0.0f)
    , elasticity(0.0f)
    , dispersion(0.0f)
    , movable(false)
    , flammable(false)
    , flammability(0.0f)
{
}

MaterialProperties::MaterialProperties(MaterialType type, const std::string& name, const glm::vec4& color)
    : type(type)
    , name(name)
    , color(color)
    , colorVariation(0.0f)
    , emissive(false)
    , emissiveStrength(0.0f)
    , density(1.0f)
    , viscosity(0.0f)
    , friction(0.5f)
    , elasticity(0.0f)
    , dispersion(0.0f)
    , movable(type != MaterialType::SOLID)
    , flammable(false)
    , flammability(0.0f)
{
}

} // namespace astral