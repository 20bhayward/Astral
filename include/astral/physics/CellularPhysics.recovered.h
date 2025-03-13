#pragma once

#include "astral/physics/PhysicsSystem.h"
#include "astral/physics/ChunkManager.h"
#include "astral/physics/Material.h"
#include "astral/core/ThreadPool.h"
#include <memory>
#include <iostream>

namespace astral {

class CellularAutomatonPhysics : public PhysicsSystem {
private:
    std::unique_ptr<MaterialRegistry> materialRegistry;
    std::unique_ptr<ChunkManager> chunkManager;
    std::unique_ptr<ThreadPool> threadPool;

public:
    CellularAutomatonPhysics()
        : materialRegistry(nullptr)
        , chunkManager(nullptr)
        , threadPool(nullptr)
    {
    }

    ~CellularAutomatonPhysics() override {
        shutdown();
    }

    void initialize() override {
        std::cout << "Initializing cellular automaton physics system..." << std::endl;
        std::cout << "Debug: Setting up material registry and creating world..." << std::endl;

        // Create material registry
        materialRegistry = std::make_unique<MaterialRegistry>();
        
        // Register some basic materials
        MaterialProperties air;
        air.name = "Air";
        air.color = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);
        air.type = MaterialType::EMPTY;
        air.density = 0.0f;
        uint16_t airID = materialRegistry->registerMaterial(air);
        
        MaterialProperties sand;
        sand.name = "Sand";
        sand.color = glm::vec4(0.76f, 0.7f, 0.5f, 1.0f);
        sand.type = MaterialType::POWDER;
        sand.density = 1.5f;
        uint16_t sandID = materialRegistry->registerMaterial(sand);
        
        MaterialProperties stone;
        stone.name = "Stone";
        stone.color = glm::vec4(0.5f, 0.5f, 0.5f, 1.0f);
        stone.type = MaterialType::SOLID;
        stone.density = 2.6f;
        uint16_t stoneID = materialRegistry->registerMaterial(stone);
        
        MaterialProperties water;
        water.name = "Water";
        water.color = glm::vec4(0.2f, 0.3f, 0.8f, 0.7f);
        water.type = MaterialType::LIQUID;
        water.density = 1.0f;
        uint16_t waterID = materialRegistry->registerMaterial(water);
        
        MaterialProperties lava;
        lava.name = "Lava";
        lava.color = glm::vec4(0.9f, 0.3f, 0.1f, 0.9f);
        lava.type = MaterialType::LIQUID;
        lava.density = 1.8f;
        lava.emissive = true;
        lava.emissiveStrength = 2.0f; // Strong emissive property
        uint16_t lavaID = materialRegistry->registerMaterial(lava);
        
        // Add fire material (emissive)
        MaterialProperties fire;
        fire.name = "Fire";
        fire.color = glm::vec4(1.0f, 0.6f, 0.2f, 0.9f);
        fire.type = MaterialType::FIRE;
        fire.density = 0.2f;
        fire.emissive = true;
        fire.emissiveStrength = 3.0f; // Very strong emissive property
        uint16_t fireID = materialRegistry->registerMaterial(fire);
        
        // Add glowing crystal material (emissive)
        MaterialProperties crystal;
        crystal.name = "GlowingCrystal";
        crystal.color = glm::vec4(0.4f, 0.7f, 1.0f, 1.0f);
        crystal.type = MaterialType::SOLID;
        crystal.density = 2.0f;
        crystal.emissive = true;
        crystal.emissiveStrength = 1.5f; // Medium emissive property
        uint16_t crystalID = materialRegistry->registerMaterial(crystal);
        
        // Create chunk manager
        chunkManager = std::make_unique<ChunkManager>(materialRegistry.get());
        
        // Create a simple demo world
        createDemoWorld(airID, sandID, stoneID, waterID, lavaID, fireID, crystalID);
        
        std::cout << "Debug: World creation complete. Created with materials: " << std::endl;
        std::cout << "  Air ID: " << airID << std::endl;
        std::cout << "  Sand ID: " << sandID << std::endl;
        std::cout << "  Stone ID: " << stoneID << std::endl;
        std::cout << "  Water ID: " << waterID << std::endl;
        std::cout << "  Lava ID: " << lavaID << std::endl;
        std::cout << "  Fire ID: " << fireID << std::endl;
        std::cout << "  GlowingCrystal ID: " << crystalID << std::endl;
    }

    void update(double deltaTime) override {
        // Create a thread pool for parallel processing if not already created
        if (!threadPool) {
            threadPool = std::make_unique<ThreadPool>();
            chunkManager->setThreadPool(threadPool.get());
        }
        
        // Convert deltaTime to float for ChunkManager
        float dt = static_cast<float>(deltaTime);
        
        // Define the active area for physics simulation
        WorldRect activeArea;
        activeArea.x = 0;
        activeArea.y = 0;
        activeArea.width = 4 * CHUNK_SIZE;  // 4 chunks wide
        activeArea.height = 4 * CHUNK_SIZE; // 4 chunks tall
        
        // Update active chunks based on the active area
        chunkManager->updateActiveChunks(activeArea);
        
        // Run the actual physics simulation using parallel processing
        chunkManager->updateChunksParallel(dt);
        
        static bool firstUpdate = true;
        if (firstUpdate) {
            std::cout << "Debug: Activated " << chunkManager->getActiveChunkCount() << " chunks" << std::endl;
            std::cout << "Debug: Total chunks in manager: " << chunkManager->getChunkCount() << std::endl;
            std::cout << "Debug: Active chunks count: " << chunkManager->getActiveChunkCount() << std::endl;
            firstUpdate = false;
        }
    }

    void shutdown() override {
        std::cout << "Shutting down cellular automaton physics system..." << std::endl;
        chunkManager.reset();
        materialRegistry.reset();
        threadPool.reset();
    }
    
    // Accessors for rendering
    const ChunkManager* getChunkManager() const override {
        return chunkManager.get();
    }
    
    const MaterialRegistry* getMaterialRegistry() const override {
        return materialRegistry.get();
    }
    
private:
    void createDemoWorld(uint16_t airID, uint16_t sandID, uint16_t stoneID, uint16_t waterID, uint16_t lavaID, uint16_t fireID, uint16_t crystalID) {
        if (!chunkManager || !materialRegistry) return;
        
        std::cout << "Debug: Creating demo world..." << std::endl;
        
        // Force creation of chunks in a small visible area first
        for (int x = 0; x < 4; x++) {
            for (int y = 0; y < 4; y++) {
                ChunkCoord coord;
                coord.x = x;
                coord.y = y;
                
                // This will create the chunk if it doesn't exist
                Chunk* chunk = chunkManager->getOrCreateChunk(coord);
                if (chunk) {
                    std::cout << "Debug: Created chunk at " << x << "," << y << std::endl;
                    
                    // Initialize chunk with air
                    for (int cellY = 0; cellY < CHUNK_SIZE; cellY++) {
                        for (int cellX = 0; cellX < CHUNK_SIZE; cellX++) {
                            Cell cell;
                            cell.material = airID;
                            cell.temperature = 20.0f;
                            
                            // Calculate world coordinates and use setCell on ChunkManager
                            int worldX = coord.x * CHUNK_SIZE + cellX;
                            int worldY = coord.y * CHUNK_SIZE + cellY;
                            chunkManager->setCell(worldX, worldY, cell);
                        }
                    }
                    
                    // The chunk should now be in the activeChunks set
                    // Let's explicitly mark it as active too
                    chunk->setActive(true);
                }
            }
        }
        
        // Create a Noita-inspired world with caverns and interactive materials
        
        // 1. Create a complex ground terrain with stone
        
        // Generate perlin noise for terrain
        auto noise = [](float x) -> float {
            return 0.5f + 0.5f * std::sin(x * 0.05f) + 0.25f * std::sin(x * 0.1f);
        };
        
        // Ground terrain with varied height
        for (int x = 0; x < 4 * CHUNK_SIZE; x++) {
            // Calculate ground height using noise function
            // Note: In our world, up is +y and down is -y
            int groundHeight = CHUNK_SIZE + 20 + static_cast<int>(30 * noise(x));
            
            // Fill everything below ground with stone
            for (int y = 0; y < groundHeight; y++) {
                Cell cell;
                
                // Add some variation in material (stone with pockets of sand)
                if (y < groundHeight - 2 && (std::hash<int>{}(x * 65537 + y * 257) % 10) < 2) {
                    cell.material = sandID;
                } else {
                    cell.material = stoneID;
                }
                
                cell.temperature = 20.0f;
                chunkManager->setCell(x, y, cell);
            }
        }
        
        // 2. Create caves
        for (int i = 0; i < 8; i++) {
            // Random cave center
            int caveX = 20 + (std::hash<int>{}(i * 123) % (4 * CHUNK_SIZE - 40));
            int caveY = 40 + (std::hash<int>{}(i * 789) % 150);
            int caveRadius = 10 + (std::hash<int>{}(i * 456) % 20);
            
            // Create circular cave
            for (int y = caveY - caveRadius; y <= caveY + caveRadius; y++) {
                for (int x = caveX - caveRadius; x <= caveX + caveRadius; x++) {
                    if (x >= 0 && x < 4 * CHUNK_SIZE && y >= 0 && y < 4 * CHUNK_SIZE) {
                        if (std::pow(x - caveX, 2) + std::pow(y - caveY, 2) < std::pow(caveRadius, 2)) {
                            Cell cell;
                            cell.material = airID;
                            cell.temperature = 20.0f;
                            chunkManager->setCell(x, y, cell);
                        }
                    }
                }
            }
            
            // 50% chance to fill cave with water or lava
            if (std::hash<int>{}(i * 333) % 4 != 0) {
                bool useLava = std::hash<int>{}(i * 999) % 5 == 0; // 20% chance for lava
                
                // Fill the bottom part of the cave
                for (int y = caveY; y <= caveY + caveRadius; y++) {
                    for (int x = caveX - caveRadius; x <= caveX + caveRadius; x++) {
                        if (x >= 0 && x < 4 * CHUNK_SIZE && y >= 0 && y < 4 * CHUNK_SIZE) {
                            if (std::pow(x - caveX, 2) + std::pow(y - caveY, 2) < std::pow(caveRadius, 2)) {
                                Cell cell;
                                if (useLava) {
                                    cell.material = lavaID;
                                    cell.temperature = 800.0f;
                                    cell.energy = 10.0f;
                                } else {
                                    cell.material = waterID;
                                    cell.temperature = 15.0f;
                                    cell.velocity = glm::vec2(0.0f, 0.01f);
                                }
                                chunkManager->setCell(x, y, cell);
                            }
                        }
                    }
                }
            }
        }
        
        // 4. Add fire in various places
        for (int i = 0; i < 10; i++) {
            int fireX = 30 + (std::hash<int>{}(i * 555) % (4 * CHUNK_SIZE - 60));
            int fireY = 80 + (std::hash<int>{}(i * 777) % 100);
            int fireRadius = 3 + (std::hash<int>{}(i * 888) % 5);
            
            // Create fire
            for (int y = fireY - fireRadius; y <= fireY + fireRadius; y++) {
                for (int x = fireX - fireRadius; x <= fireX + fireRadius; x++) {
                    if (x >= 0 && x < 4 * CHUNK_SIZE && y >= 0 && y < 4 * CHUNK_SIZE) {
                        if (std::pow(x - fireX, 2) + std::pow(y - fireY, 2) < std::pow(fireRadius, 2)) {
                            Cell cell;
                            cell.material = fireID;
                            cell.temperature = 800.0f;
                            cell.energy = 8.0f;
                            chunkManager->setCell(x, y, cell);
                        }
                    }
                }
            }
        }
        
        // 5. Add glowing crystals embedded in walls
        for (int i = 0; i < 15; i++) {
            int crystalX = 20 + (std::hash<int>{}(i * 123 + 42) % (4 * CHUNK_SIZE - 40));
            int crystalY = 30 + (std::hash<int>{}(i * 789 + 42) % 150);
            int crystalSize = 2 + (std::hash<int>{}(i * 456 + 42) % 3);
            
            // Create crystal cluster
            for (int y = crystalY - crystalSize; y <= crystalY + crystalSize; y++) {
                for (int x = crystalX - crystalSize; x <= crystalX + crystalSize; x++) {
                    if (x >= 0 && x < 4 * CHUNK_SIZE && y >= 0 && y < 4 * CHUNK_SIZE) {
                        // Make crystal shape irregular
                        float distance = std::sqrt(std::pow(x - crystalX, 2) + std::pow(y - crystalY, 2));
                        // Add some noise for irregularity
                        float noise = std::sin(x * 0.5f) * std::cos(y * 0.5f) * 0.5f;
                        if (distance + noise < crystalSize) {
                            Cell cell;
                            cell.material = crystalID;
                            cell.temperature = 20.0f;
                            chunkManager->setCell(x, y, cell);
                        }
                    }
                }
            }
        }
        
        // 3. Create a tunnel system connecting caves
        for (int i = 0; i < 15; i++) {
            int startX = 20 + (std::hash<int>{}(i * 111) % (4 * CHUNK_SIZE - 40));
            int startY = 40 + (std::hash<int>{}(i * 222) % 150);
            int endX = 20 + (std::hash<int>{}(i * 333) % (4 * CHUNK_SIZE - 40));
            int endY = 40 + (std::hash<int>{}(i * 444) % 150);
            int tunnelWidth = 3 + (std::hash<int>{}(i * 555) % 5);
            
            // Create a winding tunnel
            int steps = 50;
            int lastX = startX;
            int lastY = startY;
            
            for (int step = 0; step <= steps; step++) {
                float t = static_cast<float>(step) / steps;
                // Add some randomness to the path
                float noiseX = 15.0f * std::sin(t * 6.0f + i);
                float noiseY = 15.0f * std::sin(t * 5.0f + i * 2);
                
                int x = static_cast<int>(startX + t * (endX - startX) + noiseX);
                int y = static_cast<int>(startY + t * (endY - startY) + noiseY);
                
                // Draw a line from last position to current
                int dx = x - lastX;
                int dy = y - lastY;
                int steps = std::max(std::abs(dx), std::abs(dy));
                
                for (int j = 0; j <= steps; j++) {
                    float t = steps == 0 ? 0.0f : static_cast<float>(j) / steps;
                    int px = lastX + static_cast<int>(t * dx);
                    int py = lastY + static_cast<int>(t * dy);
                    
                    // Create a circular tunnel segment
                    for (int ty = py - tunnelWidth; ty <= py + tunnelWidth; ty++) {
                        for (int tx = px - tunnelWidth; tx <= px + tunnelWidth; tx++) {
                            if (tx >= 0 && tx < 4 * CHUNK_SIZE && ty >= 0 && ty < 4 * CHUNK_SIZE) {
                                if (std::pow(tx - px, 2) + std::pow(ty - py, 2) < std::pow(tunnelWidth, 2)) {
                                    Cell cell;
                                    cell.material = airID;
                                    cell.temperature = 20.0f;
                                    chunkManager->setCell(tx, ty, cell);
                                }
                            }
                        }
                    }
                }
                
                lastX = x;
                lastY = y;
            }
        }
        
        // 4. Add sand, water, and lava reservoirs that will collapse/flow
        
        // Sand reservoirs
        for (int i = 0; i < 5; i++) {
            int centerX = 30 + (std::hash<int>{}(i * 111) % (4 * CHUNK_SIZE - 60));
            int centerY = 180 + (std::hash<int>{}(i * 222) % 100);
            int width = 15 + (std::hash<int>{}(i * 333) % 10);
            int height = 10 + (std::hash<int>{}(i * 444) % 15);
            
            // Create rectangular sand reservoir supported by stone
            for (int y = centerY - height/2; y <= centerY + height/2; y++) {
                for (int x = centerX - width/2; x <= centerX + width/2; x++) {
                    if (x >= 0 && x < 4 * CHUNK_SIZE && y >= 0 && y < 4 * CHUNK_SIZE) {
                        Cell cell;
                        cell.material = sandID;
                        cell.temperature = 20.0f;
                        chunkManager->setCell(x, y, cell);
                    }
                }
            }
            
            // Support pillars - going down in the +Y world
            for (int x = centerX - width/2; x <= centerX + width/2; x += width) {
                for (int y = centerY - height/2 - 1; y >= 0; y--) {
                    if (y >= 0) {
                        Cell cell;
                        cell.material = stoneID;
                        cell.temperature = 20.0f;
                        chunkManager->setCell(x, y, cell);
                    }
                }
            }
        }
        
        // Water reservoirs
        for (int i = 0; i < 4; i++) {
            int centerX = 50 + (std::hash<int>{}(i * 555) % (4 * CHUNK_SIZE - 100));
            int centerY = 160 + (std::hash<int>{}(i * 666) % 80);
            int width = 20 + (std::hash<int>{}(i * 777) % 15);
            int height = 15 + (std::hash<int>{}(i * 888) % 10);
            
            // Create rectangular water reservoir contained by stone
            for (int y = centerY - height/2; y <= centerY + height/2; y++) {
                for (int x = centerX - width/2; x <= centerX + width/2; x++) {
                    if (x >= 0 && x < 4 * CHUNK_SIZE && y >= 0 && y < 4 * CHUNK_SIZE) {
                        Cell cell;
                        
                        // Water in the middle, stone walls around
                        if (x == centerX - width/2 || x == centerX + width/2 || 
                            y == centerY - height/2 || y == centerY + height/2) {
                            cell.material = stoneID;
                            cell.temperature = 20.0f;
                        } else {
                            cell.material = waterID;
                            cell.temperature = 15.0f;
                            cell.velocity = glm::vec2(0.0f, 0.0f);
                        }
                        
                        chunkManager->setCell(x, y, cell);
                    }
                }
            }
            
            // Add a small leak at the bottom of 50% of the reservoirs
            if (std::hash<int>{}(i * 999) % 2 == 0) {
                int leakX = centerX + (std::hash<int>{}(i * 123) % width - width/2);
                int leakY = centerY + height/2;
                
                Cell cell;
                cell.material = airID;
                cell.temperature = 15.0f;
                chunkManager->setCell(leakX, leakY, cell);
            }
        }
        
        // Lava reservoir (just one, near the ground)
        {
            int centerX = CHUNK_SIZE + (std::hash<int>{}(42) % (2 * CHUNK_SIZE));
            int centerY = 50;
            int width = 30;
            int height = 20;
            
            // Create rectangular lava reservoir contained by stone
            for (int y = centerY - height/2; y <= centerY + height/2; y++) {
                for (int x = centerX - width/2; x <= centerX + width/2; x++) {
                    if (x >= 0 && x < 4 * CHUNK_SIZE && y >= 0 && y < 4 * CHUNK_SIZE) {
                        Cell cell;
                        
                        // Lava in the middle, stone walls around
                        if (x == centerX - width/2 || x == centerX + width/2 || 
                            y == centerY - height/2 || y == centerY + height/2) {
                            cell.material = stoneID;
                            cell.temperature = 20.0f;
                        } else {
                            cell.material = lavaID;
                            cell.temperature = 800.0f;
                            cell.energy = 10.0f;
                        }
                        
                        chunkManager->setCell(x, y, cell);
                    }
                }
            }
            
            // Add a pipeline from lava to the higher area
            int pipeX = centerX;
            for (int y = centerY + height/2 + 1; y <= centerY + height/2 + 50; y++) {
                // Create pipe walls
                for (int dx = -2; dx <= 2; dx++) {
                    int x = pipeX + dx;
                    if (x >= 0 && x < 4 * CHUNK_SIZE && y >= 0 && y < 4 * CHUNK_SIZE) {
                        Cell cell;
                        
                        if (dx == -2 || dx == 2) {
                            cell.material = stoneID;
                            cell.temperature = 20.0f;
                        } else {
                            cell.material = airID;
                            cell.temperature = 20.0f;
                        }
                        
                        chunkManager->setCell(x, y, cell);
                    }
                }
            }
        }
        
        // 5. Add a test sandbox area in the bottom-left for player experimentation
        {
            int boxX = 10;
            int boxY = 200;
            int boxWidth = CHUNK_SIZE;
            int boxHeight = CHUNK_SIZE;
            
            // Create stone walls
            for (int y = boxY; y < boxY + boxHeight; y++) {
                for (int x = boxX; x < boxX + boxWidth; x++) {
                    if (x == boxX || x == boxX + boxWidth - 1 || 
                        y == boxY || y == boxY + boxHeight - 1) {
                        Cell cell;
                        cell.material = stoneID;
                        cell.temperature = 20.0f;
                        chunkManager->setCell(x, y, cell);
                    }
                }
            }
            
            // Create material display for testing
            int materialSpacing = 10;
            
            // Sand pile
            for (int y = boxY + 20; y < boxY + 40; y++) {
                for (int x = boxX + materialSpacing; x < boxX + materialSpacing + 10; x++) {
                    Cell cell;
                    cell.material = sandID;
                    cell.temperature = 20.0f;
                    chunkManager->setCell(x, y, cell);
                }
            }
            
            // Water pool
            for (int y = boxY + 20; y < boxY + 30; y++) {
                for (int x = boxX + 2*materialSpacing; x < boxX + 2*materialSpacing + 10; x++) {
                    Cell cell;
                    cell.material = waterID;
                    cell.temperature = 15.0f;
                    chunkManager->setCell(x, y, cell);
                }
            }
            
            // Lava pool
            for (int y = boxY + 20; y < boxY + 30; y++) {
                for (int x = boxX + 3*materialSpacing; x < boxX + 3*materialSpacing + 10; x++) {
                    Cell cell;
                    cell.material = lavaID;
                    cell.temperature = 800.0f;
                    cell.energy = 10.0f;
                    chunkManager->setCell(x, y, cell);
                }
            }
        }
        
        std::cout << "Debug: After creating chunks - Total: " << chunkManager->getChunkCount() 
                  << ", Active: " << chunkManager->getActiveChunkCount() << std::endl;
    }
};

} // namespace astral
