#include <iostream>
#include <chrono>
#include <thread>
#include <string>
#include <vector>
#include <cmath>
#include <limits>

#include "astral/physics/CellularAutomaton.h"
#include "astral/physics/Material.h"
#include "astral/core/Timer.h"

// Create a smaller world optimized for water visualization
const int WORLD_WIDTH = 100;
const int WORLD_HEIGHT = 50;

// Structure to track a point for visualization
struct Point {
    int x, y;
    astral::MaterialID material;
};

// Display the world with a legend for materials
void displayWorld(const astral::CellularAutomaton& automaton) {
    std::cout << "+" << std::string(WORLD_WIDTH, '-') << "+" << std::endl;
    
    // Print material legend
    std::cout << "| LEGEND: # = Stone, s = Sand, ~ = Water, o = Oil, L = Lava, + = Wood, * = Steam, @ = Smoke |" << std::endl;
    std::cout << "+" << std::string(WORLD_WIDTH, '-') << "+" << std::endl;
    
    for (int y = 0; y < WORLD_HEIGHT; y++) {
        std::cout << "|";
        for (int x = 0; x < WORLD_WIDTH; x++) {
            const astral::Cell& cell = automaton.getCell(x, y);
            
            // Display material with different characters
            if (cell.material == automaton.getMaterialIDByName("Air")) {
                std::cout << " ";
            } else if (cell.material == automaton.getMaterialIDByName("Sand")) {
                std::cout << "s";
            } else if (cell.material == automaton.getMaterialIDByName("Water")) {
                std::cout << "~";
            } else if (cell.material == automaton.getMaterialIDByName("Oil")) {
                std::cout << "o";
            } else if (cell.material == automaton.getMaterialIDByName("Stone")) {
                std::cout << "#";
            } else if (cell.material == automaton.getMaterialIDByName("Wood")) {
                std::cout << "+";
            } else if (cell.material == automaton.getMaterialIDByName("Fire")) {
                std::cout << "F";
            } else if (cell.material == automaton.getMaterialIDByName("Lava")) {
                std::cout << "L";
            } else if (cell.material == automaton.getMaterialIDByName("Steam")) {
                std::cout << "*";
            } else if (cell.material == automaton.getMaterialIDByName("Smoke")) {
                std::cout << "@";
            } else {
                std::cout << "?";
            }
        }
        std::cout << "|" << std::endl;
    }
    
    std::cout << "+" << std::string(WORLD_WIDTH, '-') << "+" << std::endl;
}

// Create test 1: Water pooling and flowing test
void setupPoolingTest(astral::CellularAutomaton& automaton) {
    // Clear the world
    automaton.clearWorld();
    
    // Create solid bottom
    for (int x = 0; x < WORLD_WIDTH; x++) {
        automaton.setCell(x, WORLD_HEIGHT - 1, automaton.getMaterialIDByName("Stone"));
    }
    
    // Create walls
    for (int y = 0; y < WORLD_HEIGHT; y++) {
        automaton.setCell(0, y, automaton.getMaterialIDByName("Stone"));
        automaton.setCell(WORLD_WIDTH - 1, y, automaton.getMaterialIDByName("Stone"));
    }
    
    // Create stepped terrain for water to flow over
    for (int x = 10; x < 30; x++) {
        for (int y = WORLD_HEIGHT - 10; y < WORLD_HEIGHT; y++) {
            automaton.setCell(x, y, automaton.getMaterialIDByName("Stone"));
        }
    }
    
    for (int x = 30; x < 50; x++) {
        for (int y = WORLD_HEIGHT - 15; y < WORLD_HEIGHT; y++) {
            automaton.setCell(x, y, automaton.getMaterialIDByName("Stone"));
        }
    }
    
    for (int x = 70; x < 90; x++) {
        for (int y = WORLD_HEIGHT - 5; y < WORLD_HEIGHT; y++) {
            automaton.setCell(x, y, automaton.getMaterialIDByName("Stone"));
        }
    }
    
    // Add a water source
    for (int x = 5; x < 10; x++) {
        for (int y = 5; y < 10; y++) {
            automaton.setCell(x, y, automaton.getMaterialIDByName("Water"));
        }
    }
}

// Create test 2: U-shaped container to test water leveling
void setupLevelingTest(astral::CellularAutomaton& automaton) {
    // Clear the world
    automaton.clearWorld();
    
    // Create solid bottom
    for (int x = 0; x < WORLD_WIDTH; x++) {
        automaton.setCell(x, WORLD_HEIGHT - 1, automaton.getMaterialIDByName("Stone"));
    }
    
    // Create walls
    for (int y = 0; y < WORLD_HEIGHT; y++) {
        automaton.setCell(0, y, automaton.getMaterialIDByName("Stone"));
        automaton.setCell(WORLD_WIDTH - 1, y, automaton.getMaterialIDByName("Stone"));
    }
    
    // Create U-shaped container
    // Left wall
    for (int y = WORLD_HEIGHT - 20; y < WORLD_HEIGHT; y++) {
        for (int x = 20; x < 25; x++) {
            automaton.setCell(x, y, automaton.getMaterialIDByName("Stone"));
        }
    }
    
    // Right wall
    for (int y = WORLD_HEIGHT - 20; y < WORLD_HEIGHT; y++) {
        for (int x = 75; x < 80; x++) {
            automaton.setCell(x, y, automaton.getMaterialIDByName("Stone"));
        }
    }
    
    // Create a water source on the left side
    for (int x = 25; x < 35; x++) {
        for (int y = WORLD_HEIGHT - 18; y < WORLD_HEIGHT - 13; y++) {
            automaton.setCell(x, y, automaton.getMaterialIDByName("Water"));
        }
    }
}

// Create test 3: Water and other materials interaction
void setupMaterialInteractionTest(astral::CellularAutomaton& automaton) {
    // Clear the world
    automaton.clearWorld();
    
    // Create solid bottom
    for (int x = 0; x < WORLD_WIDTH; x++) {
        automaton.setCell(x, WORLD_HEIGHT - 1, automaton.getMaterialIDByName("Stone"));
    }
    
    // Create walls
    for (int y = 0; y < WORLD_HEIGHT; y++) {
        automaton.setCell(0, y, automaton.getMaterialIDByName("Stone"));
        automaton.setCell(WORLD_WIDTH - 1, y, automaton.getMaterialIDByName("Stone"));
    }
    
    // Create some compartments with different materials
    
    // Sand pile
    for (int x = 10; x < 20; x++) {
        for (int y = WORLD_HEIGHT - 10; y < WORLD_HEIGHT - 5; y++) {
            automaton.setCell(x, y, automaton.getMaterialIDByName("Sand"));
        }
    }
    
    // Create a more distinct test for water-oil separation
    // For test 3, make a more extreme case to verify oil isn't disappearing
    // Add water column on the left
    for (int x = 40; x < 45; x++) {
        for (int y = WORLD_HEIGHT - 20; y < WORLD_HEIGHT - 5; y++) {
            automaton.setCell(x, y, automaton.getMaterialIDByName("Water"));
        }
    }
    
    // Add oil column on the right
    for (int x = 55; x < 60; x++) {
        for (int y = WORLD_HEIGHT - 20; y < WORLD_HEIGHT - 5; y++) {
            automaton.setCell(x, y, automaton.getMaterialIDByName("Oil"));
        }
    }
    
    // Wooden platform that water can flow around
    for (int x = 70; x < 90; x++) {
        automaton.setCell(x, WORLD_HEIGHT - 10, automaton.getMaterialIDByName("Wood"));
    }
    
    // Water above wooden platform
    for (int x = 75; x < 85; x++) {
        for (int y = WORLD_HEIGHT - 15; y < WORLD_HEIGHT - 11; y++) {
            automaton.setCell(x, y, automaton.getMaterialIDByName("Water"));
        }
    }
}

int main(int argc, char* argv[]) {
    std::cout << "Water Physics Simulation Test" << std::endl;
    
    // Flag for continuous simulation (will run until interrupted)
    bool runContinuously = false;
    
    // Check command line arguments
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--continuous" || arg == "-c") {
            runContinuously = true;
            std::cout << "Running in continuous mode. Press Ctrl+C to stop." << std::endl;
        }
    }
    
    // Initialize the automaton
    astral::CellularAutomaton automaton(WORLD_WIDTH, WORLD_HEIGHT);
    automaton.initialize();
    
    // Test types
    const int NUM_TESTS = 3;
    
    for (int currentTest = 1; currentTest <= NUM_TESTS; currentTest++) {
        std::cout << "\n=== Test " << currentTest << " ===\n" << std::endl;
        
        // Setup the appropriate test
        switch (currentTest) {
            case 1:
                std::cout << "Water Pooling and Flowing Test" << std::endl;
                setupPoolingTest(automaton);
                break;
            case 2:
                std::cout << "Water Leveling Test (U-shaped Container)" << std::endl;
                setupLevelingTest(automaton);
                break;
            case 3:
                std::cout << "Water and Materials Interaction Test" << std::endl;
                setupMaterialInteractionTest(automaton);
                break;
        }
        
        // Display initial state
        std::cout << "Initial state:" << std::endl;
        displayWorld(automaton);
        
        // Run simulation steps
        astral::Timer timer;
        float deltaTime = 0.05f; // Fixed time step
        
        // Extended test duration or continuous mode
        int maxSteps = runContinuously ? std::numeric_limits<int>::max() : 150;
        const int displayInterval = 10; // Display less frequently (was 3)
        
        // Initialize material counters outside the loop for continuous mode
        int initialWaterCount = 0;
        int initialOilCount = 0;
        int initialSandCount = 0;
        int initialWoodCount = 0;
        int initialStoneCount = 0;
        bool isFirstStep = true;
        
        for (int step = 1; step <= maxSteps; step++) {
            // Update simulation every step
            timer.update();
            automaton.update(deltaTime);
            
            // Only display at intervals to avoid overwhelming output
            if (step % displayInterval == 0 || step == 1) {
                std::cout << "\nStep " << step << ":" << std::endl;
                displayWorld(automaton);
                
                // Count materials for debugging
                int waterCount = 0;
                int oilCount = 0;
                int sandCount = 0;
                int woodCount = 0;
                int stoneCount = 0;
                
                // Count various materials in the world
                for (int y = 0; y < WORLD_HEIGHT; y++) {
                    for (int x = 0; x < WORLD_WIDTH; x++) {
                        const astral::Cell& cell = automaton.getCell(x, y);
                        if (cell.material == automaton.getMaterialIDByName("Water")) {
                            waterCount++;
                        } else if (cell.material == automaton.getMaterialIDByName("Oil")) {
                            oilCount++;
                        } else if (cell.material == automaton.getMaterialIDByName("Sand")) {
                            sandCount++;
                        } else if (cell.material == automaton.getMaterialIDByName("Wood")) {
                            woodCount++;
                        } else if (cell.material == automaton.getMaterialIDByName("Stone")) {
                            stoneCount++;
                        }
                    }
                }
                
                // Store initial counts for comparing later
                if (isFirstStep) {
                    initialWaterCount = waterCount;
                    initialOilCount = oilCount;
                    initialSandCount = sandCount;
                    initialWoodCount = woodCount;
                    initialStoneCount = stoneCount;
                    isFirstStep = false;
                }
                
                // Display stats
                auto stats = automaton.getSimulationStats();
                std::cout << "Active chunks: " << stats.activeChunks
                          << " | Active cells: " << stats.activeCells
                          << " | Update time: " << (timer.getDeltaTime() * 1000.0) << "ms" << std::endl;
                std::cout << "Water cells: " << waterCount << " (initial: " << initialWaterCount << ")"
                          << " | Oil cells: " << oilCount << " (initial: " << initialOilCount << ")"
                          << " | Sand cells: " << sandCount << " (initial: " << initialSandCount << ")"
                          << " | Wood cells: " << woodCount << " (initial: " << initialWoodCount << ")"
                          << " | Stone cells: " << stoneCount << " (initial: " << initialStoneCount << ")" << std::endl;
                
                // Check for material conservation and raise warning if materials are disappearing
                if (waterCount < initialWaterCount) {
                    std::cout << "WARNING: Water is disappearing! Lost " << (initialWaterCount - waterCount) << " cells" << std::endl;
                }
                if (oilCount < initialOilCount) {
                    std::cout << "WARNING: Oil is disappearing! Lost " << (initialOilCount - oilCount) << " cells" << std::endl;
                }
                if (sandCount < initialSandCount) {
                    std::cout << "WARNING: Sand is disappearing! Lost " << (initialSandCount - sandCount) << " cells" << std::endl;
                }
                          
                // Add a small delay to allow viewing the progress
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                
                // For continuous mode, reset counter after 1000 steps to prevent integer overflow
                if (runContinuously && step >= 1000) {
                    std::cout << "\nResetting step counter..." << std::endl;
                    step = 0;
                }
            }
        }
        
        // In continuous mode, only run the last test (test 3 with water and oil interactions)
        if (runContinuously && currentTest == 3) {
            break;
        }
    }
    
    std::cout << "\nWater simulation test complete!" << std::endl;
    return 0;
}