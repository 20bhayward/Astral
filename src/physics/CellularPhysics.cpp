#include "astral/physics/CellularPhysics.h"
#include "astral/physics/Material.h"
#include <cstdlib>
#include <ctime>

namespace astral {

CellularPhysics::CellularPhysics(MaterialRegistry* registry, ChunkManager* chunkManager)
    : materialRegistry(registry)
    , chunkManager(chunkManager)
    , worldWidth(1024)  // Default size
    , worldHeight(1024) // Default size
{
    // Initialize random seed
    std::srand(static_cast<unsigned int>(std::time(nullptr)));
    
    // Initialize update tracking grid
    updated.resize(worldHeight, std::vector<bool>(worldWidth, false));
}

void CellularPhysics::setWorldDimensions(int width, int height)
{
    worldWidth = width;
    worldHeight = height;
    
    // Resize update tracking grid
    updated.resize(worldHeight, std::vector<bool>(worldWidth, false));
}

void CellularPhysics::update(float deltaTime)
{
    // Clear update tracking grid
    for (auto& row : updated) {
        std::fill(row.begin(), row.end(), false);
    }
    
    // Get active chunks from chunk manager
    const auto& activeChunks = chunkManager->getActiveChunks();
    
    // Update each active chunk
    for (const auto& chunkCoord : activeChunks) {
        Chunk* chunk = chunkManager->getChunk(chunkCoord);
        if (chunk) {
            updateChunk(chunk, deltaTime);
        }
    }
}

void CellularPhysics::updateChunk(Chunk* chunk, float deltaTime)
{
    if (!chunk) return;
    
    ChunkCoord chunkCoord = chunk->getCoord();
    
    // Bottom-up update for falling materials (powders, liquids)
    for (int y = CHUNK_SIZE - 1; y >= 0; y--) {
        for (int x = 0; x < CHUNK_SIZE; x++) {
            // Convert to world coordinates
            WorldCoord worldCoord = ChunkManager::chunkToWorldCoord(chunkCoord, {x, y});
            
            // Skip if already updated
            if (worldCoord.y < worldHeight && worldCoord.x < worldWidth &&
                updated[worldCoord.y][worldCoord.x]) {
                continue;
            }
            
            // Get cell and material
            Cell& cell = chunk->getCell(x, y);
            
            // Skip empty cells
            if (cell.material == 0) {
                continue;
            }
            
            // Basic placeholder implementation for different material types
            // In a full implementation, we would check material properties
            if (cell.material % 3 == 0) {
                // Powder-like materials
                updatePowder(worldCoord.x, worldCoord.y, deltaTime);
            } else if (cell.material % 3 == 1) {
                // Liquid-like materials
                updateLiquid(worldCoord.x, worldCoord.y, deltaTime);
            } else {
                // Gas-like materials
                updateGas(worldCoord.x, worldCoord.y, deltaTime);
            }
        }
    }
}

bool CellularPhysics::canMove(int x, int y, int newX, int newY)
{
    // Check if target is within world bounds
    if (newX < 0 || newX >= worldWidth || newY < 0 || newY >= worldHeight) {
        return false;
    }
    
    try {
        // Get target cell
        const Cell& targetCell = chunkManager->getCell(newX, newY);
        
        // Can move if target is empty
        if (targetCell.material == 0) {
            return true;
        }
        
        // Get source and target material properties
        // In a real implementation, we would check density and other properties
        // For now, just a simple placeholder
        return false;
    } catch (const std::exception& e) {
        // Error getting cell
        return false;
    }
}

void CellularPhysics::swapCells(int x, int y, int newX, int newY)
{
    // Get cells
    Cell& cell1 = chunkManager->getCell(x, y);
    Cell& cell2 = chunkManager->getCell(newX, newY);
    
    // Swap cells
    Cell temp = cell1;
    cell1 = cell2;
    cell2 = temp;
    
    // Mark as updated
    if (y < worldHeight && x < worldWidth) {
        updated[y][x] = true;
    }
    if (newY < worldHeight && newX < worldWidth) {
        updated[newY][newX] = true;
    }
}

void CellularPhysics::moveCell(int x, int y, int newX, int newY)
{
    // Get cells
    Cell& cell1 = chunkManager->getCell(x, y);
    Cell& cell2 = chunkManager->getCell(newX, newY);
    
    // Move cell
    cell2 = cell1;
    cell1 = Cell(); // Empty the source cell
    
    // Mark as updated
    if (y < worldHeight && x < worldWidth) {
        updated[y][x] = true;
    }
    if (newY < worldHeight && newX < worldWidth) {
        updated[newY][newX] = true;
    }
}

void CellularPhysics::updateEmpty(int x, int y, float deltaTime)
{
    // Empty cells don't need to be updated
}

void CellularPhysics::updateSolid(int x, int y, float deltaTime)
{
    // Solid cells don't move, but could interact with neighbors
    // This would be implemented in a full version
}

void CellularPhysics::updatePowder(int x, int y, float deltaTime)
{
    // Skip if already updated
    if (y >= worldHeight || x >= worldWidth || updated[y][x]) {
        return;
    }
    
    updated[y][x] = true;
    
    // Try to fall directly down
    if (canMove(x, y, x, y + 1)) {
        moveCell(x, y, x, y + 1);
        return;
    }
    
    // Random direction preference to avoid bias
    bool tryLeftFirst = std::rand() % 2 == 0;
    
    // Try to fall diagonally
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
}

void CellularPhysics::updateLiquid(int x, int y, float deltaTime)
{
    // Skip if already updated
    if (y >= worldHeight || x >= worldWidth || updated[y][x]) {
        return;
    }
    
    updated[y][x] = true;
    
    // Try to fall down
    if (canMove(x, y, x, y + 1)) {
        moveCell(x, y, x, y + 1);
        return;
    }
    
    // Random direction preference to avoid bias
    bool tryLeftFirst = std::rand() % 2 == 0;
    
    // Try to fall diagonally
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
    
    // Try to flow horizontally
    int dispersionDistance = 3; // Would be based on viscosity in a full implementation
    
    // Try left or right with random preference
    if (tryLeftFirst) {
        for (int i = 1; i <= dispersionDistance; i++) {
            if (canMove(x, y, x - i, y)) {
                moveCell(x, y, x - i, y);
                return;
            }
            if (i > 1) break; // Only check immediate neighbors first
        }
        
        for (int i = 1; i <= dispersionDistance; i++) {
            if (canMove(x, y, x + i, y)) {
                moveCell(x, y, x + i, y);
                return;
            }
            if (i > 1) break; // Only check immediate neighbors first
        }
    } else {
        for (int i = 1; i <= dispersionDistance; i++) {
            if (canMove(x, y, x + i, y)) {
                moveCell(x, y, x + i, y);
                return;
            }
            if (i > 1) break; // Only check immediate neighbors first
        }
        
        for (int i = 1; i <= dispersionDistance; i++) {
            if (canMove(x, y, x - i, y)) {
                moveCell(x, y, x - i, y);
                return;
            }
            if (i > 1) break; // Only check immediate neighbors first
        }
    }
}

void CellularPhysics::updateGas(int x, int y, float deltaTime)
{
    // Skip if already updated
    if (y >= worldHeight || x >= worldWidth || updated[y][x]) {
        return;
    }
    
    updated[y][x] = true;
    
    // Try to rise up (opposite of liquids/powders)
    if (canMove(x, y, x, y - 1)) {
        moveCell(x, y, x, y - 1);
        return;
    }
    
    // Random direction for diagonal rise
    bool tryLeftFirst = std::rand() % 2 == 0;
    
    // Try to rise diagonally
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
    
    // Horizontal dispersion
    int dispersionDistance = 4; // Would be based on dispersion in a full implementation
    
    // Try horizontal movement with random preference
    if (tryLeftFirst) {
        for (int i = 1; i <= dispersionDistance; i++) {
            if (canMove(x, y, x - i, y)) {
                moveCell(x, y, x - i, y);
                return;
            }
        }
        
        for (int i = 1; i <= dispersionDistance; i++) {
            if (canMove(x, y, x + i, y)) {
                moveCell(x, y, x + i, y);
                return;
            }
        }
    } else {
        for (int i = 1; i <= dispersionDistance; i++) {
            if (canMove(x, y, x + i, y)) {
                moveCell(x, y, x + i, y);
                return;
            }
        }
        
        for (int i = 1; i <= dispersionDistance; i++) {
            if (canMove(x, y, x - i, y)) {
                moveCell(x, y, x - i, y);
                return;
            }
        }
    }
}

void CellularPhysics::updateFire(int x, int y, float deltaTime)
{
    // Placeholder - would be implemented in a full version
}

void CellularPhysics::updateSpecial(int x, int y, float deltaTime)
{
    // Placeholder - would be implemented in a full version
}

void CellularPhysics::processMaterialInteraction(int x1, int y1, int x2, int y2)
{
    // Placeholder - would implement material interactions in a full version
}

void CellularPhysics::applyTemperature(int x, int y, float deltaTime)
{
    // Placeholder - would implement temperature effects in a full version
}

} // namespace astral