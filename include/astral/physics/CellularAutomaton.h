#pragma once

#include <vector>
#include <string>
#include <memory>
#include <unordered_map>
#include <functional>

#include "astral/physics/ChunkManager.h"
#include "astral/physics/CellularPhysics.h"
#include "astral/physics/Material.h"
#include "astral/core/Timer.h"

namespace astral {

/**
 * World generation templates for initializing cellular automaton simulations.
 */
enum class WorldTemplate {
    EMPTY,            // Empty world with just air
    FLAT_TERRAIN,     // Flat solid ground with air above
    TERRAIN_WITH_CAVES, // Terrain with cave system
    TERRAIN_WITH_WATER, // Terrain with water pools
    RANDOM_MATERIALS, // Random assortment of materials
    SANDBOX          // Empty space in middle surrounded by solid walls
};

/**
 * Brush types for painting cells in the world
 */
enum class BrushType {
    SINGLE_CELL,  // Single cell painting
    CIRCLE,       // Circle shape brush
    SQUARE,       // Square shape brush
    LINE          // Line between points
};

/**
 * Statistics about the current simulation state
 */
struct SimulationStats {
    int totalCells = 0;            // Total number of cells in active chunks
    int activeCells = 0;           // Number of cells that changed last frame
    int activeChunks = 0;          // Number of active chunks
    float averageTemp = 0.0f;      // Average temperature of active cells
    float averagePressure = 0.0f;  // Average pressure of active cells
    float updateTimeMs = 0.0f;     // Time taken for last update in milliseconds
    float fpsLimit = 60.0f;        // Current FPS limit for updates
    std::unordered_map<MaterialID, int> materialCounts; // Count of each material type
};

/**
 * CellularAutomaton is a high-level controller for cellular automaton simulations.
 * It manages the physics engine, material registry, and provides an interface for
 * simulation control and manipulation.
 */
class CellularAutomaton {
private:
    MaterialRegistry materialRegistry;
    std::unique_ptr<ChunkManager> chunkManager;
    std::unique_ptr<CellularPhysics> physics;
    
    bool isPaused;
    float timeScale;
    int worldWidth;
    int worldHeight;
    WorldRect activeArea;
    Timer updateTimer;
    SimulationStats stats;
    
    // Initialize simulation with a specific world template
    void initializeWorldFromTemplate(WorldTemplate tmpl);
    
    // Calculate simulation statistics
    void updateSimulationStats();
    
    // Utility method for placing materials in circle/rectangle patterns
    void fillShape(int centerX, int centerY, int radius, MaterialID material);
    
public:
    CellularAutomaton(int width = 1000, int height = 1000);
    ~CellularAutomaton();
    
    // Initialization
    void initialize();
    void reset(WorldTemplate tmpl = WorldTemplate::EMPTY);
    
    // Simulation control
    void update(float deltaTime);
    void pause() { isPaused = true; }
    void resume() { isPaused = false; }
    bool isSimulationPaused() const { return isPaused; }
    void setTimeScale(float scale) { timeScale = scale; }
    float getTimeScale() const { return timeScale; }
    
    // Cell access and modification
    Cell& getCell(int x, int y);
    const Cell& getCell(int x, int y) const;
    void setCell(int x, int y, const Cell& cell);
    void setCell(int x, int y, MaterialID material);
    
    // Material management
    MaterialID registerMaterial(const MaterialProperties& properties);
    MaterialProperties getMaterial(MaterialID id) const;
    MaterialID getMaterialIDByName(const std::string& name) const;
    
    // Painting tools
    void paintCell(int x, int y, MaterialID material);
    void paintLine(int x1, int y1, int x2, int y2, MaterialID material, int thickness = 1);
    void paintCircle(int x, int y, int radius, MaterialID material);
    void fillRectangle(int x, int y, int width, int height, MaterialID material);
    
    // Special effects
    void createExplosion(int x, int y, float radius, float power);
    void createHeatSource(int x, int y, float temperature, float radius);
    void applyForce(int x, int y, const glm::vec2& direction, float strength, float radius);
    
    // World generation
    void generateWorld(WorldTemplate tmpl);
    void clearWorld();
    
    // Simulation statistics
    const SimulationStats& getSimulationStats() const { return stats; }
    
    // World properties
    int getWorldWidth() const { return worldWidth; }
    int getWorldHeight() const { return worldHeight; }
    void setActiveArea(int x, int y, int width, int height);
    
    // Save/load world
    bool saveWorld(const std::string& filename) const;
    bool loadWorld(const std::string& filename);
};

} // namespace astral