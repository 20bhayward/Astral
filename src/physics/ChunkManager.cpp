#include "astral/physics/ChunkManager.h"
#include "astral/physics/Material.h"
#include <stdexcept>
#include <algorithm>

namespace astral {

// ==================== Chunk Implementation ====================

Chunk::Chunk(ChunkCoord coord, MaterialRegistry* materialRegistry)
    : coord(coord)
    , isDirtyFlag(true)
    , isActiveFlag(false)
    , materialRegistry(materialRegistry)
{
    // Initialize all cells to empty (air)
    for (int y = 0; y < CHUNK_SIZE; y++) {
        for (int x = 0; x < CHUNK_SIZE; x++) {
            cells[y][x] = Cell();
        }
    }
    
    // Initialize active cells tracking
    activeCells.resize(CHUNK_SIZE, std::vector<bool>(CHUNK_SIZE, false));
}

Cell& Chunk::getCell(int x, int y) {
    if (x < 0 || x >= CHUNK_SIZE || y < 0 || y >= CHUNK_SIZE) {
        throw std::out_of_range("Cell coordinates out of range");
    }
    return cells[y][x];
}

const Cell& Chunk::getCell(int x, int y) const {
    if (x < 0 || x >= CHUNK_SIZE || y < 0 || y >= CHUNK_SIZE) {
        throw std::out_of_range("Cell coordinates out of range");
    }
    return cells[y][x];
}

void Chunk::setCell(int x, int y, const Cell& cell) {
    if (x < 0 || x >= CHUNK_SIZE || y < 0 || y >= CHUNK_SIZE) {
        throw std::out_of_range("Cell coordinates out of range");
    }
    cells[y][x] = cell;
    markDirty();
}

bool Chunk::isCellActive(int x, int y) const {
    if (x < 0 || x >= CHUNK_SIZE || y < 0 || y >= CHUNK_SIZE) {
        return false;
    }
    return activeCells[y][x];
}

bool Chunk::hasActiveCells() const {
    return isActiveFlag;
}

void Chunk::updateActiveState() {
    // Always consider the chunk active for Lava Lake simulation
    isActiveFlag = true;
    
    // Mark all non-empty cells as active
    for (int y = 0; y < CHUNK_SIZE; y++) {
        for (int x = 0; x < CHUNK_SIZE; x++) {
            if (cells[y][x].material != 0) {
                activeCells[y][x] = true;
            } else {
                activeCells[y][x] = false;
            }
        }
    }
}

void Chunk::update(float deltaTime) {
    // Always set active if on chunk boundaries - to help with particles moving across chunks
    bool hasBoundaryCells = false;
    
    // Check cells at edges of chunk to see if they contain particles 
    // that might need to cross boundaries
    for (int y = 0; y < CHUNK_SIZE; y++) {
        for (int x = 0; x < CHUNK_SIZE; x++) {
            // Check only boundary cells (edges of the chunk)
            if (x == 0 || x == CHUNK_SIZE-1 || y == 0 || y == CHUNK_SIZE-1) {
                if (cells[y][x].material != 0) {
                    // Boundary has material - mark chunk as active
                    hasBoundaryCells = true;
                    
                    // For powder materials, explicitly force active
                    const MaterialProperties& props = materialRegistry->getMaterial(cells[y][x].material);
                    if (props.type == MaterialType::POWDER) {
                        setActive(true);
                        return;
                    }
                }
            }
        }
    }
    
    // If there are particles at boundaries, keep active
    if (hasBoundaryCells) {
        setActive(true);
        return;
    }
    
    // Update active state after physics simulation
    updateActiveState();
}

// ==================== ChunkManager Implementation ====================

ChunkManager::ChunkManager(MaterialRegistry* materialRegistry)
    : materialRegistry(materialRegistry)
{
}

Chunk* ChunkManager::getChunk(ChunkCoord coord) {
    auto it = chunks.find(coord);
    if (it != chunks.end()) {
        return it->second.get();
    }
    return nullptr;
}

Chunk* ChunkManager::getOrCreateChunk(ChunkCoord coord) {
    auto it = chunks.find(coord);
    if (it != chunks.end()) {
        return it->second.get();
    }
    
    // Create new chunk
    auto chunk = std::make_unique<Chunk>(coord, materialRegistry);
    Chunk* chunkPtr = chunk.get();
    chunks[coord] = std::move(chunk);
    
    return chunkPtr;
}

void ChunkManager::removeChunk(ChunkCoord coord) {
    chunks.erase(coord);
    activeChunks.erase(coord);
}

Cell& ChunkManager::getCell(int worldX, int worldY) {
    ChunkCoord chunkCoord = worldToChunkCoord(worldX, worldY);
    LocalCoord localCoord = worldToLocalCoord(worldX, worldY);
    
    Chunk* chunk = getOrCreateChunk(chunkCoord);
    return chunk->getCell(localCoord.x, localCoord.y);
}

Cell& ChunkManager::getCell(WorldCoord coord) {
    return getCell(coord.x, coord.y);
}

const Cell& ChunkManager::getCell(int worldX, int worldY) const {
    ChunkCoord chunkCoord = worldToChunkCoord(worldX, worldY);
    LocalCoord localCoord = worldToLocalCoord(worldX, worldY);
    
    auto it = chunks.find(chunkCoord);
    if (it == chunks.end()) {
        throw std::out_of_range("No chunk at the specified coordinates");
    }
    
    return it->second->getCell(localCoord.x, localCoord.y);
}

const Cell& ChunkManager::getCell(WorldCoord coord) const {
    return getCell(coord.x, coord.y);
}

void ChunkManager::setCell(int worldX, int worldY, const Cell& cell) {
    ChunkCoord chunkCoord = worldToChunkCoord(worldX, worldY);
    LocalCoord localCoord = worldToLocalCoord(worldX, worldY);
    
    Chunk* chunk = getOrCreateChunk(chunkCoord);
    chunk->setCell(localCoord.x, localCoord.y, cell);
    chunk->markDirty();
    
    // Add to active chunks if not already there
    activeChunks.insert(chunkCoord);
}

void ChunkManager::setCell(WorldCoord coord, const Cell& cell) {
    setCell(coord.x, coord.y, cell);
}

ChunkCoord ChunkManager::worldToChunkCoord(int worldX, int worldY) {
    // Handle negative coordinates correctly
    int chunkX = (worldX >= 0) ? (worldX / CHUNK_SIZE) : ((worldX - CHUNK_SIZE + 1) / CHUNK_SIZE);
    int chunkY = (worldY >= 0) ? (worldY / CHUNK_SIZE) : ((worldY - CHUNK_SIZE + 1) / CHUNK_SIZE);
    
    return {chunkX, chunkY};
}

ChunkCoord ChunkManager::worldToChunkCoord(WorldCoord worldCoord) {
    return worldToChunkCoord(worldCoord.x, worldCoord.y);
}

LocalCoord ChunkManager::worldToLocalCoord(int worldX, int worldY) {
    // Handle negative coordinates correctly
    int localX = worldX >= 0 ? (worldX % CHUNK_SIZE) : (CHUNK_SIZE + (worldX % CHUNK_SIZE)) % CHUNK_SIZE;
    int localY = worldY >= 0 ? (worldY % CHUNK_SIZE) : (CHUNK_SIZE + (worldY % CHUNK_SIZE)) % CHUNK_SIZE;
    
    return {localX, localY};
}

LocalCoord ChunkManager::worldToLocalCoord(WorldCoord worldCoord) {
    return worldToLocalCoord(worldCoord.x, worldCoord.y);
}

WorldCoord ChunkManager::chunkToWorldCoord(ChunkCoord chunkCoord, LocalCoord localCoord) {
    return {chunkCoord.x * CHUNK_SIZE + localCoord.x, chunkCoord.y * CHUNK_SIZE + localCoord.y};
}

void ChunkManager::updateActiveChunks(const WorldRect& activeArea) {
    // CRITICAL BUG FIX: The issue is that we're not activating ALL chunks in the world
    // This is causing materials to not move or interact
    
    // First, get all chunks that exist in the world
    std::vector<ChunkCoord> allChunks;
    for (const auto& pair : chunks) {
        allChunks.push_back(pair.first);
    }
    
    // Activate all chunks, regardless of their position or content
    for (const auto& coord : allChunks) {
        Chunk* chunk = getChunk(coord);
        if (chunk) {
            // Force the chunk to be active
            chunk->setActive(true);
            
            // Make sure all cells in the chunk are marked as updated
            for (int y = 0; y < CHUNK_SIZE; y++) {
                for (int x = 0; x < CHUNK_SIZE; x++) {
                    Cell& cell = chunk->getCell(x, y);
                    if (cell.material != 0) {
                        cell.updated = true;
                    }
                }
            }
            
            // Add to active chunks set
            activeChunks.insert(coord);
        }
    }
    
    // Create chunks for the active area if they don't exist
    ChunkCoord minChunk = worldToChunkCoord(activeArea.x, activeArea.y);
    ChunkCoord maxChunk = worldToChunkCoord(
        activeArea.x + activeArea.width - 1, 
        activeArea.y + activeArea.height - 1
    );
    
    for (int y = minChunk.y; y <= maxChunk.y; y++) {
        for (int x = minChunk.x; x <= maxChunk.x; x++) {
            ChunkCoord coord = {x, y};
            
            // Create chunk if it doesn't exist
            Chunk* chunk = getOrCreateChunk(coord);
            
            // Ensure it's active
            chunk->setActive(true);
            activeChunks.insert(coord);
        }
    }
}

void ChunkManager::updateChunks(float deltaTime) {
    // Update all active chunks
    for (const auto& chunkCoord : activeChunks) {
        Chunk* chunk = getChunk(chunkCoord);
        if (chunk) {
            // Set all non-empty cells active - THIS IS THE BUG FIX
            // We need this for every update to ensure cells with materials are ALWAYS processed
            for (int y = 0; y < CHUNK_SIZE; y++) {
                for (int x = 0; x < CHUNK_SIZE; x++) {
                    Cell& cell = chunk->getCell(x, y);
                    // The fix: Force material cells to be active by setting updated=true
                    if (cell.material != 0) {
                        cell.updated = true;
                    }
                }
            }
            
            // Mark the chunk as active
            chunk->setActive(true);
            
            // Update the chunk
            chunk->update(deltaTime);
        }
    }
    
    // NEVER remove chunks from active list or reset cell states
    // This was the bug - chunks need to stay active
}

bool ChunkManager::isValidCoord(WorldCoord coord) const {
    // For now, we'll consider any coordinate valid
    // In a real implementation, you might have world boundaries or other constraints
    return true;
}

} // namespace astral