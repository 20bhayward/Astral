#include <iostream>
#include <chrono>
#include <thread>
#include <string>
#include <vector>
#include <cmath>

#include "astral/physics/CellularAutomaton.h"
#include "astral/physics/Material.h"
#include "astral/core/Timer.h"

// Create a larger world to test chunk-based physics
const int WORLD_WIDTH = 256;  // Spans multiple chunks (64x64)
const int WORLD_HEIGHT = 128; // Spans multiple chunks (64x64)

// Structure to track a point for visualization
struct Point {
    int x, y;
    astral::MaterialID material;
};

// Function to display the world in the console with boundaries
void displayWorld(const astral::CellularAutomaton& automaton) {
    // Calculate a window for viewing a subset of the world
    int viewWidth = 80;  // Console width
    int viewHeight = 30; // Console height
    
    // Adjust view to show the interesting part of the world (massive sand and platform)
    int viewX = 30;  // Show more of the left side to see all the sand
    int viewY = 10;  // Show more of the top to see the sand falling
    
    // Show view coordinates
    std::cout << "Viewing area: (" << viewX << "," << viewY << ") to ("
              << (viewX + viewWidth - 1) << "," << (viewY + viewHeight - 1) << ")" << std::endl;
    
    // Draw top border
    std::cout << "+" << std::string(viewWidth, '-') << "+" << std::endl;
    
    // Print material legend
    std::cout << "| LEGEND: # = Stone, s = Sand, ~ = Water, o = Oil, L = Lava |" << std::endl;
    std::cout << "+" << std::string(viewWidth, '-') << "+" << std::endl;
    
    // Draw world cells
    for (int y = viewY; y < viewY + viewHeight; y++) {
        std::cout << "|";
        for (int x = viewX; x < viewX + viewWidth; x++) {
            // Skip if out of world bounds
            if (x < 0 || x >= WORLD_WIDTH || y < 0 || y >= WORLD_HEIGHT) {
                std::cout << " ";
                continue;
            }
            
            const astral::Cell& cell = automaton.getCell(x, y);
            
            // Draw cell based on material
            if (cell.material == automaton.getMaterialIDByName("Air")) {
                std::cout << " ";  // Empty
            } else if (cell.material == automaton.getMaterialIDByName("Sand")) {
                std::cout << "s";  // Sand
            } else if (cell.material == automaton.getMaterialIDByName("Water")) {
                std::cout << "~";  // Water
            } else if (cell.material == automaton.getMaterialIDByName("Stone")) {
                std::cout << "#";  // Stone
            } else if (cell.material == automaton.getMaterialIDByName("Oil")) {
                std::cout << "o";  // Oil
            } else if (cell.material == automaton.getMaterialIDByName("Lava")) {
                std::cout << "L";  // Lava
            } else {
                std::cout << "?";  // Unknown
            }
        }
        std::cout << "|" << std::endl;
    }
    
    // Draw bottom border
    std::cout << "+" << std::string(viewWidth, '-') << "+" << std::endl;
    
    // Draw chunk grid
    std::cout << "Chunk divisions (each chunk is 64x64):" << std::endl;
    int numChunksX = (WORLD_WIDTH + 63) / 64;  // Ceiling division
    int numChunksY = (WORLD_HEIGHT + 63) / 64; // Ceiling division
    
    for (int cy = 0; cy < numChunksY; cy++) {
        for (int cx = 0; cx < numChunksX; cx++) {
            std::cout << "(" << cx << "," << cy << ") ";
        }
        std::cout << std::endl;
    }
}

// Create interesting patterns across chunk boundaries
std::vector<Point> createSandTower(int centerX, int centerY, int height, int width) {
    std::vector<Point> points;
    
    // Create a massive amount of sand - fill a large portion of the top of the world
    for (int y = 0; y < 30; y++) {
        for (int x = 20; x < 100; x++) {
            if (x >= 0 && x < WORLD_WIDTH && y >= 0 && y < WORLD_HEIGHT) {
                points.push_back({x, y, 2}); // Sand
            }
        }
    }
    
    return points;
}

// Create a pool of water
std::vector<Point> createWaterPool(int centerX, int centerY, int width, int depth) {
    std::vector<Point> points;
    
    for (int y = centerY; y < centerY + depth; y++) {
        for (int x = centerX - width / 2; x <= centerX + width / 2; x++) {
            if (x >= 0 && x < WORLD_WIDTH && y >= 0 && y < WORLD_HEIGHT) {
                points.push_back({x, y, 3}); // Water
            }
        }
    }
    
    return points;
}

// Add stone walls
std::vector<Point> createStoneWalls() {
    std::vector<Point> points;
    
    // Bottom floor - cover entire bottom of world
    for (int x = 0; x < WORLD_WIDTH; x++) {
        points.push_back({x, WORLD_HEIGHT - 1, 1}); // Stone floor
    }
    
    // Lower platform to catch the falling sand
    for (int x = 30; x < 90; x++) {
        points.push_back({x, 40, 1}); // Stone platform
    }
    
    // Side walls
    for (int y = 0; y < WORLD_HEIGHT; y++) {
        points.push_back({0, y, 1}); // Left wall
        points.push_back({WORLD_WIDTH - 1, y, 1}); // Right wall
    }
    
    // Add some internal walls that cross chunk boundaries
    int midX = WORLD_WIDTH / 2;
    int midY = WORLD_HEIGHT / 2;
    
    // Horizontal wall in the middle
    for (int x = midX - 40; x <= midX + 40; x++) {
        if (x != midX - 10 && x != midX + 10) { // Gaps for materials to flow
            points.push_back({x, midY, 1});
        }
    }
    
    // Diagonal wall - spans across chunks
    for (int i = 0; i < 60; i++) {
        points.push_back({midX - 30 + i, midY + 20 - i/2, 1});
    }
    
    return points;
}

int main() {
    std::cout << "Large World Chunk Testing" << std::endl;
    std::cout << "World size: " << WORLD_WIDTH << "x" << WORLD_HEIGHT << " (spans multiple chunks)" << std::endl;
    
    // Initialize the automaton with a large world
    astral::CellularAutomaton automaton(WORLD_WIDTH, WORLD_HEIGHT);
    automaton.initialize();
    
    // Clear world
    automaton.clearWorld();
    
    // Add static structures
    std::vector<Point> walls = createStoneWalls();
    for (const auto& point : walls) {
        automaton.setCell(point.x, point.y, point.material);
    }
    
    // Create a massive amount of sand for stress testing
    std::vector<Point> massiveSand = createSandTower(0, 0, 0, 0); // Parameters not used in new implementation
    std::cout << "Creating " << massiveSand.size() << " sand particles for stress test..." << std::endl;
    
    for (const auto& point : massiveSand) {
        automaton.setCell(point.x, point.y, point.material);
    }
    
    // Add water pools
    std::vector<Point> pool1 = createWaterPool(WORLD_WIDTH / 4, 3 * WORLD_HEIGHT / 4, 30, 10);
    for (const auto& point : pool1) {
        automaton.setCell(point.x, point.y, point.material);
    }
    
    std::vector<Point> pool2 = createWaterPool(3 * WORLD_WIDTH / 4, 3 * WORLD_HEIGHT / 4, 20, 8);
    for (const auto& point : pool2) {
        automaton.setCell(point.x, point.y, automaton.getMaterialIDByName("Oil"));
    }
    
    // Display initial state
    std::cout << "Initial state:" << std::endl;
    displayWorld(automaton);
    std::cout << "Active chunks: " << automaton.getSimulationStats().activeChunks << std::endl;
    
    // Run simulation steps
    astral::Timer timer;
    float deltaTime = 0.1f; // Fixed time step
    
    for (int step = 1; step <= 30; step++) {
        // Update the simulation
        timer.reset();
        automaton.update(deltaTime);
        
        // Only display every 3 steps to reduce output volume
        if (step % 3 == 0) {
            std::cout << "\nStep " << step << ":" << std::endl;
            displayWorld(automaton);
            
            // Show some stats
            const auto& stats = automaton.getSimulationStats();
            std::cout << "Active chunks: " << stats.activeChunks << std::endl;
            std::cout << "Active cells: " << stats.activeCells << "/" << stats.totalCells << std::endl;
            std::cout << "Update time: " << stats.updateTimeMs << "ms" << std::endl;
            
            // Material counts
            std::cout << "Material counts: ";
            for (const auto& pair : stats.materialCounts) {
                if (pair.first > 0) { // Skip air
                    std::cout << pair.first << "=" << pair.second << " ";
                }
            }
            std::cout << std::endl;
        }
        
        // Pause for visibility
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    std::cout << "\nLarge world test complete!" << std::endl;
    return 0;
}