#pragma once

#include <unordered_map>
#include <vector>
#include <memory>
#include <set>
#include "astral/physics/Cell.h"

namespace astral {

struct ChunkCoord {
    int x;
    int y;
    
    bool operator==(const ChunkCoord& other) const {
        return x == other.x && y == other.y;
    }
};

struct WorldCoord {
    int x;
    int y;
    
    bool operator==(const WorldCoord& other) const {
        return x == other.x && y == other.y;
    }
};

struct LocalCoord {
    int x;
    int y;
    
    bool operator==(const LocalCoord& other) const {
        return x == other.x && y == other.y;
    }
};

struct WorldRect {
    int x;
    int y;
    int width;
    int height;
};

// Hash function for ChunkCoord
struct ChunkCoordHash {
    std::size_t operator()(const ChunkCoord& coord) const {
        return std::hash<int>()(coord.x) ^ (std::hash<int>()(coord.y) << 1);
    }
};

// Forward declarations
class MaterialRegistry;

constexpr int CHUNK_SIZE = 64; // 64x64 cells per chunk

/**
 * A chunk contains a grid of cells that make up a portion of the world.
 */
class Chunk {
private:
    ChunkCoord coord;
    Cell cells[CHUNK_SIZE][CHUNK_SIZE];
    bool isDirtyFlag;
    bool isActiveFlag;
    std::vector<std::vector<bool>> activeCells;
    MaterialRegistry* materialRegistry;
    
public:
    Chunk(ChunkCoord coord, MaterialRegistry* materialRegistry);
    ~Chunk() = default;
    
    // Cell access
    Cell& getCell(int x, int y);
    const Cell& getCell(int x, int y) const;
    void setCell(int x, int y, const Cell& cell);
    
    // Chunk properties
    ChunkCoord getCoord() const { return coord; }
    bool isDirty() const { return isDirtyFlag; }
    void markDirty() { isDirtyFlag = true; }
    void clearDirty() { isDirtyFlag = false; }
    
    bool isActive() const { return isActiveFlag; }
    void setActive(bool active) { isActiveFlag = active; }
    
    bool isCellActive(int x, int y) const;
    bool hasActiveCells() const;
    void updateActiveState();
    
    // Update physics in this chunk
    void update(float deltaTime);
};

/**
 * Manages chunks that make up the world, including creation, destruction,
 * and access to cells.
 */
class ChunkManager {
private:
    std::unordered_map<ChunkCoord, std::unique_ptr<Chunk>, ChunkCoordHash> chunks;
    std::set<ChunkCoord> activeChunks;
    MaterialRegistry* materialRegistry;
    
public:
    ChunkManager(MaterialRegistry* materialRegistry);
    ~ChunkManager() = default;
    
    // Chunk access
    Chunk* getChunk(ChunkCoord coord);
    Chunk* getOrCreateChunk(ChunkCoord coord);
    void removeChunk(ChunkCoord coord);
    
    // Cell access
    Cell& getCell(int worldX, int worldY);
    Cell& getCell(WorldCoord coord);
    const Cell& getCell(int worldX, int worldY) const;
    const Cell& getCell(WorldCoord coord) const;
    void setCell(int worldX, int worldY, const Cell& cell);
    void setCell(WorldCoord coord, const Cell& cell);
    
    // Coordinate conversion
    static ChunkCoord worldToChunkCoord(int worldX, int worldY);
    static ChunkCoord worldToChunkCoord(WorldCoord worldCoord);
    static LocalCoord worldToLocalCoord(int worldX, int worldY);
    static LocalCoord worldToLocalCoord(WorldCoord worldCoord);
    static WorldCoord chunkToWorldCoord(ChunkCoord chunkCoord, LocalCoord localCoord);
    
    // Active chunks
    const std::set<ChunkCoord>& getActiveChunks() const { return activeChunks; }
    void updateActiveChunks(const WorldRect& activeArea);
    void updateChunks(float deltaTime);
    
    // Utility
    bool isValidCoord(WorldCoord coord) const;
    int getChunkCount() const { return chunks.size(); }
    int getActiveChunkCount() const { return activeChunks.size(); }
};

} // namespace astral