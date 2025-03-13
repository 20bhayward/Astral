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
    registerMaterial(water);
    
    // Oil - DARK BROWN/BLACK
    MaterialProperties oil(MaterialType::LIQUID, "Oil", glm::vec4(0.25f, 0.15f, 0.0f, 0.8f));
    oil.density = 850.0f;  // Less dense than water so it floats on top
    oil.viscosity = 0.7f;  // More viscous (thicker) than water
    oil.dispersion = 2.0f; // Flows less easily than water
    oil.flammable = true;
    oil.flammability = 0.8f;
    oil.movable = true;
    registerMaterial(oil);
    
    // Lava - BRIGHT RED/ORANGE 
    // THIS IS THE CRITICAL BUG: Lava was set as a LIQUID but has other issues
    MaterialProperties lava(MaterialType::LIQUID, "Lava", glm::vec4(1.0f, 0.3f, 0.0f, 1.0f));
    // Fix critical properties
    lava.density = 2000.0f;  
    lava.dispersion = 5.0f;  
    lava.viscosity = 0.2f;   
    lava.emissive = true;
    lava.emissiveStrength = 0.7f;
    // CRITICAL FIX: Ensure this is movable
    lava.movable = true;
    registerMaterial(lava);
    
    // Fire - ORANGE/YELLOW
    MaterialProperties fire(MaterialType::FIRE, "Fire", glm::vec4(1.0f, 0.6f, 0.1f, 0.9f));
    fire.density = 0.3f;
    fire.emissive = true;
    fire.emissiveStrength = 0.8f;
    registerMaterial(fire);
    
    // Steam - WHITE/LIGHT BLUE
    MaterialProperties steam(MaterialType::GAS, "Steam", glm::vec4(0.9f, 0.9f, 1.0f, 0.3f));
    steam.density = 0.6f;
    steam.viscosity = 0.1f;     // Low viscosity for fluid movement
    steam.dispersion = 8.0f;    // High dispersion for wide spreading
    steam.movable = true;
    registerMaterial(steam);
    
    // Smoke - DARK GRAY
    MaterialProperties smoke(MaterialType::GAS, "Smoke", glm::vec4(0.2f, 0.2f, 0.2f, 0.7f));
    smoke.density = 0.5f;       // Slightly denser than steam
    smoke.viscosity = 0.2f;     // Slightly more viscous than steam
    smoke.dispersion = 6.0f;    // Still high dispersion but less than steam
    smoke.movable = true;
    registerMaterial(smoke);
    
    // Wood - BROWN
    MaterialProperties wood(MaterialType::SOLID, "Wood", glm::vec4(0.6f, 0.4f, 0.2f, 1.0f));
    wood.density = 700.0f;
    wood.flammable = true;
    wood.flammability = 0.4f;
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

// CellProcessor implementation
CellProcessor::CellProcessor(MaterialRegistry* registry) 
    : materialRegistry(registry)
{
    // Initialize random number generator with current time
    random.seed(static_cast<unsigned int>(std::chrono::system_clock::now().time_since_epoch().count()));
}

void CellProcessor::initializeCellFromMaterial(Cell& cell, MaterialID materialID) const {
    // Get the material properties
    const MaterialProperties& props = materialRegistry->getMaterial(materialID);
    
    // Initialize the cell with the material's properties
    cell.material = materialID;
    cell.temperature = 20.0f; // Room temperature
    cell.velocity = glm::vec2(0.0f, 0.0f);
    cell.metadata = 0;
    cell.pressure = 0.0f;
    cell.updated = false;
}

void CellProcessor::applyMaterialProperties(Cell& cell, const MaterialProperties& props) const {
    // This would apply material-specific properties to a cell
}

// Minimal stub implementations of the required methods to make compilation succeed

bool CellProcessor::canCellMove(const Cell& cell, const Cell& target) const {
    // If source is empty, it can't move
    if (cell.material == 0) return false;
    
    // If target is empty, movement is always possible
    if (target.material == 0) return true;
    
    // Get material properties
    const MaterialProperties& sourceProps = materialRegistry->getMaterial(cell.material);
    const MaterialProperties& targetProps = materialRegistry->getMaterial(target.material);
    
    // Liquids can displace other liquids of lower density
    if (sourceProps.type == MaterialType::LIQUID && targetProps.type == MaterialType::LIQUID) {
        return sourceProps.density > targetProps.density;
    }
    
    // By default, cells can't move into non-empty cells
    return false;
}

bool CellProcessor::canDisplace(const Cell& mover, const Cell& target) const {
    return true;
}

bool CellProcessor::shouldSwapCells(const Cell& cell1, const Cell& cell2) const {
    return true;
}

bool CellProcessor::canReact(const Cell& cell1, const Cell& cell2) const {
    return false;
}

bool CellProcessor::processPotentialReaction(Cell& cell1, Cell& cell2, float deltaTime) {
    return false;
}

void CellProcessor::processStateChange(Cell& cell, float deltaTime) {
    // Stub implementation
}

void CellProcessor::transferHeat(Cell& sourceCell, Cell& targetCell, float deltaTime) const {
    // Stub implementation
}

bool CellProcessor::checkStateChangeByTemperature(Cell& cell) {
    return false;
}

void CellProcessor::applyVelocity(Cell& cell, const glm::vec2& direction, float speed) {
    // Stub implementation
}

void CellProcessor::applyPressure(Cell& cell, float amount) {
    // Stub implementation
}

void CellProcessor::damageCell(Cell& cell, float amount) {
    // Stub implementation
}

void CellProcessor::igniteCell(Cell& cell) {
    // Stub implementation
}

void CellProcessor::extinguishCell(Cell& cell) {
    // Stub implementation
}

void CellProcessor::freezeCell(Cell& cell) {
    // Stub implementation
}

void CellProcessor::meltCell(Cell& cell) {
    // Stub implementation
}

void CellProcessor::dissolveCell(Cell& cell, float rate) {
    // Stub implementation
}

float CellProcessor::getRandomFloat(float min, float max) {
    std::uniform_real_distribution<float> dist(min, max);
    return dist(random);
}

int CellProcessor::getRandomInt(int min, int max) {
    std::uniform_int_distribution<int> dist(min, max);
    return dist(random);
}

bool CellProcessor::rollProbability(float chance) {
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    return dist(random) < chance;
}

} // namespace astral