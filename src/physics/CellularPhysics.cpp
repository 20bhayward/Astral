#include "astral/physics/CellularPhysics.h"
#include "astral/physics/Material.h"
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
    static int canMoveCount = 0;
    canMoveCount++;
    
    // Boundary check
    if (!isValidPosition(x, y) || !isValidPosition(newX, newY)) {
        if (canMoveCount % 1000 == 0) {
            std::cout << "CAN_MOVE CALL #" << canMoveCount << ": FAILED due to out of bounds - (" 
                     << x << "," << y << ") to (" << newX << "," << newY << ")" << std::endl;
        }
        return false;
    }
    
    // Get cells
    Cell& sourceCell = getCell(x, y);
    Cell& targetCell = getCell(newX, newY);
    
    // Get result early for logging
    bool canMoveResult = cellProcessor->canCellMove(sourceCell, targetCell);
    
    // Print status every 1000 calls
    if (canMoveCount % 1000 == 0) {
        const MaterialProperties& sourceProp = materialRegistry->getMaterial(sourceCell.material);
        const MaterialProperties& targetProp = materialRegistry->getMaterial(targetCell.material);
        
        std::cout << "CAN_MOVE CALL #" << canMoveCount << ": " 
                 << (canMoveResult ? "ALLOWED" : "BLOCKED") 
                 << " - Source: " << sourceProp.name << "(" << x << "," << y << ")"
                 << ", Target: " << targetProp.name << "(" << newX << "," << newY << ")" << std::endl;
    }
    
    return canMoveResult;
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
    static int moveCount = 0;
    
    // Only print if there's actual movement (different coordinates)
    if (x != newX || y != newY) {
        moveCount++;
        std::cout << "MOVEMENT #" << moveCount << ": Cell moved from (" 
                  << x << "," << y << ") to (" << newX << "," << newY << ")" << std::endl;
    }
}

void CellularPhysics::moveCell(int x, int y, int newX, int newY)
{
    static int callCount = 0;
    callCount++;
    
    // Just print every 100th call to avoid spamming
    if (callCount % 100 == 0) {
        std::cout << "MOVE CALL #" << callCount << ": Attempt to move from (" 
                  << x << "," << y << ") to (" << newX << "," << newY << ")" << std::endl;
    }
    
    // Boundary check
    if (!isValidPosition(x, y) || !isValidPosition(newX, newY)) {
        if (callCount % 100 == 0) {
            std::cout << "  -> FAILED: Out of bounds" << std::endl;
        }
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
    static int powderUpdateCount = 0;
    powderUpdateCount++;
    
    // Print every 1000 calls
    if (powderUpdateCount % 1000 == 0) {
        std::cout << "UPDATE POWDER #" << powderUpdateCount << ": Position (" << x << "," << y << ")" << std::endl;
    }
    
    // Skip if already updated
    if (updated[y][x]) {
        if (powderUpdateCount % 1000 == 0) {
            std::cout << "  -> SKIPPED: Already updated" << std::endl;
        }
        return;
    }
    
    Cell& cell = getCell(x, y);
    cell.updated = true;
    updated[y][x] = true;
    
    // Get material properties
    const MaterialProperties& props = materialRegistry->getMaterial(cell.material);
    
    // Check if the material is actually of powder type
    if (props.type != MaterialType::POWDER) {
        if (powderUpdateCount % 1000 == 0) {
            std::cout << "  -> SKIPPED: Not powder type" << std::endl;
        }
        return;
    }
    
    // Apply temperature effects
    applyTemperature(x, y, deltaTime);
    
    // Basic powder simulation: try to fall down
    // FIXED: In screen coordinates, down is +y
    if (canMove(x, y, x, y + 1)) {
        moveCell(x, y, x, y + 1);
        return;
    }
    
    // Try diagonal falls with random direction preference
    bool tryLeftFirst = cellProcessor->rollProbability(0.5f);
    
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
    
    // If can't fall, apply any existing velocity
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
                cell.velocity *= 0.8f;
            }
        }
    }
    
    // Check for potential horizontal movement to reach a more stable position
    if (cell.pressure > 0.1f || glm::length(cell.velocity) > 0.1f) {
        // Determine stacking pressure by counting cells above
        int stackHeight = 0;
        for (int cy = y - 1; cy >= 0 && cy >= y - 10; cy--) {
            if (!isValidPosition(x, cy)) break;
            
            const Cell& above = getCell(x, cy);
            const MaterialProperties& aboveProps = materialRegistry->getMaterial(above.material);
            
            if (aboveProps.type == MaterialType::POWDER || aboveProps.type == MaterialType::SOLID) {
                stackHeight++;
            } else {
                break;
            }
        }
        
        // Pressure from stack increases chance of horizontal movement
        float horizontalChance = std::min(0.3f, 0.05f + stackHeight * 0.03f);
        
        if (cellProcessor->rollProbability(horizontalChance)) {
            // Try to move horizontally
            int dir = cellProcessor->rollProbability(0.5f) ? 1 : -1;
            
            if (canMove(x, y, x + dir, y)) {
                moveCell(x, y, x + dir, y);
            }
        }
    }
}

void CellularPhysics::updateLiquid(int x, int y, float deltaTime)
{
    static int liquidUpdateCount = 0;
    liquidUpdateCount++;
    
    // Print every 1000 calls
    if (liquidUpdateCount % 1000 == 0) {
        std::cout << "UPDATE LIQUID #" << liquidUpdateCount << ": Position (" << x << "," << y << ")" << std::endl;
    }
    
    // Skip if already updated
    if (updated[y][x]) {
        if (liquidUpdateCount % 1000 == 0) {
            std::cout << "  -> SKIPPED: Already updated" << std::endl;
        }
        return;
    }
    
    Cell& cell = getCell(x, y);
    cell.updated = true;
    updated[y][x] = true;
    
    // Get material properties
    const MaterialProperties& props = materialRegistry->getMaterial(cell.material);
    
    // Check if the material is actually of liquid type
    if (props.type != MaterialType::LIQUID) {
        if (liquidUpdateCount % 1000 == 0) {
            std::cout << "  -> SKIPPED: Not liquid type (material=" << props.name << ")" << std::endl;
        }
        return;
    }
    
    // Apply temperature effects
    applyTemperature(x, y, deltaTime);
    
    // Viscosity affects update frequency
    if (props.viscosity > 0.0f && cellProcessor->rollProbability(props.viscosity)) {
        return; // Skip movement based on viscosity
    }
    
    // Basic liquid simulation: try to fall down
    // FIXED: In screen coordinates or window coordinates, down is +y
    if (canMove(x, y, x, y + 1)) {
        moveCell(x, y, x, y + 1);
        return;
    }
    
    // Try diagonal falls with random direction preference
    bool tryLeftFirst = cellProcessor->rollProbability(0.5f);
    
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
    
    // Horizontal flow based on dispersion
    int dispersionDistance = static_cast<int>(props.dispersion * 5.0f) + 1;
    
    for (int dist = 1; dist <= dispersionDistance; dist++) {
        // Flow probability decreases with distance and increases with pressure
        float flowChance = 0.8f - (dist - 1) * 0.2f + cell.pressure * 0.05f;
        
        if (!cellProcessor->rollProbability(flowChance)) {
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
        
        // Only check the immediate neighbors for performance
        if (dist > 1) break;
    }
    
    // Pressure effects: High pressure can force liquid upward
    // Note: In our world, up is +y (OpenGL standard coordinates)
    if (cell.pressure > 2.0f) {
        float upChance = (cell.pressure - 2.0f) * 0.1f;
        if (cellProcessor->rollProbability(upChance)) {
            if (canMove(x, y, x, y + 1)) {
                moveCell(x, y, x, y + 1);
                return;
            }
        }
    }
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
    // Note: In our world, up is +y (OpenGL standard coordinates)
    if (canMove(x, y, x, y + 1)) {
        moveCell(x, y, x, y + 1);
        return;
    }
    
    // Try diagonal rises with random direction preference
    bool tryLeftFirst = cellProcessor->rollProbability(0.5f);
    
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
                float ignitionChance = neighborProps.flammability * 0.05f * deltaTime * 10.0f;
                if (cellProcessor->rollProbability(ignitionChance)) {
                    // Ignite neighbor
                    cellProcessor->igniteCell(neighbor);
                }
            }
        }
    }
    
    // Fire has a chance to burn out and turn to smoke
    if (cell.lifetime > 0) {
        cell.lifetime--;
        if (cell.lifetime == 0) {
            // Transform to smoke
            cell.material = materialRegistry->getSmokeID();
            cell.clearFlag(Cell::FLAG_BURNING);
            cell.temperature = 100.0f;
            cell.lifetime = 100; // Smoke lasts a while
            return;
        }
    } else if (cellProcessor->rollProbability(0.05f * deltaTime * 10.0f)) {
        // Random burnout chance
        cell.material = materialRegistry->getSmokeID();
        cell.clearFlag(Cell::FLAG_BURNING);
        return;
    }
    
    // Fire rises like gas
    // Note: In our world, up is +y (OpenGL standard coordinates)
    if (canMove(x, y, x, y + 1)) {
        moveCell(x, y, x, y + 1);
        return;
    }
    
    // Fire can also rise diagonally
    bool tryLeftFirst = cellProcessor->rollProbability(0.5f);
    
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
    
    // Horizontal movement for fire
    if (cellProcessor->rollProbability(0.3f)) {
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
    static int updateCallCount = 0;
    updateCallCount++;
    std::cout << "PHYSICS UPDATE #" << updateCallCount << " called with deltaTime=" << deltaTime << std::endl;
    
    // Reset update tracking for new frame
    resetUpdateTracker();
    
    // Use optimized parallel chunk processing for better performance
    std::cout << " -> Calling chunkManager->updateChunksParallel()" << std::endl;
    chunkManager->updateChunksParallel(deltaTime);
    
    // CRITICAL FIX: Directly update all cells in active chunks since Chunk::update doesn't call physics
    std::cout << " -> CRITICAL FIX: Manually processing all cells in active chunks" << std::endl;
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
    // This is the critical step that was being skipped/overlooked!
    std::cout << " -> PROCESSING ALL MATERIAL INTERACTIONS" << std::endl;
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
            for (int y = 0; y < CHUNK_SIZE; y++) {
                for (int x = 0; x < CHUNK_SIZE; x++) {
                    Cell& cell = chunk->getCell(x, y);
                    // Materials should stay active
                    if (cell.material != 0) {
                        // Ensure all material cells are active for the simulation
                        cell.updated = true;
                        
                        // Give fluid cells a small random velocity to kickstart movement
                        const MaterialProperties& props = materialRegistry->getMaterial(cell.material);
                        if (props.type == MaterialType::LIQUID || props.type == MaterialType::POWDER || 
                            props.type == MaterialType::GAS || props.type == MaterialType::FIRE) {
                            // Add a larger velocity to all cells to ensure they exceed movement threshold
                            // The velocity threshold for movement is 0.1f, so we need to exceed that
                            cell.velocity.x = (rand() % 100 - 50) / 250.0f; // Doubled magnitude
                            cell.velocity.y = (rand() % 100 - 50) / 250.0f; // Added negative values too
                        }
                    }
                }
            }
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