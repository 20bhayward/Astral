#include "astral/physics/CellularPhysics.h"
#include "astral/physics/Material.h"
#include "astral/physics/CellProcessor.h"
#include <chrono>
#include <algorithm>
#include <random>
#include <cmath>
#include <functional>
#include <iostream>

namespace astral {

CellularPhysics::CellularPhysics(MaterialRegistry* registry, ChunkManager* chunkManager)
    : materialRegistry(registry)
    , chunkManager(chunkManager)
    , cellProcessor(nullptr)
    , worldWidth(1000) // Default values, should be set properly later
    , worldHeight(1000)
{
    // Initialize with current time
    random.seed(static_cast<unsigned int>(std::chrono::system_clock::now().time_since_epoch().count()));
    
    // Create cell processor
    cellProcessor = new CellProcessor(materialRegistry);
    
    // Initialize
    initialize();
}

CellularPhysics::~CellularPhysics()
{
    if (cellProcessor) {
        delete cellProcessor;
        cellProcessor = nullptr;
    }
}

void CellularPhysics::initialize()
{
    // Set up update tracking grid
    resetUpdateTracker();
    
    // Map material types to their corresponding update functions
    setupUpdateFunctions();
}

void CellularPhysics::setWorldDimensions(int width, int height)
{
    worldWidth = width;
    worldHeight = height;
    
    // Resize update tracking grid
    resetUpdateTracker();
}

void CellularPhysics::setupUpdateFunctions()
{
    // Map each material type to its update function
    updateFunctions[MaterialType::EMPTY] = &CellularPhysics::updateEmpty;
    updateFunctions[MaterialType::SOLID] = &CellularPhysics::updateSolid;
    updateFunctions[MaterialType::POWDER] = &CellularPhysics::updatePowder;
    updateFunctions[MaterialType::LIQUID] = &CellularPhysics::updateLiquid;
    updateFunctions[MaterialType::GAS] = &CellularPhysics::updateGas;
    updateFunctions[MaterialType::FIRE] = &CellularPhysics::updateFire;
    updateFunctions[MaterialType::SPECIAL] = &CellularPhysics::updateSpecial;
}

void CellularPhysics::resetUpdateTracker()
{
    updated.resize(worldHeight);
    for (int y = 0; y < worldHeight; y++) {
        updated[y].resize(worldWidth);
        for (int x = 0; x < worldWidth; x++) {
            updated[y][x] = false;
        }
    }
}

bool CellularPhysics::isValidPosition(int x, int y) const
{
    return x >= 0 && x < worldWidth && y >= 0 && y < worldHeight;
}

Cell& CellularPhysics::getCell(int x, int y)
{
    return chunkManager->getCell(x, y);
}

const Cell& CellularPhysics::getCell(int x, int y) const
{
    return chunkManager->getCell(x, y);
}

MaterialProperties CellularPhysics::getMaterialProperties(int x, int y) const
{
    const Cell& cell = getCell(x, y);
    return materialRegistry->getMaterial(cell.material);
}

bool CellularPhysics::canMove(int x, int y, int newX, int newY)
{
    // Boundary check
    if (!isValidPosition(x, y) || !isValidPosition(newX, newY)) {
        return false;
    }
    
    // Get cells
    Cell& sourceCell = getCell(x, y);
    Cell& targetCell = getCell(newX, newY);
    
    // Return result
    return cellProcessor->canCellMove(sourceCell, targetCell);
}

void CellularPhysics::swapCells(int x, int y, int newX, int newY)
{
    // Boundary check
    if (!isValidPosition(x, y) || !isValidPosition(newX, newY)) {
        return;
    }
    
    // Get cells
    Cell& cell1 = getCell(x, y);
    Cell& cell2 = getCell(newX, newY);
    
    // Perform swap
    std::swap(cell1.material, cell2.material);
    std::swap(cell1.temperature, cell2.temperature);
    std::swap(cell1.velocity, cell2.velocity);
    std::swap(cell1.metadata, cell2.metadata);
    std::swap(cell1.pressure, cell2.pressure);
    std::swap(cell1.health, cell2.health);
    std::swap(cell1.lifetime, cell2.lifetime);
    std::swap(cell1.energy, cell2.energy);
    std::swap(cell1.charge, cell2.charge);
    std::swap(cell1.stateFlags, cell2.stateFlags);
    
    // Don't swap the 'updated' flag - both cells are now updated
    cell1.updated = true;
    cell2.updated = true;
    
    // Mark in update tracker
    if (isValidPosition(x, y)) updated[y][x] = true;
    if (isValidPosition(newX, newY)) updated[newY][newX] = true;
}

// Track ANY cell movements
void CellularPhysics::trackLavaMovement(int x, int y, int newX, int newY) {
    // Function kept for API compatibility, but debug output removed
}

void CellularPhysics::moveCell(int x, int y, int newX, int newY)
{
    // Boundary check
    if (!isValidPosition(x, y) || !isValidPosition(newX, newY)) {
        return;
    }
    
    // Get cells
    Cell& sourceCell = getCell(x, y);
    Cell& targetCell = getCell(newX, newY);
    
    // Preserve target cell temporarily
    Cell tempCell = targetCell;
    
    // Move source to target
    targetCell.material = sourceCell.material;
    targetCell.temperature = sourceCell.temperature;
    targetCell.velocity = sourceCell.velocity;
    targetCell.metadata = sourceCell.metadata;
    targetCell.pressure = sourceCell.pressure;
    targetCell.health = sourceCell.health;
    targetCell.lifetime = sourceCell.lifetime;
    targetCell.energy = sourceCell.energy;
    targetCell.charge = sourceCell.charge;
    targetCell.stateFlags = sourceCell.stateFlags;
    targetCell.updated = true;
    
    // Reset source cell (to air/empty)
    sourceCell.material = materialRegistry->getDefaultMaterialID();
    sourceCell.temperature = 20.0f; // Room temperature
    sourceCell.velocity = glm::vec2(0.0f, 0.0f);
    sourceCell.metadata = 0;
    sourceCell.pressure = 0.0f;
    sourceCell.health = 1.0f;
    sourceCell.lifetime = 0;
    sourceCell.energy = 0.0f;
    sourceCell.charge = 0.0f;
    sourceCell.stateFlags = 0;
    sourceCell.updated = true;
    
    // Mark in update tracker
    if (isValidPosition(x, y)) updated[y][x] = true;
    if (isValidPosition(newX, newY)) updated[newY][newX] = true;
}

void CellularPhysics::applyForce(int x, int y, const glm::vec2& force)
{
    // Boundary check
    if (!isValidPosition(x, y)) {
        return;
    }
    
    // Get cell
    Cell& cell = getCell(x, y);
    
    // Get material properties
    const MaterialProperties& props = materialRegistry->getMaterial(cell.material);
    
    // If material can move, apply the force
    if (props.movable) {
        // Scale force based on material type
        float scaleFactor = 1.0f;
        
        switch (props.type) {
            case MaterialType::POWDER:
                scaleFactor = 0.8f - props.friction * 0.5f;
                break;
                
            case MaterialType::LIQUID:
                scaleFactor = 1.0f - props.viscosity * 0.5f;
                break;
                
            case MaterialType::GAS:
                scaleFactor = 1.2f;
                break;
                
            case MaterialType::FIRE:
                scaleFactor = 1.1f;
                break;
                
            default:
                scaleFactor = 0.5f;
                break;
        }
        
        // Apply scaled force
        cell.velocity += force * scaleFactor;
        
        // Cap maximum velocity
        const float MAX_VELOCITY = 10.0f;
        if (glm::length(cell.velocity) > MAX_VELOCITY) {
            cell.velocity = glm::normalize(cell.velocity) * MAX_VELOCITY;
        }
    }
}

void CellularPhysics::processMaterialInteraction(int x1, int y1, int x2, int y2, float deltaTime)
{
    // Boundary check
    if (!isValidPosition(x1, y1) || !isValidPosition(x2, y2)) {
        return;
    }
    
    // Get cells
    Cell& cell1 = getCell(x1, y1);
    Cell& cell2 = getCell(x2, y2);
    
    // Skip if either cell is empty
    if (cell1.material == materialRegistry->getDefaultMaterialID() && 
        cell2.material == materialRegistry->getDefaultMaterialID()) {
        return;
    }
    
    // Transfer heat between cells
    cellProcessor->transferHeat(cell1, cell2, deltaTime);
    
    // Check for and process reactions
    cellProcessor->processPotentialReaction(cell1, cell2, deltaTime);
    
    // Check for pressure equalization (for fluids)
    const MaterialProperties& props1 = materialRegistry->getMaterial(cell1.material);
    const MaterialProperties& props2 = materialRegistry->getMaterial(cell2.material);
    
    if ((props1.type == MaterialType::LIQUID || props1.type == MaterialType::GAS) &&
        (props2.type == MaterialType::LIQUID || props2.type == MaterialType::GAS)) {
        
        // Equalize pressure
        float avgPressure = (cell1.pressure + cell2.pressure) * 0.5f;
        cell1.pressure = avgPressure;
        cell2.pressure = avgPressure;
    }
}

void CellularPhysics::applyTemperature(int x, int y, float deltaTime)
{
    // Boundary check
    if (!isValidPosition(x, y)) {
        return;
    }
    
    // Get cell
    Cell& cell = getCell(x, y);
    
    // Skip empty cells
    if (cell.material == materialRegistry->getDefaultMaterialID()) {
        return;
    }
    
    // Get material properties
    const MaterialProperties& props = materialRegistry->getMaterial(cell.material);
    
    // Natural cooling/heating towards ambient temperature
    const float AMBIENT_TEMP = 20.0f; // Room temperature
    const float AMBIENT_RATE = 0.01f;
    
    cell.temperature += (AMBIENT_TEMP - cell.temperature) * AMBIENT_RATE * deltaTime;
    
    // Heat generation for fire
    if (props.type == MaterialType::FIRE || cell.hasFlag(Cell::FLAG_BURNING)) {
        cell.temperature = std::max(cell.temperature, 500.0f);
    }
    
    // Special material-specific temperature effects
    if (props.type == MaterialType::GAS && props.name.find("Steam") != std::string::npos) {
        // Steam gradually cools and might condense
        cell.temperature -= 0.1f * deltaTime;
    }
    
    // Apply temperature-based state changes
    cellProcessor->checkStateChangeByTemperature(cell);
}

// Cellular automaton rules for different material types

void CellularPhysics::updateEmpty(int x, int y, float deltaTime)
{
    // Empty cells don't need special updates
    // But they could have temperature or other ambient effects
    Cell& cell = getCell(x, y);
    
    // Apply ambient temperature
    cell.temperature = 20.0f; // Standard room temperature
    
    // Reset other properties just to be safe
    cell.velocity = glm::vec2(0.0f, 0.0f);
    cell.pressure = 0.0f;
    cell.health = 1.0f;
    cell.updated = true;
}

void CellularPhysics::updateSolid(int x, int y, float deltaTime)
{
    // Skip if already updated
    if (updated[y][x]) return;
    
    Cell& cell = getCell(x, y);
    cell.updated = true;
    updated[y][x] = true;
    
    // Get material properties
    const MaterialProperties& props = materialRegistry->getMaterial(cell.material);
    
    // Solids generally don't move, but they can:
    // 1. Conduct heat
    // 2. Change state based on temperature
    // 3. Be affected by gravity if support is removed
    
    // Apply temperature effects
    applyTemperature(x, y, deltaTime);
    
    // Check for support (simple gravity check)
    if (props.movable) {
        // Check if there's solid support below
        bool hasSupport = false;
        
        // Check cell below
        if (isValidPosition(x, y + 1)) {
            const Cell& below = getCell(x, y + 1);
            const MaterialProperties& belowProps = materialRegistry->getMaterial(below.material);
            
            if (belowProps.type == MaterialType::SOLID && !belowProps.movable) {
                hasSupport = true;
            }
        }
        
        // If no support, fall like a powder
        // Note: In our world, down is -y (OpenGL standard coordinates)
        if (!hasSupport && canMove(x, y, x, y - 1)) {
            moveCell(x, y, x, y - 1);
        }
    }
    
    // Apply any velocity the solid might have
    if (glm::length(cell.velocity) > 0.1f) {
        // Calculate movement direction
        int dx = cell.velocity.x > 0.1f ? 1 : (cell.velocity.x < -0.1f ? -1 : 0);
        int dy = cell.velocity.y > 0.1f ? 1 : (cell.velocity.y < -0.1f ? -1 : 0);
        
        // Try to move in the velocity direction
        if (dx != 0 || dy != 0) {
            if (canMove(x, y, x + dx, y + dy)) {
                moveCell(x, y, x + dx, y + dy);
            } else {
                // If blocked, reduce velocity
                cell.velocity *= 0.5f;
            }
        }
    }
}

void CellularPhysics::updatePowder(int x, int y, float deltaTime)
{
    // Skip if already updated
    if (updated[y][x]) {
        return;
    }
    
    Cell& cell = getCell(x, y);
    cell.updated = true;
    updated[y][x] = true;
    
    // Get material properties
    const MaterialProperties& props = materialRegistry->getMaterial(cell.material);
    
    // Check if the material is actually of powder type
    if (props.type != MaterialType::POWDER) {
        return;
    }
    
    // Apply temperature effects
    applyTemperature(x, y, deltaTime);
    
    // SIMPLIFY: Basic powder physics for cleaner, more predictable behavior
    
    // Step 1: Try to fall straight down 
    if (canMove(x, y, x, y + 1)) {
        moveCell(x, y, x, y + 1);
        return;
    }
    
    // Step 2: If blocked below, try to slide diagonally (sand pile formation)
    // Alternate between left and right randomly to avoid biased piles
    bool tryLeftFirst = (x % 2 == 0); // Deterministic but looks random
    
    if (tryLeftFirst) {
        if (canMove(x, y, x - 1, y + 1)) {
            moveCell(x, y, x - 1, y + 1);
            return;
        }
        if (canMove(x, y, x + 1, y + 1)) {
            moveCell(x, y, x + 1, y + 1);
            return;
        }
    } else {
        if (canMove(x, y, x + 1, y + 1)) {
            moveCell(x, y, x + 1, y + 1);
            return;
        }
        if (canMove(x, y, x - 1, y + 1)) {
            moveCell(x, y, x - 1, y + 1);
            return;
        }
    }
    
    // Step 3: If powder is at rest (can't fall further), stop all movement
    // Reset velocity to prevent random jittering
    cell.velocity = glm::vec2(0.0f, 0.0f);
    
}

void CellularPhysics::updateLiquid(int x, int y, float deltaTime)
{
    // Skip if already updated this tick
    if (updated[y][x]) {
        return;
    }
    
    Cell& cell = getCell(x, y);
    cell.updated = true;
    updated[y][x] = true;
    
    // Only process if the cell is a liquid
    const MaterialProperties& props = materialRegistry->getMaterial(cell.material);
    if (props.type != MaterialType::LIQUID) {
        return;
    }
    
    // Apply any temperature effects
    applyTemperature(x, y, deltaTime);
    
    // Step 1: Try to fall straight down
    if (canMove(x, y, x, y + 1)) {
        moveCell(x, y, x, y + 1);
        return;
    }
    
    // Step 2: If blocked below, try to slide diagonally (like powder)
    // Use a deterministic choice for left/right to avoid bias
    bool tryLeftFirst = (x % 2 == 0);
    if (tryLeftFirst) {
        if (canMove(x, y, x - 1, y + 1)) {
            moveCell(x, y, x - 1, y + 1);
            return;
        }
        if (canMove(x, y, x + 1, y + 1)) {
            moveCell(x, y, x + 1, y + 1);
            return;
        }
    } else {
        if (canMove(x, y, x + 1, y + 1)) {
            moveCell(x, y, x + 1, y + 1);
            return;
        }
        if (canMove(x, y, x - 1, y + 1)) {
            moveCell(x, y, x - 1, y + 1);
            return;
        }
    }
    
    // Step 3: If completely blocked below, try horizontal flow to flatten out
    // This mimics spreading behavior with a forced downward check
    if (canMove(x, y, x - 1, y)) {
        moveCell(x, y, x - 1, y);
        // Apply strong downward pressure after horizontal movement
        if (canMove(x - 1, y, x - 1, y + 1)) {
            moveCell(x - 1, y, x - 1, y + 1);
        }
        return;
    }
    if (canMove(x, y, x + 1, y)) {
        moveCell(x, y, x + 1, y);
        if (canMove(x + 1, y, x + 1, y + 1)) {
            moveCell(x + 1, y, x + 1, y + 1);
        }
        return;
    }
    
    // Step 4: As a final measure, check again for downward movement to enforce pressure
    if (canMove(x, y, x, y + 1)) {
        moveCell(x, y, x, y + 1);
        return;
    }
    
    // No movement possible; reset velocity to avoid jitter
    cell.velocity = glm::vec2(0.0f, 0.0f);
}


void CellularPhysics::updateGas(int x, int y, float deltaTime)
{
    // Skip if already updated
    if (updated[y][x]) return;
    
    Cell& cell = getCell(x, y);
    cell.updated = true;
    updated[y][x] = true;
    
    // Get material properties
    const MaterialProperties& props = materialRegistry->getMaterial(cell.material);
    
    // Check if the material is actually of gas type
    if (props.type != MaterialType::GAS) {
        return;
    }
    
    // Apply temperature effects
    applyTemperature(x, y, deltaTime);
    
    // Lifetime decay for gases
    if (cell.lifetime > 0) {
        cell.lifetime--;
        if (cell.lifetime == 0) {
            cell.material = materialRegistry->getDefaultMaterialID(); // Dissipate
            cell.updated = true;
            return;
        }
    }
    
    // Gas simulation: try to rise up (opposite of liquids)
    // Note: In our coordinate system, up is -y (negative y is upward)
    
    // Smoke and steam should rise MUCH faster than fire for stark contrast
    // Make smoke rise aggressively while fire stays mostly in place
    if (props.name == "Smoke" || props.name == "Steam") {
        // Much higher rise chance for smoke and steam
        float riseSpeed = (props.name == "Smoke") ? 0.99f : 0.95f; // Smoke rises extremely fast
        
        // Make fresh smoke rise faster than dissipating smoke
        if (cell.lifetime > 0) {
            float freshness = static_cast<float>(cell.lifetime) / 100.0f; // Normalize to 0-1
            freshness = std::min(freshness, 1.0f);
            // Always keep the rise chance very high (0.9 to 0.99)
            riseSpeed = 0.9f + (freshness * 0.09f);
            
            // Increase vertical speed for smoke - make it rise faster
            if (freshness > 0.5f && cellProcessor->rollProbability(0.3f)) {
                // Occasionally try to make smoke move up two cells at once for faster rising
                int upDist = (props.name == "Smoke") ? 2 : 1;
                if (isValidPosition(x, y - upDist) && 
                    getCell(x, y - upDist).material == materialRegistry->getDefaultMaterialID()) {
                    moveCell(x, y, x, y - upDist);
                    return;
                }
            }
        }
        
        // Higher probability of rising for smoke/steam
        if (cellProcessor->rollProbability(riseSpeed)) {
            if (canMove(x, y, x, y - 1)) {
                moveCell(x, y, x, y - 1);
                return;
            }
        }
    } else {
        // Regular gas movement (slower)
        if (canMove(x, y, x, y - 1)) {
            moveCell(x, y, x, y - 1);
            return;
        }
    }
    
    // Try diagonal rises with random direction preference
    bool tryLeftFirst = cellProcessor->rollProbability(0.5f);
    
    if (tryLeftFirst) {
        if (canMove(x, y, x - 1, y - 1)) {
            moveCell(x, y, x - 1, y - 1);
            return;
        }
        if (canMove(x, y, x + 1, y - 1)) {
            moveCell(x, y, x + 1, y - 1);
            return;
        }
    } else {
        if (canMove(x, y, x + 1, y - 1)) {
            moveCell(x, y, x + 1, y - 1);
            return;
        }
        if (canMove(x, y, x - 1, y - 1)) {
            moveCell(x, y, x - 1, y - 1);
            return;
        }
    }
    
    // Horizontal dispersion (similar to liquid but with higher dispersion)
    int dispersionDistance = static_cast<int>(props.dispersion * 7.0f) + 1;
    
    for (int dist = 1; dist <= dispersionDistance; dist++) {
        // Dispersion probability decreases with distance
        float disperseChance = 0.7f - (dist - 1) * 0.1f;
        
        if (!cellProcessor->rollProbability(disperseChance)) {
            continue;
        }
        
        // Try left or right with random preference
        if (tryLeftFirst) {
            if (canMove(x, y, x - dist, y)) {
                moveCell(x, y, x - dist, y);
                return;
            }
            if (canMove(x, y, x + dist, y)) {
                moveCell(x, y, x + dist, y);
                return;
            }
        } else {
            if (canMove(x, y, x + dist, y)) {
                moveCell(x, y, x + dist, y);
                return;
            }
            if (canMove(x, y, x - dist, y)) {
                moveCell(x, y, x - dist, y);
                return;
            }
        }
    }
}

void CellularPhysics::updateFire(int x, int y, float deltaTime)
{
    // Skip if already updated
    if (updated[y][x]) return;
    
    Cell& cell = getCell(x, y);
    cell.updated = true;
    updated[y][x] = true;
    
    // Get material properties
    const MaterialProperties& props = materialRegistry->getMaterial(cell.material);
    
    // Check if the material is actually of fire type
    if (props.type != MaterialType::FIRE) {
        return;
    }
    
    // Determine if this is oil fire or regular fire
    bool isOilFire = (cell.material == materialRegistry->getOilFireID());
    
    // Fire has a chance to spread to flammable neighbors
    for (int dy = -1; dy <= 1; dy++) {
        for (int dx = -1; dx <= 1; dx++) {
            if (dx == 0 && dy == 0) continue;
            
            int nx = x + dx;
            int ny = y + dy;
            
            if (!isValidPosition(nx, ny)) continue;
            
            Cell& neighbor = getCell(nx, ny);
            const MaterialProperties& neighborProps = materialRegistry->getMaterial(neighbor.material);
            
            if (neighborProps.flammable) {
                // Oil fire spreads more aggressively
                float ignitionMultiplier = isOilFire ? 0.08f : 0.05f;
                float ignitionChance = neighborProps.flammability * ignitionMultiplier * deltaTime * 10.0f;
                
                // Increased ignition chance for wood specifically
                if (neighbor.material == materialRegistry->getWoodID()) {
                    ignitionChance *= 1.2f;
                }
                
                if (cellProcessor->rollProbability(ignitionChance)) {
                    // Ignite neighbor
                    cellProcessor->igniteCell(neighbor);
                }
            }
        }
    }
    
    // Generate smoke periodically while burning, but at a reduced rate
    // Check if fire has fuel beneath it - only generate substantial smoke with fuel
    bool hasFuel = false;
    if (isValidPosition(x, y+1)) { // Check below
        const Cell& belowCell = getCell(x, y+1);
        const MaterialProperties& belowProps = materialRegistry->getMaterial(belowCell.material);
        hasFuel = belowProps.flammable || belowProps.type == MaterialType::FIRE || 
                 belowProps.name == "Lava" || belowProps.name == "Oil";
    }
    
    // Reduced smoke generation probability
    float smokeChance = 0.0f;
    if (hasFuel) {
        // More smoke when burning fuel
        smokeChance = isOilFire ? 0.015f : 0.01f;
    } else {
        // Much less smoke for floating fire
        smokeChance = isOilFire ? 0.005f : 0.003f;
    }
    
    if (cellProcessor->rollProbability(smokeChance)) {
        // Check if there's an empty space above to create smoke
        int smokeY = y - 1;  // Smoke rises upward (negative y)
        if (isValidPosition(x, smokeY) && 
            getCell(x, smokeY).material == materialRegistry->getDefaultMaterialID()) {
            
            Cell& smokeCell = getCell(x, smokeY);
            smokeCell.material = materialRegistry->getSmokeID();
            smokeCell.temperature = isOilFire ? 130.0f : 100.0f;
            
            // Shorter-lived smoke
            smokeCell.lifetime = hasFuel ? 
                (isOilFire ? 80 : 60) :  // On fuel - longer smoke
                (isOilFire ? 40 : 30);   // No fuel - brief smoke
            
            // Darker smoke for oil fires
            if (isOilFire) {
                smokeCell.metadata = 1;
            }
        }
    }
    
    // Make fire visually fade as it burns out by adjusting its velocity
    // We'll use velocity.x as a visual indicator for fire intensity (used in rendering)
    if (cell.lifetime > 0) {
        // Calculate intensity as a percentage of remaining lifetime
        float totalLifetime = isOilFire ? 60.0f : 30.0f;
        float intensity = cell.lifetime / totalLifetime;
        
        // Store intensity in velocity.x for visual effects during rendering
        cell.velocity.x = intensity;
        
        // Check if fire has something to burn beneath it
        bool hasFuel = false;
        if (isValidPosition(x, y+1)) { // Down is +y direction
            const Cell& belowCell = getCell(x, y+1);
            const MaterialProperties& belowProps = materialRegistry->getMaterial(belowCell.material);
            
            // Fire has fuel if it's on flammable material or another fire
            hasFuel = belowProps.flammable || belowProps.type == MaterialType::FIRE || 
                      belowProps.name == "Lava";
        }
        
        // If fire has no fuel source below it, make it burn out EXTREMELY quickly
        if (!hasFuel) {
            // EXTREMELY aggressive burn out for floating fire
            if (cellProcessor->rollProbability(0.9f)) {
                cell.lifetime -= 5; // Burn out 5x faster when not on fuel
            }
            
            // Almost guaranteed to convert to air when no fuel source
            if (cellProcessor->rollProbability(0.8f)) {
                // Convert floating fire directly to air in most cases
                if (cellProcessor->rollProbability(0.8f)) {
                    // Just remove the fire completely
                    cell.material = materialRegistry->getDefaultMaterialID();
                    cell.clearFlag(Cell::FLAG_BURNING);
                } else {
                    // Occasionally convert to a small amount of smoke
                    cell.material = materialRegistry->getSmokeID();
                    cell.clearFlag(Cell::FLAG_BURNING);
                    cell.temperature = isOilFire ? 120.0f : 90.0f;
                    cell.lifetime = 15 + cellProcessor->getRandomInt(0, 10); // Very short-lived smoke
                    cell.metadata = isOilFire ? 1 : 0;
                }
                return;
            }
        }
        
        // As fire burns out, it gets less hot and has a random chance to turn to smoke
        if (cell.lifetime < 10) {
            // Accelerate conversion to smoke when almost burnt out
            cell.temperature = cell.temperature * 0.92f;
            
            // Higher chance to convert to smoke when nearly extinguished
            if (cellProcessor->rollProbability(0.25f)) {
                // Convert low-intensity fire directly to smoke
                cell.material = materialRegistry->getSmokeID();
                cell.clearFlag(Cell::FLAG_BURNING);
                
                // Oil fire produces darker, hotter smoke that lasts longer
                if (isOilFire) {
                    cell.temperature = 130.0f;
                    cell.lifetime = 70 + cellProcessor->getRandomInt(0, 30);
                    cell.metadata = 1; // Mark as oil fire smoke
                } else {
                    cell.temperature = 100.0f;
                    cell.lifetime = 50 + cellProcessor->getRandomInt(0, 30);
                }
                return;
            }
        }
        
        // Decrement lifetime with some randomness for more natural fading
        if (cellProcessor->rollProbability(0.95f)) { // Increased probability to ensure depletion
            cell.lifetime--;
        }
        
        // When fire is almost extinguished, slow down its movement and increase smoke generation
        if (cell.lifetime < 5) {
            // Generate more smoke as the fire is dying
            if (cellProcessor->rollProbability(0.3f)) {
                // Check if there's space above to create smoke
                int smokeY = y - 1;  // Smoke rises upward (negative y)
                if (isValidPosition(x, smokeY) && 
                    getCell(x, smokeY).material == materialRegistry->getDefaultMaterialID()) {
                    
                    Cell& smokeCell = getCell(x, smokeY);
                    smokeCell.material = materialRegistry->getSmokeID();
                    smokeCell.temperature = isOilFire ? 130.0f : 100.0f;
                    smokeCell.lifetime = isOilFire ? 120 : 80;
                    
                    // Darker smoke for oil fires
                    if (isOilFire) {
                        smokeCell.metadata = 1;
                    }
                }
            }
            
            // In the last moments, fire barely moves upward
            return; // Skip movement code below
        }
        
        if (cell.lifetime <= 0) {
            // Transform to smoke when lifetime is depleted
            cell.material = materialRegistry->getSmokeID();
            cell.clearFlag(Cell::FLAG_BURNING);
            
            // Oil fire produces darker, hotter smoke that lasts longer
            if (isOilFire) {
                cell.temperature = 130.0f;
                cell.lifetime = 120 + cellProcessor->getRandomInt(0, 30);
                cell.metadata = 1; // Mark as oil fire smoke
            } else {
                cell.temperature = 100.0f;
                cell.lifetime = 80 + cellProcessor->getRandomInt(0, 20);
            }
            return;
        }
    } else if (cellProcessor->rollProbability(isOilFire ? 0.07f : 0.12f * deltaTime * 10.0f)) { // Increased chance
        // Random burnout chance (oil fire has lower chance)
        // Add some additional smoke before fully burning out
        if (cellProcessor->rollProbability(0.4f)) {
            // Check if there's space around to create smoke
            for (int dy = -1; dy <= 0; dy++) {
                for (int dx = -1; dx <= 1; dx++) {
                    if (dx == 0 && dy == 0) continue;
                    
                    // Try neighboring cells
                    int nx = x + dx;
                    int ny = y + dy;
                    
                    if (!isValidPosition(nx, ny)) continue;
                    
                    if (getCell(nx, ny).material == materialRegistry->getDefaultMaterialID()) {
                        // Create some additional smoke
                        Cell& smokeCell = getCell(nx, ny);
                        smokeCell.material = materialRegistry->getSmokeID();
                        smokeCell.temperature = isOilFire ? 130.0f : 100.0f;
                        smokeCell.lifetime = isOilFire ? 120 : 80;
                        if (isOilFire) {
                            smokeCell.metadata = 1;
                        }
                        break;
                    }
                }
            }
        }
        
        // Convert the fire cell to smoke
        cell.material = materialRegistry->getSmokeID();
        cell.clearFlag(Cell::FLAG_BURNING);
        
        // Oil fire produces darker, hotter smoke that lasts longer
        if (isOilFire) {
            cell.temperature = 130.0f;
            cell.lifetime = 150;
            cell.metadata = 1; // Mark as oil fire smoke
        } else {
            cell.temperature = 100.0f;
            cell.lifetime = 100;
        }
        return;
    }
    
    // Fire rises like gas, but with different characteristics based on type
    // Note: In our coordinate system, up is -y (negative y is upward)
    
    // Make fire much more like real fire - it should barely rise at all
    // Fire should "lick" upward occasionally, but mostly stay in place and flicker
    float intensity = cell.velocity.x; // Use stored intensity value (1.0 = fresh fire, 0.0 = about to extinguish)
    
    // Extremely low rise chance for realistic fire behavior
    // In real fire, flames flicker but mostly stay in place with the fuel
    float riseChance = isOilFire ? 
                       (0.04f + intensity * 0.06f) : // Oil fire: 0.04-0.1 rise chance (very low)
                       (0.06f + intensity * 0.08f); // Regular fire: 0.06-0.14 rise chance (very low)
                       
    // Fire should spread horizontally much more than rising
    float spreadChance = isOilFire ? 
                        (0.4f + intensity * 0.3f) : // Oil fire spreads aggressively (0.4-0.7)
                        (0.15f + intensity * 0.15f); // Regular fire (0.15-0.3)
                        
    // Add randomness to fire height - occasionally let it "lick" upward
    // This creates a flickering effect without constant rising
    if (cellProcessor->rollProbability(0.03f)) {
        // Random "licking" of flames - temporary rise
        riseChance = 0.8f;  // Occasional burst upward
    }
    
    if (cellProcessor->rollProbability(riseChance)) {
        // Try to rise upward
        if (canMove(x, y, x, y - 1)) {
            moveCell(x, y, x, y - 1);
            return;
        }
        
        // Fire can also rise diagonally
        bool tryLeftFirst = cellProcessor->rollProbability(0.5f);
        
        if (tryLeftFirst) {
            if (canMove(x, y, x - 1, y - 1)) {
                moveCell(x, y, x - 1, y - 1);
                return;
            }
            if (canMove(x, y, x + 1, y - 1)) {
                moveCell(x, y, x + 1, y - 1);
                return;
            }
        } else {
            if (canMove(x, y, x + 1, y - 1)) {
                moveCell(x, y, x + 1, y - 1);
                return;
            }
            if (canMove(x, y, x - 1, y - 1)) {
                moveCell(x, y, x - 1, y - 1);
                return;
            }
        }
    }
    
    // Horizontal movement for fire
    if (cellProcessor->rollProbability(spreadChance)) {
        int dir = cellProcessor->rollProbability(0.5f) ? 1 : -1;
        
        if (canMove(x, y, x + dir, y)) {
            moveCell(x, y, x + dir, y);
        }
    }
}

void CellularPhysics::updateSpecial(int x, int y, float deltaTime)
{
    // Skip if already updated
    if (updated[y][x]) return;
    
    Cell& cell = getCell(x, y);
    cell.updated = true;
    updated[y][x] = true;
    
    // Special materials can have custom behavior defined by the metadata
    // For example, metadata could define different special material types:
    // - Explosive (metadata = 1)
    // - Acid (metadata = 2)
    // - Generator (metadata = 3)
    // - etc.
    
    // Apply temperature effects
    applyTemperature(x, y, deltaTime);
    
    // Process special behavior based on metadata
    switch (cell.metadata) {
        case 1: // Explosive
            // Check if triggered
            if (cell.temperature > 100.0f || cell.hasFlag(Cell::FLAG_BURNING)) {
                // Create explosion
                createExplosion(x, y, 5.0f, 10.0f);
                cell.material = materialRegistry->getFireID();
                cell.temperature = 500.0f;
                cell.setFlag(Cell::FLAG_BURNING);
            }
            break;
            
        case 2: // Acid
            // Try to dissolve neighbors
            for (int dy = -1; dy <= 1; dy++) {
                for (int dx = -1; dx <= 1; dx++) {
                    if (dx == 0 && dy == 0) continue;
                    
                    int nx = x + dx;
                    int ny = y + dy;
                    
                    if (!isValidPosition(nx, ny)) continue;
                    
                    Cell& neighbor = getCell(nx, ny);
                    const MaterialProperties& neighborProps = materialRegistry->getMaterial(neighbor.material);
                    
                    // Acid doesn't dissolve other acid or empty space
                    if (neighbor.material == materialRegistry->getDefaultMaterialID() ||
                        neighbor.metadata == 2) continue;
                    
                    // Try to dissolve
                    if (cellProcessor->rollProbability(0.1f * deltaTime * 5.0f)) {
                        cellProcessor->damageCell(neighbor, 0.2f);
                    }
                }
            }
            
            // Acid behaves like a liquid
            updateLiquid(x, y, deltaTime);
            break;
            
        case 3: // Heat generator
            // Generate heat
            cell.temperature = 500.0f;
            
            // Heat neighbors
            for (int dy = -1; dy <= 1; dy++) {
                for (int dx = -1; dx <= 1; dx++) {
                    if (dx == 0 && dy == 0) continue;
                    
                    int nx = x + dx;
                    int ny = y + dy;
                    
                    if (!isValidPosition(nx, ny)) continue;
                    
                    Cell& neighbor = getCell(nx, ny);
                    neighbor.temperature += 5.0f * deltaTime;
                }
            }
            break;
            
        default:
            // Unknown special material, behave like a solid
            updateSolid(x, y, deltaTime);
            break;
    }
}

void CellularPhysics::updateChunk(Chunk* chunk, float deltaTime)
{
    if (!chunk || !chunk->isActive()) return;
    
    ChunkCoord coord = chunk->getCoord();
    
    // To avoid artifacts from sequential updating, use different orders:
    // - For powders and liquids, update bottom to top
    // - For gases and fire, update top to bottom
    
    // First pass: bottom to top (for falling materials)
    for (int localY = CHUNK_SIZE - 1; localY >= 0; localY--) {
        for (int localX = 0; localX < CHUNK_SIZE; localX++) {
            // Convert to world coordinates
            WorldCoord worldCoord = ChunkManager::chunkToWorldCoord(coord, {localX, localY});
            
            if (!isValidPosition(worldCoord.x, worldCoord.y)) continue;
            
            const Cell& cell = chunk->getCell(localX, localY);
            const MaterialProperties& props = materialRegistry->getMaterial(cell.material);
            
            // Update falling materials
            if (props.type == MaterialType::POWDER || props.type == MaterialType::LIQUID) {
                // Use the appropriate update function
                auto it = updateFunctions.find(props.type);
                if (it != updateFunctions.end()) {
                    it->second(this, worldCoord.x, worldCoord.y, deltaTime);
                }
            }
        }
    }
    
    // Second pass: top to bottom (for rising materials)
    for (int localY = 0; localY < CHUNK_SIZE; localY++) {
        for (int localX = 0; localX < CHUNK_SIZE; localX++) {
            // Convert to world coordinates
            WorldCoord worldCoord = ChunkManager::chunkToWorldCoord(coord, {localX, localY});
            
            if (!isValidPosition(worldCoord.x, worldCoord.y)) continue;
            
            const Cell& cell = chunk->getCell(localX, localY);
            const MaterialProperties& props = materialRegistry->getMaterial(cell.material);
            
            // Update rising materials
            if (props.type == MaterialType::GAS || props.type == MaterialType::FIRE) {
                // Use the appropriate update function
                auto it = updateFunctions.find(props.type);
                if (it != updateFunctions.end()) {
                    it->second(this, worldCoord.x, worldCoord.y, deltaTime);
                }
            }
        }
    }
    
    // Third pass: solid and special materials
    for (int localY = 0; localY < CHUNK_SIZE; localY++) {
        for (int localX = 0; localX < CHUNK_SIZE; localX++) {
            // Convert to world coordinates
            WorldCoord worldCoord = ChunkManager::chunkToWorldCoord(coord, {localX, localY});
            
            if (!isValidPosition(worldCoord.x, worldCoord.y)) continue;
            
            const Cell& cell = chunk->getCell(localX, localY);
            const MaterialProperties& props = materialRegistry->getMaterial(cell.material);
            
            // Update solid and special materials
            if (props.type == MaterialType::SOLID || props.type == MaterialType::SPECIAL || 
                props.type == MaterialType::EMPTY) {
                
                // Use the appropriate update function
                auto it = updateFunctions.find(props.type);
                if (it != updateFunctions.end()) {
                    it->second(this, worldCoord.x, worldCoord.y, deltaTime);
                }
            }
        }
    }
    
    // Fourth pass: process interactions between adjacent cells
    for (int localY = 0; localY < CHUNK_SIZE; localY++) {
        for (int localX = 0; localX < CHUNK_SIZE; localX++) {
            // Convert to world coordinates
            WorldCoord worldCoord = ChunkManager::chunkToWorldCoord(coord, {localX, localY});
            
            if (!isValidPosition(worldCoord.x, worldCoord.y)) continue;
            
            // Process interactions with neighbors
            for (int dy = -1; dy <= 1; dy++) {
                for (int dx = -1; dx <= 1; dx++) {
                    if (dx == 0 && dy == 0) continue;
                    
                    int nx = worldCoord.x + dx;
                    int ny = worldCoord.y + dy;
                    
                    if (!isValidPosition(nx, ny)) continue;
                    
                    processMaterialInteraction(worldCoord.x, worldCoord.y, nx, ny, deltaTime);
                }
            }
        }
    }
}

void CellularPhysics::update(float deltaTime)
{
    // Reset update tracking for new frame
    resetUpdateTracker();
    
    // Use optimized parallel chunk processing for better performance
    chunkManager->updateChunksParallel(deltaTime);
    
    // Directly update all cells in active chunks
    const auto& activeChunks = chunkManager->getActiveChunks();
    
    // FIRST PHASE: Process all cell movements based on their type
    for (const auto& chunkCoord : activeChunks) {
        Chunk* chunk = chunkManager->getChunk(chunkCoord);
        if (chunk) {
            // Process cells in the chunk using our update methods
            for (int localY = 0; localY < CHUNK_SIZE; localY++) {
                for (int localX = 0; localX < CHUNK_SIZE; localX++) {
                    // Convert to world coordinates
                    int worldX = chunkCoord.x * CHUNK_SIZE + localX;
                    int worldY = chunkCoord.y * CHUNK_SIZE + localY;
                    
                    // Get cell and material 
                    Cell& cell = chunkManager->getCell(worldX, worldY);
                    
                    // Skip empty cells
                    if (cell.material == 0) continue;
                    
                    // Get material properties
                    MaterialProperties props = materialRegistry->getMaterial(cell.material);
                    
                    // Call the appropriate update function based on material type
                    switch (props.type) {
                        case MaterialType::EMPTY:
                            updateEmpty(worldX, worldY, deltaTime);
                            break;
                        case MaterialType::SOLID:
                            updateSolid(worldX, worldY, deltaTime);
                            break;
                        case MaterialType::POWDER:
                            updatePowder(worldX, worldY, deltaTime);
                            break;
                        case MaterialType::LIQUID:
                            updateLiquid(worldX, worldY, deltaTime);
                            break;
                        case MaterialType::GAS:
                            updateGas(worldX, worldY, deltaTime);
                            break;
                        case MaterialType::FIRE:
                            updateFire(worldX, worldY, deltaTime);
                            break;
                        case MaterialType::SPECIAL:
                            updateSpecial(worldX, worldY, deltaTime);
                            break;
                    }
                }
            }
        }
    }
    
    // SECOND PHASE: Process all material interactions between cells
    for (const auto& chunkCoord : activeChunks) {
        Chunk* chunk = chunkManager->getChunk(chunkCoord);
        if (chunk) {
            for (int localY = 0; localY < CHUNK_SIZE; localY++) {
                for (int localX = 0; localX < CHUNK_SIZE; localX++) {
                    // Convert to world coordinates
                    int worldX = chunkCoord.x * CHUNK_SIZE + localX;
                    int worldY = chunkCoord.y * CHUNK_SIZE + localY;
                    
                    // Skip empty cells or out of bounds
                    if (!isValidPosition(worldX, worldY)) continue;
                    Cell& cell = chunkManager->getCell(worldX, worldY);
                    if (cell.material == 0) continue;
                    
                    // Apply temperature effects to all cells
                    applyTemperature(worldX, worldY, deltaTime);
                    
                    // Process interactions with ALL neighboring cells
                    for (int dy = -1; dy <= 1; dy++) {
                        for (int dx = -1; dx <= 1; dx++) {
                            if (dx == 0 && dy == 0) continue;
                            
                            int nx = worldX + dx;
                            int ny = worldY + dy;
                            
                            if (isValidPosition(nx, ny)) {
                                // Process material interaction and heat transfer between cells
                                processMaterialInteraction(worldX, worldY, nx, ny, deltaTime);
                            }
                        }
                    }
                    
                    // Check for state changes by temperature for this cell
                    // This handles phase transitions like water->steam, etc.
                    cellProcessor->checkStateChangeByTemperature(cell);
                }
            }
        }
    }
    
    // THIRD PHASE: Ensure all cells are active for the next frame
    for (const auto& chunkCoord : activeChunks) {
        Chunk* chunk = chunkManager->getChunk(chunkCoord);
        if (chunk) {
            // Set random velocities on some cells to kick-start activity
            // This ensures particles start moving even if initial conditions are static
            // for (int y = 0; y < CHUNK_SIZE; y++) {
            //     for (int x = 0; x < CHUNK_SIZE; x++) {
            //         Cell& cell = chunk->getCell(x, y);
            //         // Materials should stay active
            //         if (cell.material != 0) {
            //             // Ensure all material cells are active for the simulation
            //             cell.updated = true;
                        
            //             // Only give random velocity to cells that are already moving or to gases/fire
            //             const MaterialProperties& props = materialRegistry->getMaterial(cell.material);
                        
            //             // Check if cell is already moving - don't disturb resting cells
            //             bool isMoving = (glm::length(cell.velocity) > 0.05f);
                        
            //             // Only apply to gases and fire which should always be moving
            //             // Don't disturb liquids and powders that have settled
            //             if ((props.type == MaterialType::GAS || props.type == MaterialType::FIRE) ||
            //                 // Only add small velocity to moving liquids/powders (avoid creating perpetual motion)
            //                 (isMoving && (props.type == MaterialType::LIQUID || props.type == MaterialType::POWDER))) {
                            
            //                 // Reduce velocity magnitude significantly for stability
            //                 float velMagnitude = 0.05f;
            //                 if (props.type == MaterialType::GAS || props.type == MaterialType::FIRE) {
            //                     // Gases and fire should move more actively
            //                     velMagnitude = 0.1f;
            //                 }
                            
            //                 cell.velocity.x += (rand() % 100 - 50) / 1000.0f * velMagnitude;
            //                 cell.velocity.y += (rand() % 100 - 50) / 1000.0f * velMagnitude;
                            
            //                 // Cap maximum velocity to avoid erratic behavior
            //                 float maxVel = 2.0f;
            //                 if (glm::length(cell.velocity) > maxVel) {
            //                     cell.velocity = glm::normalize(cell.velocity) * maxVel;
            //                 }
            //             } 
            //             // Apply damping to settled materials over time
            //             else if (props.type == MaterialType::LIQUID || props.type == MaterialType::POWDER) {
            //                 // Gradually stop movement of settled materials
            //                 cell.velocity *= 0.8f;
                            
            //                 // If velocity is very small, just stop completely
            //                 if (glm::length(cell.velocity) < 0.05f) {
            //                     cell.velocity = glm::vec2(0.0f, 0.0f);
            //                 }
            //             }
            //         }
            //     }
            // }
            chunk->setActive(true);
        }
    }
    
    // If we have any active special effects, process them
    processActiveEffects(deltaTime);
}

void CellularPhysics::createExplosion(int x, int y, float radius, float power)
{
    // Apply force and damage in a circular area
    int intRadius = static_cast<int>(radius);
    
    for (int dy = -intRadius; dy <= intRadius; dy++) {
        for (int dx = -intRadius; dx <= intRadius; dx++) {
            int nx = x + dx;
            int ny = y + dy;
            
            if (!isValidPosition(nx, ny)) continue;
            
            // Calculate distance from center
            float distance = std::sqrt(dx * dx + dy * dy);
            if (distance > radius) continue;
            
            // Calculate normalized direction from center
            glm::vec2 direction(dx, dy);
            if (glm::length(direction) > 0.0f) {
                direction = glm::normalize(direction);
            } else {
                direction = glm::vec2(0.0f, 0.0f);
            }
            
            // Force and damage decrease with distance
            float intensity = 1.0f - (distance / radius);
            float forceMagnitude = power * intensity;
            float damageAmount = power * intensity * 0.2f;
            
            // Apply explosion effects
            Cell& cell = getCell(nx, ny);
            
            // Apply force
            applyForce(nx, ny, direction * forceMagnitude);
            
            // Apply damage
            cellProcessor->damageCell(cell, damageAmount);
            
            // Increase temperature
            cell.temperature += 200.0f * intensity;
            
            // Chance to ignite flammable materials
            const MaterialProperties& props = materialRegistry->getMaterial(cell.material);
            if (props.flammable && cellProcessor->rollProbability(props.flammability * intensity)) {
                cellProcessor->igniteCell(cell);
            }
        }
    }
    
    // Create fire at the center of explosion
    if (isValidPosition(x, y)) {
        Cell& centerCell = getCell(x, y);
        centerCell.material = materialRegistry->getFireID();
        centerCell.temperature = 800.0f;
        centerCell.setFlag(Cell::FLAG_BURNING);
    }
}

void CellularPhysics::createHeatSource(int x, int y, float temperature, float radius)
{
    // Apply heat in a circular area
    int intRadius = static_cast<int>(radius);
    
    for (int dy = -intRadius; dy <= intRadius; dy++) {
        for (int dx = -intRadius; dx <= intRadius; dx++) {
            int nx = x + dx;
            int ny = y + dy;
            
            if (!isValidPosition(nx, ny)) continue;
            
            // Calculate distance from center
            float distance = std::sqrt(dx * dx + dy * dy);
            if (distance > radius) continue;
            
            // Heat decreases with distance
            float intensity = 1.0f - (distance / radius);
            float heatAmount = temperature * intensity;
            
            // Apply heat
            Cell& cell = getCell(nx, ny);
            cell.temperature = std::max(cell.temperature, heatAmount);
            
            // Check for state changes due to temperature
            cellProcessor->checkStateChangeByTemperature(cell);
        }
    }
}

void CellularPhysics::applyForceField(int x, int y, const glm::vec2& direction, float strength, float radius)
{
    // Apply force in a circular area
    int intRadius = static_cast<int>(radius);
    
    for (int dy = -intRadius; dy <= intRadius; dy++) {
        for (int dx = -intRadius; dx <= intRadius; dx++) {
            int nx = x + dx;
            int ny = y + dy;
            
            if (!isValidPosition(nx, ny)) continue;
            
            // Calculate distance from center
            float distance = std::sqrt(dx * dx + dy * dy);
            if (distance > radius) continue;
            
            // Force decreases with distance
            float intensity = 1.0f - (distance / radius);
            float forceMagnitude = strength * intensity;
            
            // Apply force
            applyForce(nx, ny, direction * forceMagnitude);
        }
    }
}

bool CellularPhysics::isCellUpdated(int x, int y) const
{
    if (!isValidPosition(x, y)) return false;
    return updated[y][x];
}

void CellularPhysics::visualizePropertyField(const std::string& propertyName)
{
    // This method would be implemented to visualize different cell properties
    // like temperature, pressure, etc. as color overlays for debugging
    // Actual implementation would depend on the rendering system
}

void CellularPhysics::processActiveEffects(float deltaTime)
{
    // This method processes any active special effects that need continuous updating
    // Currently, there are no persistent effects that need frame-by-frame updates
    // but this provides a hook for adding such features in the future
    
    // Process temperature diffusion between chunks
    const auto& activeChunks = chunkManager->getActiveChunks();
    for (const auto& chunkCoord : activeChunks) {
        // Process heat sources
        Chunk* chunk = chunkManager->getChunk(chunkCoord);
        if (chunk) {
            for (int y = 0; y < CHUNK_SIZE; y++) {
                for (int x = 0; x < CHUNK_SIZE; x++) {
                    Cell& cell = chunk->getCell(x, y);
                    
                    // Skip cells that aren't heat sources
                    if (cell.metadata != 1) continue;
                    
                    // Convert local coordinates to world coordinates
                    WorldCoord worldCoord = ChunkManager::chunkToWorldCoord(
                        chunkCoord, {x, y}
                    );
                    
                    // Apply heat to surrounding cells
                    float radius = static_cast<float>(cell.temperature / 100.0f);
                    createHeatSource(worldCoord.x, worldCoord.y, cell.temperature, radius);
                }
            }
        }
    }
}

void CellularPhysics::dumpPerformanceStats() const
{
    // Get performance statistics from the chunk manager
    auto stats = chunkManager->getPerformanceStats();
    
    // Output the stats
    std::cout << "===== Physics Performance Stats =====" << std::endl;
    std::cout << "Total Chunks: " << stats.totalChunks << std::endl;
    std::cout << "Active Chunks: " << stats.activeChunks << " (" 
              << (stats.totalChunks > 0 ? (static_cast<float>(stats.activeChunks) / stats.totalChunks * 100.0f) : 0.0f)
              << "%)" << std::endl;
    std::cout << "Total Cells: " << stats.totalCells << std::endl;
    std::cout << "Active Cells: " << stats.activeCells << " (" << stats.activePercentage << "%)" << std::endl;
    std::cout << "Update Time: " << stats.updateTime << " ms" << std::endl;
    std::cout << "===================================" << std::endl;
}

} // namespace astral