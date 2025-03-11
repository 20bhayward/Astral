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
    isActiveFlag = false;
    
    // For now, just a simple placeholder implementation
    // In a real implementation, we would check all cells based on material properties
    for (int y = 0; y < CHUNK_SIZE; y++) {
        for (int x = 0; x < CHUNK_SIZE; x++) {
            const Cell& cell = cells[y][x];
            
            // A cell is active if it has velocity or is not empty
            // This would be refined based on MaterialRegistry in a full implementation
            bool isActive = (cell.material != 0) && (cell.velocity.x != 0.0f || cell.velocity.y != 0.0f);
            
            activeCells[y][x] = isActive;
            if (isActive) {
                isActiveFlag = true;
            }
        }
    }
}

void Chunk::update(float deltaTime) {
    // Placeholder for physics update
    // In a real implementation, this would update all cells based on physics rules
    
    // For now, just update velocity on all cells to simulate some movement
    for (int y = 0; y < CHUNK_SIZE; y++) {
        for (int x = 0; x < CHUNK_SIZE; x++) {
            Cell& cell = cells[y][x];
            
            // Apply gravity to cells with non-zero material (not air)
            if (cell.material != 0) {
                cell.velocity.y += 9.8f * deltaTime; // Simple gravity
            }
        }
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
    // Calculate chunk range for the active area
    ChunkCoord minChunk = worldToChunkCoord(activeArea.x, activeArea.y);
    ChunkCoord maxChunk = worldToChunkCoord(
        activeArea.x + activeArea.width - 1,
        activeArea.y + activeArea.height - 1
    );
    
    // Clear active chunks list and refill
    activeChunks.clear();
    
    for (int y = minChunk.y; y <= maxChunk.y; y++) {
        for (int x = minChunk.x; x <= maxChunk.x; x++) {
            ChunkCoord coord = {x, y};
            
            Chunk* chunk = getChunk(coord);
            if (chunk && chunk->hasActiveCells()) {
                activeChunks.insert(coord);
            }
        }
    }
}

void ChunkManager::updateChunks(float deltaTime) {
    // Update all active chunks
    for (const auto& chunkCoord : activeChunks) {
        Chunk* chunk = getChunk(chunkCoord);
        if (chunk) {
            chunk->update(deltaTime);
        }
    }
    
    // Cleanup: remove inactive chunks that were active before
    std::vector<ChunkCoord> inactiveChunks;
    for (const auto& chunkCoord : activeChunks) {
        Chunk* chunk = getChunk(chunkCoord);
        if (chunk && !chunk->hasActiveCells()) {
            inactiveChunks.push_back(chunkCoord);
        }
    }
    
    for (const auto& chunkCoord : inactiveChunks) {
        activeChunks.erase(chunkCoord);
    }
}

bool ChunkManager::isValidCoord(WorldCoord coord) const {
    // For now, we'll consider any coordinate valid
    // In a real implementation, you might have world boundaries or other constraints
    return true;
}

} // namespace astral