#include "astral/physics/Material.h"
#include <iostream>
#include <chrono>

namespace astral {

// Forward declaration for IDs
// Instead of hardcoding IDs, we'll use these variables to store the IDs
// after registration
static MaterialID MATERIAL_ID_AIR = 0; // Air is always 0
static MaterialID MATERIAL_ID_STONE;
static MaterialID MATERIAL_ID_SAND;
static MaterialID MATERIAL_ID_WATER;
static MaterialID MATERIAL_ID_OIL;
static MaterialID MATERIAL_ID_LAVA;
static MaterialID MATERIAL_ID_FIRE;
static MaterialID MATERIAL_ID_OIL_FIRE;
static MaterialID MATERIAL_ID_STEAM;
static MaterialID MATERIAL_ID_SMOKE;
static MaterialID MATERIAL_ID_WOOD;

MaterialProperties::MaterialProperties()
    : type(MaterialType::EMPTY)
    , name("")
    , category(MaterialCategory::NONE)
    , color(0.0f, 0.0f, 0.0f, 0.0f)
    , colorVariation(0.0f)
    , emissive(false)
    , emissiveStrength(0.0f)
    , density(0.0f)
    , viscosity(0.0f)
    , dispersion(0.0f)
    , friction(0.0f)
    , movable(false)
    , flammable(false)
    , flammability(0.0f)
    , meltingPoint(0.0f)
    , freezingPoint(0.0f)
    , boilingPoint(0.0f)
    , ignitionPoint(0.0f)
    , lifetime(0.0f)
    , burnRate(0.0f)
    , flags(0)
{
}

MaterialProperties::MaterialProperties(MaterialType type, const std::string& name, const glm::vec4& color)
    : type(type)
    , name(name)
    , category(MaterialCategory::NONE)
    , color(color)
    , colorVariation(0.0f)
    , emissive(false)
    , emissiveStrength(0.0f)
    , density(1.0f)
    , viscosity(0.0f)
    , dispersion(0.0f)
    , friction(0.5f)
    , movable(type != MaterialType::SOLID)
    , flammable(false)
    , flammability(0.0f)
    , meltingPoint(0.0f)
    , freezingPoint(0.0f)
    , boilingPoint(0.0f)
    , ignitionPoint(0.0f)
    , lifetime(0.0f)
    , burnRate(0.0f)
    , flags(0)
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
    stone.category = MaterialCategory::STONE;
    stone.density = 2600.0f;
    stone.movable = false;
    MATERIAL_ID_STONE = registerMaterial(stone);
    
    // Additional stone variants
    
    // Granite - dense, hard stone
    MaterialProperties granite(MaterialType::SOLID, "Granite", glm::vec4(0.65f, 0.45f, 0.45f, 1.0f));
    granite.category = MaterialCategory::STONE;
    granite.density = 2800.0f;     // Denser than regular stone
    granite.movable = false;
    granite.meltingPoint = 1200.0f; // Higher melting point
    registerMaterial(granite);
    
    // Marble - decorative stone
    MaterialProperties marble(MaterialType::SOLID, "Marble", glm::vec4(0.9f, 0.9f, 0.92f, 1.0f));
    marble.category = MaterialCategory::STONE;
    marble.density = 2700.0f;
    marble.movable = false;
    marble.colorVariation = 0.1f;  // Slight variations in color for veining
    registerMaterial(marble);
    
    // Obsidian - volcanic glass
    MaterialProperties obsidian(MaterialType::SOLID, "Obsidian", glm::vec4(0.1f, 0.05f, 0.15f, 1.0f));
    obsidian.category = MaterialCategory::STONE;
    obsidian.density = 2550.0f;
    obsidian.movable = false;
    obsidian.emissive = true;      // Slight glow for visual effect
    obsidian.emissiveStrength = 0.1f;
    obsidian.setFlag(MaterialProperties::Flags::BREAKABLE);  // More breakable than other stone
    registerMaterial(obsidian);
    
    // Sand - YELLOW/BROWN
    MaterialProperties sand(MaterialType::POWDER, "Sand", glm::vec4(0.76f, 0.7f, 0.5f, 1.0f));
    sand.density = 1600.0f;
    sand.movable = true;
    sand.dispersion = 10.0f; // Much higher dispersion to force it to spread out horizontally
    MATERIAL_ID_SAND = registerMaterial(sand);
    
    // Water - BLUE
    MaterialProperties water(MaterialType::LIQUID, "Water", glm::vec4(0.0f, 0.4f, 0.8f, 0.8f));
    water.density = 1000.0f;
    water.viscosity = 0.0f;   // Zero viscosity 
    water.dispersion = 10.0f; // Very high dispersion to force horizontal spread
    water.movable = true;
    
    // Add thermal properties
    water.boilingPoint = 100.0f;  // Water boils at 100C
    water.freezingPoint = 0.0f;   // Water freezes at 0C
    // Water has good thermal properties
    
    // We'll add the state changes after we've registered all materials
    MATERIAL_ID_WATER = registerMaterial(water);
    
    // Oil - DARK BROWN/BLACK
    MaterialProperties oil(MaterialType::LIQUID, "Oil", glm::vec4(0.25f, 0.15f, 0.0f, 0.8f));
    oil.density = 850.0f;  // Less dense than water so it floats on top
    oil.viscosity = 0.5f;  // More viscous than water but not too sluggish for better gameplay
    oil.dispersion = 5.0f; // Improved dispersion - flows better for gameplay
    oil.flammable = true;
    oil.flammability = 0.98f;  // Extremely high flammability
    oil.ignitionPoint = 180.0f;  // Very low ignition point - ignites extremely easily
    oil.burnRate = 2.5f;        // Burns VERY quickly - much faster than wood for contrast
    oil.movable = true;
    // Oil has good thermal properties
    
    // We'll add reactions later after registering all materials
    MATERIAL_ID_OIL = registerMaterial(oil);
    
    // Lava - BRIGHT RED/ORANGE 
    MaterialProperties lava(MaterialType::LAVA, "Lava", glm::vec4(1.0f, 0.3f, 0.0f, 1.0f));
    lava.category = MaterialCategory::LAVA;
    
    // Enhanced lava properties for better gameplay behavior
    lava.density = 2800.0f;       // Higher density to ensure lava can displace most materials
    lava.dispersion = 4.0f;       // Lower dispersion for thicker flow
    lava.viscosity = 0.6f;        // Higher viscosity for slower, more realistic flow
    lava.emissive = true;
    lava.emissiveStrength = 0.8f; // Brighter glow
    lava.movable = true;
    
    // Add thermal properties
    lava.meltingPoint = 800.0f;   // Melting point of rock
    lava.ignitionPoint = 0.0f;    // Always hot enough to ignite things
    
    // Set special flags
    lava.setFlag(MaterialProperties::Flags::HOT);
    lava.setFlag(MaterialProperties::Flags::CORROSIVE);
    
    MATERIAL_ID_LAVA = registerMaterial(lava);
    
    // Create several lava variants with different properties
    
    // Blue Lava (hotter, more fluid) - inspired by Kawah Ijen volcano in Indonesia
    MaterialProperties blueLava(MaterialType::LAVA, "BlueLava", glm::vec4(0.1f, 0.4f, 1.0f, 1.0f));
    blueLava.category = MaterialCategory::LAVA;
    blueLava.density = 2500.0f;       // Slightly less dense
    blueLava.dispersion = 6.0f;       // Higher dispersion for more fluid flow
    blueLava.viscosity = 0.4f;        // Less viscous, flows faster
    blueLava.emissive = true;
    blueLava.emissiveStrength = 1.0f; // Brighter glow
    blueLava.movable = true;
    blueLava.meltingPoint = 900.0f;   // Higher melting point
    blueLava.setFlag(MaterialProperties::Flags::HOT);
    blueLava.setFlag(MaterialProperties::Flags::CORROSIVE);
    registerMaterial(blueLava);
    
    // Obsidian Lava (cooler, more viscous) - thicker and darker
    MaterialProperties obsidianLava(MaterialType::LAVA, "ObsidianLava", glm::vec4(0.2f, 0.0f, 0.2f, 1.0f));
    obsidianLava.category = MaterialCategory::LAVA;
    obsidianLava.density = 3000.0f;       // Denser
    obsidianLava.dispersion = 2.0f;       // Lower dispersion for slower flow
    obsidianLava.viscosity = 0.8f;        // Much more viscous, flows slowly
    obsidianLava.emissive = true;
    obsidianLava.emissiveStrength = 0.6f; // Less bright
    obsidianLava.movable = true;
    obsidianLava.meltingPoint = 700.0f;   // Lower melting point
    obsidianLava.setFlag(MaterialProperties::Flags::HOT);
    obsidianLava.setFlag(MaterialProperties::Flags::CORROSIVE);
    registerMaterial(obsidianLava);
    
    // Molten Metal (silver color, very hot) - for metallic melts
    MaterialProperties moltenMetal(MaterialType::LAVA, "MoltenMetal", glm::vec4(0.8f, 0.8f, 0.9f, 1.0f));
    moltenMetal.category = MaterialCategory::METAL;
    moltenMetal.density = 3500.0f;       // Very dense
    moltenMetal.dispersion = 3.0f;       // Moderate dispersion
    moltenMetal.viscosity = 0.5f;        // Moderate viscosity
    moltenMetal.emissive = true;
    moltenMetal.emissiveStrength = 0.9f;  // Bright glow
    moltenMetal.movable = true;
    moltenMetal.meltingPoint = 1200.0f;   // High melting point
    moltenMetal.setFlag(MaterialProperties::Flags::HOT);
    moltenMetal.setFlag(MaterialProperties::Flags::CONDUCTIVE);
    registerMaterial(moltenMetal);
    
    // Fire - ORANGE/YELLOW
    MaterialProperties fire(MaterialType::FIRE, "Fire", glm::vec4(1.0f, 0.6f, 0.1f, 0.9f));
    fire.density = 0.25f;  // Slightly reduced to make it rise faster
    fire.emissive = true;
    fire.emissiveStrength = 0.8f;
    fire.movable = true;  // Fire can move upward
    // Fire has excellent heat transfer properties
    
    // Set lifetime for fire
    fire.lifetime = 30;  // Longer lifetime for standard fire
    fire.burnRate = 1.0f;  // Standard burn rate
    
    MATERIAL_ID_FIRE = registerMaterial(fire);
    
    // Oil Fire - Deeper orange/yellow with hints of blue
    MaterialProperties oilFire(MaterialType::FIRE, "OilFire", glm::vec4(1.0f, 0.4f, 0.1f, 0.9f));
    oilFire.density = 0.3f;  // Slightly denser than normal fire (oil fires are heavier)
    oilFire.emissive = true;
    oilFire.emissiveStrength = 1.0f;  // Brighter glow
    oilFire.movable = true;
    // Oil fire has excellent heat transfer properties
    
    // Set longer lifetime for oil fire
    oilFire.lifetime = 60;  // Oil fire burns twice as long
    oilFire.burnRate = 0.6f;  // Burns more slowly than standard fire
    
    MATERIAL_ID_OIL_FIRE = registerMaterial(oilFire);
    
    // Steam - WHITE/LIGHT BLUE
    MaterialProperties steam(MaterialType::GAS, "Steam", glm::vec4(0.9f, 0.9f, 1.0f, 0.3f));
    steam.density = 0.6f;
    steam.viscosity = 0.1f;     // Low viscosity for fluid movement
    steam.dispersion = 8.0f;    // High dispersion for wide spreading
    steam.movable = true;
    // Steam conducts heat well
    steam.lifetime = 30;  // Steam lasts a while before dissipating
    
    MATERIAL_ID_STEAM = registerMaterial(steam);
    
    // Smoke - DARK GRAY
    MaterialProperties smoke(MaterialType::GAS, "Smoke", glm::vec4(0.2f, 0.2f, 0.2f, 0.7f));
    smoke.density = 0.4f;       // Reduced density to make it rise faster
    smoke.viscosity = 0.15f;    // Less viscous for more fluid movement
    smoke.dispersion = 7.0f;    // Higher dispersion for better spreading
    smoke.movable = true;
    smoke.lifetime = 50;        // Smoke lasts longer before dissipating
    // Smoke has poor heat conductivity
    
    // Create smoke variants that can be produced by different types of fires
    // (This is visual only - we don't actually change the smoke's material ID)
    // This is handled via the color variation property
    smoke.colorVariation = 0.15f;  // Allow smoke to vary in color
    
    MATERIAL_ID_SMOKE = registerMaterial(smoke);
    
    // Wood - BROWN
    MaterialProperties wood(MaterialType::SOLID, "Wood", glm::vec4(0.6f, 0.4f, 0.2f, 1.0f));
    wood.density = 700.0f;
    wood.flammable = true;
    wood.flammability = 0.5f;       // Increased flammability
    wood.ignitionPoint = 280.0f;    // Lower ignition point
    wood.burnRate = 0.7f;           // Slower burn rate - wood burns longer than before
    // Wood has low thermal conductivity
    
    MATERIAL_ID_WOOD = registerMaterial(wood);
    
    // Now add state changes and reactions using the registered material IDs
    
    // WATER - add boiling state change and reaction with lava
    MaterialProperties& waterProps = materials[MATERIAL_ID_WATER];
    
    // Boiling to steam at high temperature
    MaterialStateChange steamChange;
    steamChange.targetMaterial = MATERIAL_ID_STEAM;
    steamChange.temperatureThreshold = 100.0f;  // Boiling point
    steamChange.probability = 0.5f;  // Moderate probability
    waterProps.stateChanges.push_back(steamChange);
    
    // Add water-lava reaction (creates steam and stone)
    MaterialReaction lavaReaction;
    lavaReaction.reactantMaterial = MATERIAL_ID_LAVA;
    lavaReaction.resultMaterial = MATERIAL_ID_STEAM;   // Water turns to steam when it contacts lava
    lavaReaction.byproduct = MATERIAL_ID_STONE;        // Lava turns to stone
    lavaReaction.probability = 0.85f;                  // High probability
    waterProps.reactions.push_back(lavaReaction);
    
    // LAVA - add cooling to stone and reactions with water
    MaterialProperties& lavaProps = materials[MATERIAL_ID_LAVA];
    
    // Cooling to stone over time
    MaterialStateChange stoneChange;
    stoneChange.targetMaterial = MATERIAL_ID_STONE;
    stoneChange.temperatureThreshold = -800.0f;  // Negative for cooling threshold
    stoneChange.probability = 0.15f;  // Low probability so it cools slowly
    lavaProps.stateChanges.push_back(stoneChange);
    
    // Add lava-water reaction (creates stone and steam)
    MaterialReaction waterReaction;
    waterReaction.reactantMaterial = MATERIAL_ID_WATER;
    waterReaction.resultMaterial = MATERIAL_ID_STONE;  // Lava turns to stone when it contacts water
    waterReaction.byproduct = MATERIAL_ID_STEAM;       // Water turns to steam
    waterReaction.probability = 0.8f;                  // High probability for predictable gameplay
    lavaProps.reactions.push_back(waterReaction);
    
    // FIRE - add smoke transformation
    MaterialProperties& fireProps = materials[MATERIAL_ID_FIRE];
    MaterialStateChange smokeChange;
    smokeChange.targetMaterial = MATERIAL_ID_SMOKE;
    smokeChange.temperatureThreshold = -100.0f;  // When fire cools below 100
    smokeChange.probability = 0.6f;  // Slightly reduced probability for more consistent fire
    fireProps.stateChanges.push_back(smokeChange);
    
    // OIL FIRE - add smoke transformation
    MaterialProperties& oilFireProps = materials[MATERIAL_ID_OIL_FIRE];
    MaterialStateChange oilSmokeChange;
    oilSmokeChange.targetMaterial = MATERIAL_ID_SMOKE;
    oilSmokeChange.temperatureThreshold = -120.0f;  // Oil fire needs to cool more to turn to smoke
    oilSmokeChange.probability = 0.4f;  // Lower probability = longer lasting
    oilFireProps.stateChanges.push_back(oilSmokeChange);
    
    // STEAM - add condensation
    MaterialProperties& steamProps = materials[MATERIAL_ID_STEAM];
    MaterialStateChange waterChange;
    waterChange.targetMaterial = MATERIAL_ID_WATER;
    waterChange.temperatureThreshold = -90.0f;  // When steam cools below 90
    waterChange.probability = 0.2f;  // Low probability so it doesn't condense too fast
    steamProps.stateChanges.push_back(waterChange);
    
    // SMOKE - add dissipation
    MaterialProperties& smokeProps = materials[MATERIAL_ID_SMOKE];
    MaterialStateChange smokeDissipate;
    smokeDissipate.targetMaterial = MATERIAL_ID_AIR;
    smokeDissipate.temperatureThreshold = -50.0f;  // When smoke cools significantly
    smokeDissipate.probability = 0.1f;  // Low probability so it dissipates slowly
    smokeProps.stateChanges.push_back(smokeDissipate);
    
    // OIL - add ignition and reactions
    MaterialProperties& oilProps = materials[MATERIAL_ID_OIL];
    
    // Oil can turn to OIL FIRE when hot enough
    MaterialStateChange oilFireChange;
    oilFireChange.targetMaterial = MATERIAL_ID_OIL_FIRE;
    oilFireChange.temperatureThreshold = 220.0f;  // Ignition point
    oilFireChange.probability = 0.7f;   // Higher probability of ignition
    oilProps.stateChanges.push_back(oilFireChange);
    
    // Add reaction for oil being near fire - can catch fire from proximity
    MaterialReaction fireReaction;
    fireReaction.reactantMaterial = MATERIAL_ID_FIRE;
    fireReaction.resultMaterial = MATERIAL_ID_OIL_FIRE;
    fireReaction.probability = 0.6f;    // Good chance of ignition when near fire
    // Reaction releases significant energy
    oilProps.reactions.push_back(fireReaction);
    
    // WOOD - add ignition and reactions
    MaterialProperties& woodProps = materials[MATERIAL_ID_WOOD];
    
    // Wood can turn to fire when hot enough
    MaterialStateChange woodFireChange;
    woodFireChange.targetMaterial = MATERIAL_ID_FIRE;
    woodFireChange.temperatureThreshold = 280.0f;  // Ignition point
    woodFireChange.probability = 0.4f;   // Moderate probability of ignition
    woodProps.stateChanges.push_back(woodFireChange);
    
    // Add reaction for wood being near fire - can catch fire from proximity
    MaterialReaction woodFireReaction;
    woodFireReaction.reactantMaterial = MATERIAL_ID_FIRE;
    woodFireReaction.resultMaterial = MATERIAL_ID_FIRE;
    woodFireReaction.probability = 0.3f;     // Chance of ignition when near fire
    // Reaction releases energy
    woodProps.reactions.push_back(woodFireReaction);
    
    // Add reaction for wood being near oil fire - catches fire faster from oil fire
    MaterialReaction woodOilFireReaction;
    woodOilFireReaction.reactantMaterial = MATERIAL_ID_OIL_FIRE;
    woodOilFireReaction.resultMaterial = MATERIAL_ID_FIRE;
    woodOilFireReaction.probability = 0.5f;     // Higher chance of ignition from oil fire
    // Reaction releases significant energy
    woodProps.reactions.push_back(woodOilFireReaction);
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
    
    // Removed debug output
    
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
    return getIDFromName("Sand");
}

MaterialID MaterialRegistry::getWaterID() const {
    return getIDFromName("Water");
}

MaterialID MaterialRegistry::getStoneID() const {
    return getIDFromName("Stone");
}

MaterialID MaterialRegistry::getOilID() const {
    return getIDFromName("Oil");
}

MaterialID MaterialRegistry::getLavaID() const {
    return getIDFromName("Lava");
}

MaterialID MaterialRegistry::getFireID() const {
    return getIDFromName("Fire");
}

MaterialID MaterialRegistry::getSteamID() const {
    return getIDFromName("Steam");
}

MaterialID MaterialRegistry::getSmokeID() const {
    return getIDFromName("Smoke");
}

MaterialID MaterialRegistry::getWoodID() const {
    return getIDFromName("Wood");
}

MaterialID MaterialRegistry::getOilFireID() const {
    return getIDFromName("OilFire");
}

} // namespace astral