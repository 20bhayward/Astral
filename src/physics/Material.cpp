#include "astral/physics/Material.h"
#include <iostream>
#include <chrono>

namespace astral {

// Forward declaration for IDs
static const MaterialID MATERIAL_ID_AIR = 0;
static const MaterialID MATERIAL_ID_STONE = 1;
static const MaterialID MATERIAL_ID_SAND = 2;
static const MaterialID MATERIAL_ID_WATER = 3;
static const MaterialID MATERIAL_ID_OIL = 4;
static const MaterialID MATERIAL_ID_LAVA = 5;
static const MaterialID MATERIAL_ID_FIRE = 6;
static const MaterialID MATERIAL_ID_STEAM = 7;
static const MaterialID MATERIAL_ID_SMOKE = 8;
static const MaterialID MATERIAL_ID_WOOD = 9;

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

// MaterialRegistry implementation
MaterialRegistry::MaterialRegistry() : nextID(1) {
    // Always register air/empty as ID 0
    MaterialProperties air;
    air.type = MaterialType::EMPTY;
    air.name = "Air";
    air.color = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);
    materials[MATERIAL_ID_AIR] = air;
    nameToID["Air"] = MATERIAL_ID_AIR;
}

void MaterialRegistry::registerBasicMaterials() {
    // Stone - GRAY
    MaterialProperties stone(MaterialType::SOLID, "Stone", glm::vec4(0.5f, 0.5f, 0.5f, 1.0f));
    stone.density = 2600.0f;
    stone.movable = false;
    registerMaterial(stone);
    
    // Sand - YELLOW/BROWN
    MaterialProperties sand(MaterialType::POWDER, "Sand", glm::vec4(0.76f, 0.7f, 0.5f, 1.0f));
    sand.density = 1600.0f;
    sand.movable = true;
    sand.dispersion = 15.0f; // Much higher dispersion to force it to spread out horizontally
    registerMaterial(sand);
    
    // Water - BLUE
    MaterialProperties water(MaterialType::LIQUID, "Water", glm::vec4(0.0f, 0.4f, 0.8f, 0.8f));
    water.density = 1000.0f;
    water.viscosity = 0.0f;   // Zero viscosity 
    water.dispersion = 10.0f; // Very high dispersion to force horizontal spread
    water.movable = true;
    
    // Add thermal properties
    water.boilingPoint = 100.0f;  // Water boils at 100C
    water.freezingPoint = 0.0f;   // Water freezes at 0C
    water.thermalConductivity = 0.6f;  // Moderate thermal conductivity
    water.specificHeat = 4.18f;  // High specific heat
    
    // CRITICAL: Add state changes for boiling!
    MaterialStateChange steamChange;
    steamChange.targetMaterial = 7;  // Steam ID
    steamChange.temperatureThreshold = 100.0f;  // Boiling point
    steamChange.probability = 0.5f;  // Moderate probability
    water.stateChanges.push_back(steamChange);
    registerMaterial(water);
    
    // Oil - DARK BROWN/BLACK
    MaterialProperties oil(MaterialType::LIQUID, "Oil", glm::vec4(0.25f, 0.15f, 0.0f, 0.8f));
    oil.density = 850.0f;  // Less dense than water so it floats on top
    oil.viscosity = 0.7f;  // More viscous (thicker) than water
    oil.dispersion = 2.0f; // Flows less easily than water
    oil.flammable = true;
    oil.flammability = 0.8f;
    oil.ignitionPoint = 250.0f;  // Ignites easily
    oil.burnRate = 1.2f;        // Burns quickly
    oil.movable = true;
    oil.thermalConductivity = 0.2f;
    
    // Oil can turn to fire when hot enough
    MaterialStateChange fireChange;
    fireChange.targetMaterial = 6;  // Fire ID
    fireChange.temperatureThreshold = 250.0f;  // Ignition point
    fireChange.probability = 0.5f;
    oil.stateChanges.push_back(fireChange);
    registerMaterial(oil);
    
    // Lava - BRIGHT RED/ORANGE 
    MaterialProperties lava(MaterialType::LIQUID, "Lava", glm::vec4(1.0f, 0.3f, 0.0f, 1.0f));
    // Fix critical properties
    lava.density = 2000.0f;  
    lava.dispersion = 5.0f;  
    lava.viscosity = 0.2f;   
    lava.emissive = true;
    lava.emissiveStrength = 0.7f;
    lava.movable = true;
    
    // Add thermal properties
    lava.meltingPoint = 800.0f;  // Melting point of rock
    lava.thermalConductivity = 0.5f;
    
    // Add state change for cooling to stone
    MaterialStateChange stoneChange;
    stoneChange.targetMaterial = 1;  // Stone ID
    stoneChange.temperatureThreshold = -800.0f;  // Negative for cooling threshold
    stoneChange.probability = 0.2f;  // Low probability so it cools slowly
    lava.stateChanges.push_back(stoneChange);
    
    // Lava ignites things nearby
    lava.ignitionPoint = 0.0f;  // Always hot enough to ignite things
    registerMaterial(lava);
    
    // Fire - ORANGE/YELLOW
    MaterialProperties fire(MaterialType::FIRE, "Fire", glm::vec4(1.0f, 0.6f, 0.1f, 0.9f));
    fire.density = 0.3f;
    fire.emissive = true;
    fire.emissiveStrength = 0.8f;
    fire.movable = true;  // Fire can move upward
    fire.thermalConductivity = 0.8f;  // High thermal conductivity
    
    // Set lifetime for fire
    fire.lifetime = 20;  // Fire burns out after some time
    
    // Add state change from fire to smoke
    MaterialStateChange smokeChange;
    smokeChange.targetMaterial = 8;  // Smoke ID
    smokeChange.temperatureThreshold = -100.0f;  // When fire cools below 100
    smokeChange.probability = 0.8f;  // High probability
    fire.stateChanges.push_back(smokeChange);
    registerMaterial(fire);
    
    // Steam - WHITE/LIGHT BLUE
    MaterialProperties steam(MaterialType::GAS, "Steam", glm::vec4(0.9f, 0.9f, 1.0f, 0.3f));
    steam.density = 0.6f;
    steam.viscosity = 0.1f;     // Low viscosity for fluid movement
    steam.dispersion = 8.0f;    // High dispersion for wide spreading
    steam.movable = true;
    steam.thermalConductivity = 0.4f;
    steam.lifetime = 30;  // Steam lasts a while before dissipating
    
    // Steam condenses back to water when it cools
    MaterialStateChange waterChange;
    waterChange.targetMaterial = 3;  // Water ID
    waterChange.temperatureThreshold = -90.0f;  // When steam cools below 90
    waterChange.probability = 0.2f;  // Low probability so it doesn't condense too fast
    steam.stateChanges.push_back(waterChange);
    registerMaterial(steam);
    
    // Smoke - DARK GRAY
    MaterialProperties smoke(MaterialType::GAS, "Smoke", glm::vec4(0.2f, 0.2f, 0.2f, 0.7f));
    smoke.density = 0.5f;       // Slightly denser than steam
    smoke.viscosity = 0.2f;     // Slightly more viscous than steam
    smoke.dispersion = 6.0f;    // Still high dispersion but less than steam
    smoke.movable = true;
    smoke.lifetime = 40;        // Smoke lasts longer than steam before dissipating
    smoke.thermalConductivity = 0.2f;
    registerMaterial(smoke);
    
    // Wood - BROWN
    MaterialProperties wood(MaterialType::SOLID, "Wood", glm::vec4(0.6f, 0.4f, 0.2f, 1.0f));
    wood.density = 700.0f;
    wood.flammable = true;
    wood.flammability = 0.4f;
    wood.ignitionPoint = 300.0f;  // Wood ignites at 300 degrees
    wood.burnRate = 0.8f;         // Burns relatively quickly
    wood.thermalConductivity = 0.2f;  // Low thermal conductivity
    registerMaterial(wood);
}

MaterialID MaterialRegistry::registerMaterial(const MaterialProperties& properties) {
    // Check if a material with this name already exists
    if (nameToID.find(properties.name) != nameToID.end()) {
        return nameToID[properties.name];
    }
    
    // Register the material with the next available ID
    MaterialID id = nextID++;
    materials[id] = properties;
    nameToID[properties.name] = id;
    
    // For debugging
    std::cout << "Registered material: " << properties.name << " with ID " << id << std::endl;
    
    return id;
}

MaterialProperties MaterialRegistry::getMaterial(MaterialID id) const {
    if (materials.find(id) != materials.end()) {
        return materials.at(id);
    }
    
    // Return air/empty if not found
    return materials.at(MATERIAL_ID_AIR);
}

MaterialID MaterialRegistry::getIDFromName(const std::string& name) const {
    if (nameToID.find(name) != nameToID.end()) {
        return nameToID.at(name);
    }
    
    // Return air/empty if not found
    return MATERIAL_ID_AIR;
}

bool MaterialRegistry::hasMaterialName(const std::string& name) const {
    return nameToID.find(name) != nameToID.end();
}

MaterialID MaterialRegistry::getSandID() const {
    return MATERIAL_ID_SAND;
}

MaterialID MaterialRegistry::getWaterID() const {
    return MATERIAL_ID_WATER;
}

MaterialID MaterialRegistry::getStoneID() const {
    return MATERIAL_ID_STONE;
}

MaterialID MaterialRegistry::getOilID() const {
    return MATERIAL_ID_OIL;
}

MaterialID MaterialRegistry::getLavaID() const {
    return MATERIAL_ID_LAVA;
}

MaterialID MaterialRegistry::getFireID() const {
    return MATERIAL_ID_FIRE;
}

MaterialID MaterialRegistry::getSteamID() const {
    return MATERIAL_ID_STEAM;
}

MaterialID MaterialRegistry::getSmokeID() const {
    return MATERIAL_ID_SMOKE;
}

MaterialID MaterialRegistry::getWoodID() const {
    return MATERIAL_ID_WOOD;
}

} // namespace astral