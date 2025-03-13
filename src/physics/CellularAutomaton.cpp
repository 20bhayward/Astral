#include "astral/physics/CellularAutomaton.h"
#include <random>
#include <chrono>
#include <cmath>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

namespace astral {

CellularAutomaton::CellularAutomaton(int width, int height)
    : materialRegistry()
    , chunkManager(nullptr)
    , physics(nullptr)
    , isPaused(false)
    , timeScale(1.0f)
    , worldWidth(width)
    , worldHeight(height)
    , updateTimer()
{
    // Initialize active area to the full world
    activeArea.x = 0;
    activeArea.y = 0;
    activeArea.width = worldWidth;
    activeArea.height = worldHeight;
    
    // Initialize the material registry with default materials
    initialize();
}

CellularAutomaton::~CellularAutomaton()
{
    // Materials will be cleaned up by the registry's destructor
    // Chunks will be cleaned up by the ChunkManager's destructor
}

void CellularAutomaton::initialize()
{
    // Use the built-in basic materials registration
    materialRegistry.registerBasicMaterials();
    
    // Create chunk manager
    chunkManager = std::make_unique<ChunkManager>(&materialRegistry);
    
    // Create cellular physics system
    physics = std::make_unique<CellularPhysics>(&materialRegistry, chunkManager.get());
    physics->setWorldDimensions(worldWidth, worldHeight);
    
    // Initialize with empty world
    reset(WorldTemplate::EMPTY);
}

void CellularAutomaton::reset(WorldTemplate tmpl)
{
    // Reset timer
    updateTimer.reset();
    
    // Clear existing world first
    clearWorld();
    
    // Initialize with the selected template
    initializeWorldFromTemplate(tmpl);
    
    // Set active area to the full world to ensure all cells can be active
    WorldRect initialActiveArea = {0, 0, worldWidth, worldHeight};
    chunkManager->updateActiveChunks(initialActiveArea);
    
    // Also set the active area for later updates
    activeArea = initialActiveArea;
    
    // Reset stats
    stats = SimulationStats();
    stats.activeChunks = chunkManager->getActiveChunkCount();
    stats.totalCells = worldWidth * worldHeight;
    
    // Resume simulation
    isPaused = false;
    timeScale = 1.0f;
}

void CellularAutomaton::update(float deltaTime)
{
    // Skip if paused
    if (isPaused) {
        return;
    }
    
    // Apply time scaling
    float scaledDeltaTime = deltaTime * timeScale;
    
    // Reset timer for timing this update
    updateTimer.reset();
    
    // Update active chunks
    chunkManager->updateActiveChunks(activeArea);
    
    // Update physics
    physics->update(scaledDeltaTime);
    
    // Update timer to get elapsed time
    updateTimer.update();
    
    // Update statistics
    updateSimulationStats();
}

Cell& CellularAutomaton::getCell(int x, int y)
{
    return chunkManager->getCell(x, y);
}

const Cell& CellularAutomaton::getCell(int x, int y) const
{
    return chunkManager->getCell(x, y);
}

void CellularAutomaton::setCell(int x, int y, const Cell& cell)
{
    chunkManager->setCell(x, y, cell);
}

void CellularAutomaton::setCell(int x, int y, MaterialID material)
{
    // Make sure the position is in the world
    if (x < 0 || x >= worldWidth || y < 0 || y >= worldHeight) {
        return;
    }
    
    // Create a cell with the specified material
    Cell cell(material);
    
    // Initialize the cell with the material's properties
    CellProcessor processor(&materialRegistry);
    processor.initializeCellFromMaterial(cell, material);
    
    // Mark the cell as updated to ensure it's active for at least one frame
    cell.updated = true;
    
    // Set the cell in the world
    chunkManager->setCell(x, y, cell);
    
    // Make sure the active area includes this cell
    WorldRect cellRect = {x, y, 1, 1};
    chunkManager->updateActiveChunks(cellRect);
}

void CellularAutomaton::updateSimulationStats()
{
    // Get active chunks
    const auto& activeChunks = chunkManager->getActiveChunks();
    
    // Reset stats
    stats.totalCells = 0;
    stats.activeCells = 0;
    stats.activeChunks = activeChunks.size();
    stats.averageTemp = 0.0f;
    stats.averagePressure = 0.0f;
    stats.materialCounts.clear();
    
    // Time taken for update (convert from seconds to milliseconds)
    stats.updateTimeMs = static_cast<float>(updateTimer.getDeltaTime() * 1000.0);
    
    // Count active cells and calculate averages
    int tempCellCount = 0;
    int pressureCellCount = 0;
    
    for (const auto& chunkCoord : activeChunks) {
        Chunk* chunk = chunkManager->getChunk(chunkCoord);
        if (!chunk) continue;
        
        for (int y = 0; y < CHUNK_SIZE; y++) {
            for (int x = 0; x < CHUNK_SIZE; x++) {
                const Cell& cell = chunk->getCell(x, y);
                stats.totalCells++;
                
                // Count cells that were updated this frame
                if (cell.updated) {
                    stats.activeCells++;
                }
                
                // Count by material type
                stats.materialCounts[cell.material]++;
                
                // Temperature average (skip empty cells)
                if (cell.material != materialRegistry.getDefaultMaterialID()) {
                    stats.averageTemp += cell.temperature;
                    tempCellCount++;
                }
                
                // Pressure average (only for fluids and gases)
                const MaterialProperties& props = materialRegistry.getMaterial(cell.material);
                if (props.type == MaterialType::LIQUID || props.type == MaterialType::GAS) {
                    stats.averagePressure += cell.pressure;
                    pressureCellCount++;
                }
            }
        }
    }
    
    // Calculate averages
    if (tempCellCount > 0) {
        stats.averageTemp /= tempCellCount;
    }
    
    if (pressureCellCount > 0) {
        stats.averagePressure /= pressureCellCount;
    }
}

MaterialID CellularAutomaton::registerMaterial(const MaterialProperties& properties)
{
    return materialRegistry.registerMaterial(properties);
}

MaterialProperties CellularAutomaton::getMaterial(MaterialID id) const
{
    return materialRegistry.getMaterial(id);
}

MaterialID CellularAutomaton::getMaterialIDByName(const std::string& name) const
{
    return materialRegistry.getIDFromName(name);
}

void CellularAutomaton::paintCell(int x, int y, MaterialID material)
{
    setCell(x, y, material);
}

void CellularAutomaton::paintLine(int x1, int y1, int x2, int y2, MaterialID material, int thickness)
{
    // Bresenham's line algorithm
    int dx = std::abs(x2 - x1);
    int dy = std::abs(y2 - y1);
    int sx = (x1 < x2) ? 1 : -1;
    int sy = (y1 < y2) ? 1 : -1;
    int err = dx - dy;
    
    while (true) {
        // Paint a cell at the current point
        if (thickness <= 1) {
            paintCell(x1, y1, material);
        } else {
            // For thicker lines, paint a circle at each point
            paintCircle(x1, y1, thickness / 2, material);
        }
        
        // Exit if we've reached the end point
        if (x1 == x2 && y1 == y2) break;
        
        // Calculate next point
        int e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x1 += sx;
        }
        if (e2 < dx) {
            err += dx;
            y1 += sy;
        }
    }
}

void CellularAutomaton::paintCircle(int x, int y, int radius, MaterialID material)
{
    // Check basic bounds first
    if (x < -radius || x >= worldWidth + radius || 
        y < -radius || y >= worldHeight + radius) {
        return;
    }
    
    // Fill a circle using midpoint circle algorithm
    for (int cy = -radius; cy <= radius; cy++) {
        for (int cx = -radius; cx <= radius; cx++) {
            if (cx*cx + cy*cy <= radius*radius) {
                int px = x + cx;
                int py = y + cy;
                
                // Check bounds for each point
                if (px >= 0 && px < worldWidth && py >= 0 && py < worldHeight) {
                    paintCell(px, py, material);
                }
            }
        }
    }
}

void CellularAutomaton::fillRectangle(int x, int y, int width, int height, MaterialID material)
{
    // Clamp to world boundaries
    int startX = std::max(0, x);
    int startY = std::max(0, y);
    int endX = std::min(worldWidth - 1, x + width - 1);
    int endY = std::min(worldHeight - 1, y + height - 1);
    
    // Fill the rectangle
    for (int cy = startY; cy <= endY; cy++) {
        for (int cx = startX; cx <= endX; cx++) {
            paintCell(cx, cy, material);
        }
    }
}

void CellularAutomaton::fillShape(int centerX, int centerY, int radius, MaterialID material)
{
    // This is a more general utility method used by paintCircle and fillRectangle
    // In this implementation, we're just mapping to paintCircle
    paintCircle(centerX, centerY, radius, material);
}

void CellularAutomaton::createExplosion(int x, int y, float radius, float power)
{
    physics->createExplosion(x, y, radius, power);
}

void CellularAutomaton::createHeatSource(int x, int y, float temperature, float radius)
{
    physics->createHeatSource(x, y, temperature, radius);
}

void CellularAutomaton::applyForce(int x, int y, const glm::vec2& direction, float strength, float radius)
{
    physics->applyForceField(x, y, direction, strength, radius);
}

void CellularAutomaton::setActiveArea(int x, int y, int width, int height)
{
    // Clamp to world boundaries
    activeArea.x = std::max(0, x);
    activeArea.y = std::max(0, y);
    activeArea.width = std::min(worldWidth - activeArea.x, width);
    activeArea.height = std::min(worldHeight - activeArea.y, height);
    
    // Update active chunks
    chunkManager->updateActiveChunks(activeArea);
}

void CellularAutomaton::clearWorld()
{
    // Fill the world with the default material (air)
    MaterialID airId = materialRegistry.getDefaultMaterialID();
    
    // For efficiency, we'll activate chunks first
    WorldRect fullWorld = {0, 0, worldWidth, worldHeight};
    chunkManager->updateActiveChunks(fullWorld);
    
    // Fill all active chunks with air
    const auto& activeChunks = chunkManager->getActiveChunks();
    for (const auto& chunkCoord : activeChunks) {
        Chunk* chunk = chunkManager->getChunk(chunkCoord);
        if (!chunk) continue;
        
        for (int y = 0; y < CHUNK_SIZE; y++) {
            for (int x = 0; x < CHUNK_SIZE; x++) {
                Cell emptyCell(airId);
                chunk->setCell(x, y, emptyCell);
            }
        }
    }
}

void CellularAutomaton::generateWorld(WorldTemplate tmpl)
{
    reset(tmpl);
}

void CellularAutomaton::initializeWorldFromTemplate(WorldTemplate tmpl)
{
    // Create a random number generator
    std::random_device rd;
    std::mt19937 gen(rd());
    
    // Clear existing world
    clearWorld();
    
    // Get material IDs for common materials
    MaterialID airId = materialRegistry.getDefaultMaterialID();
    MaterialID stoneId = materialRegistry.getStoneID();
    MaterialID sandId = materialRegistry.getSandID();
    MaterialID waterId = materialRegistry.getWaterID();
    
    // Generate world based on template
    switch (tmpl) {
        case WorldTemplate::EMPTY:
            // Already cleared
            break;
            
        case WorldTemplate::FLAT_TERRAIN: {
            // Create flat ground at bottom half of world
            int groundLevel = worldHeight / 2;
            
            // Fill bottom half with stone
            fillRectangle(0, groundLevel, worldWidth, worldHeight - groundLevel, stoneId);
            
            // Add layer of sand on top
            fillRectangle(0, groundLevel - 20, worldWidth, 20, sandId);
            break;
        }
            
        case WorldTemplate::TERRAIN_WITH_CAVES: {
            // Create terrain with perlin noise
            // (Simplified for this implementation)
            
            // Fill bottom half with stone first
            int groundLevel = worldHeight * 2 / 3;
            fillRectangle(0, groundLevel, worldWidth, worldHeight - groundLevel, stoneId);
            
            // Add some caves (simple randomly placed circles of air)
            std::uniform_int_distribution<> xDist(0, worldWidth);
            std::uniform_int_distribution<> yDist(groundLevel, worldHeight);
            std::uniform_int_distribution<> radiusDist(5, 30);
            
            for (int i = 0; i < 50; i++) {
                int caveX = xDist(gen);
                int caveY = yDist(gen);
                int caveRadius = radiusDist(gen);
                
                paintCircle(caveX, caveY, caveRadius, airId);
            }
            
            // Add a layer of sand on top
            fillRectangle(0, groundLevel - 15, worldWidth, 15, sandId);
            break;
        }
            
        case WorldTemplate::TERRAIN_WITH_WATER: {
            // Similar to flat terrain, but with water pools
            int groundLevel = worldHeight * 2 / 3;
            
            // Fill bottom half with stone
            fillRectangle(0, groundLevel, worldWidth, worldHeight - groundLevel, stoneId);
            
            // Add layer of sand on top
            fillRectangle(0, groundLevel - 15, worldWidth, 15, sandId);
            
            // Create a few water pools
            std::uniform_int_distribution<> xDist(50, worldWidth - 50);
            std::uniform_int_distribution<> widthDist(20, 100);
            std::uniform_int_distribution<> depthDist(10, 30);
            
            for (int i = 0; i < 3; i++) {
                int poolX = xDist(gen);
                int poolWidth = widthDist(gen);
                int poolDepth = depthDist(gen);
                
                int poolY = groundLevel - 15;
                fillRectangle(poolX - poolWidth/2, poolY - poolDepth, poolWidth, poolDepth, waterId);
            }
            break;
        }
            
        case WorldTemplate::RANDOM_MATERIALS: {
            // Fill world with random materials
            std::uniform_int_distribution<> xDist(0, worldWidth - 1);
            std::uniform_int_distribution<> yDist(0, worldHeight - 1);
            std::uniform_int_distribution<> radiusDist(5, 30);
            std::uniform_int_distribution<> materialCount(10, 100); // Number of random blobs
            
            int numMaterials = materialCount(gen);
            
            // Get all available material IDs
            std::vector<MaterialID> availableMaterials;
            // This would normally get the materials from the registry
            // For now, we'll just use known IDs
            availableMaterials.push_back(stoneId);
            availableMaterials.push_back(sandId);
            availableMaterials.push_back(waterId);
            
            if (!availableMaterials.empty()) {
                std::uniform_int_distribution<> materialDist(0, availableMaterials.size() - 1);
                
                for (int i = 0; i < numMaterials; i++) {
                    int x = xDist(gen);
                    int y = yDist(gen);
                    int radius = radiusDist(gen);
                    MaterialID matId = availableMaterials[materialDist(gen)];
                    
                    paintCircle(x, y, radius, matId);
                }
            }
            break;
        }
            
        case WorldTemplate::SANDBOX: {
            // Create a sandbox with solid walls and empty middle
            
            // Bottom wall
            fillRectangle(0, worldHeight - 50, worldWidth, 50, stoneId);
            
            // Left wall
            fillRectangle(0, 0, 50, worldHeight, stoneId);
            
            // Right wall
            fillRectangle(worldWidth - 50, 0, 50, worldHeight, stoneId);
            
            // Top wall
            fillRectangle(0, 0, worldWidth, 50, stoneId);
            
            // Add some sand at the bottom
            fillRectangle(100, worldHeight - 100, worldWidth - 200, 30, sandId);
            break;
        }
    }
}

bool CellularAutomaton::saveWorld(const std::string& filename) const
{
    // TODO: Implement world saving using JSON or binary format
    // For now, return a failure indicator
    std::cerr << "World saving not implemented yet" << std::endl;
    return false;
}

bool CellularAutomaton::loadWorld(const std::string& filename)
{
    // TODO: Implement world loading using JSON or binary format
    // For now, return a failure indicator
    std::cerr << "World loading not implemented yet" << std::endl;
    return false;
}

} // namespace astral