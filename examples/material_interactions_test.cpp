#include <iostream>
#include <chrono>
#include <thread>
#include <string>
#include <vector>
#include <cmath>

#include "astral/physics/CellularAutomaton.h"
#include "astral/physics/Material.h"
#include "astral/core/Timer.h"

// Create a smaller world to better visualize interactions
const int WORLD_WIDTH = 80;
const int WORLD_HEIGHT = 40;

// Function to display the world in the console
void displayWorld(const astral::CellularAutomaton& automaton) {
    std::cout << "+" << std::string(WORLD_WIDTH, '-') << "+" << std::endl;
    
    // Print material legend
    std::cout << "| LEGEND: # = Stone, s = Sand, ~ = Water, o = Oil, L = Lava, * = Fire, + = Wood |" << std::endl;
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
                std::cout << "*";
            } else if (cell.material == automaton.getMaterialIDByName("Lava")) {
                std::cout << "L";
            } else {
                std::cout << "?";
            }
        }
        std::cout << "|" << std::endl;
    }
    
    std::cout << "+" << std::string(WORLD_WIDTH, '-') << "+" << std::endl;
}

// Create interesting test scenarios
void setupInteractionTest1(astral::CellularAutomaton& automaton) {
    // Clear the world
    automaton.clearWorld();
    
    // Bottom wall
    for (int x = 0; x < WORLD_WIDTH; x++) {
        automaton.setCell(x, WORLD_HEIGHT - 1, automaton.getMaterialIDByName("Stone"));
    }
    
    // Side walls
    for (int y = 0; y < WORLD_HEIGHT; y++) {
        automaton.setCell(0, y, automaton.getMaterialIDByName("Stone"));
        automaton.setCell(WORLD_WIDTH - 1, y, automaton.getMaterialIDByName("Stone"));
    }
    
    // Create a set of "stair" platforms for sand to cascade down
    for (int i = 0; i < 5; i++) {
        int platformWidth = 10;
        int platformX = 10 + i * 10;
        int platformY = WORLD_HEIGHT - 5 - i * 5;
        
        for (int x = 0; x < platformWidth; x++) {
            automaton.setCell(platformX + x, platformY, automaton.getMaterialIDByName("Stone"));
        }
    }
    
    // Add water pool at the bottom
    for (int x = 15; x < 45; x++) {
        for (int y = WORLD_HEIGHT - 4; y < WORLD_HEIGHT - 1; y++) {
            automaton.setCell(x, y, automaton.getMaterialIDByName("Water"));
        }
    }
    
    // Add oil pool at right side
    for (int x = 55; x < 70; x++) {
        for (int y = WORLD_HEIGHT - 4; y < WORLD_HEIGHT - 1; y++) {
            automaton.setCell(x, y, automaton.getMaterialIDByName("Oil"));
        }
    }
    
    // Add sand pile at the top to cascade down
    for (int x = 10; x < 30; x++) {
        for (int y = 2; y < 8; y++) {
            automaton.setCell(x, y, automaton.getMaterialIDByName("Sand"));
        }
    }
}

void setupInteractionTest2(astral::CellularAutomaton& automaton) {
    // Clear the world
    automaton.clearWorld();
    
    // Bottom wall
    for (int x = 0; x < WORLD_WIDTH; x++) {
        automaton.setCell(x, WORLD_HEIGHT - 1, automaton.getMaterialIDByName("Stone"));
    }
    
    // Side walls
    for (int y = 0; y < WORLD_HEIGHT; y++) {
        automaton.setCell(0, y, automaton.getMaterialIDByName("Stone"));
        automaton.setCell(WORLD_WIDTH - 1, y, automaton.getMaterialIDByName("Stone"));
    }
    
    // Create a series of platforms to form a funnel
    int midX = WORLD_WIDTH / 2;
    int platformLength = 25;
    
    // Left side of funnel
    for (int i = 0; i < platformLength; i++) {
        automaton.setCell(midX - 10 - i, 15 + i/2, automaton.getMaterialIDByName("Stone"));
    }
    
    // Right side of funnel
    for (int i = 0; i < platformLength; i++) {
        automaton.setCell(midX + 10 + i, 15 + i/2, automaton.getMaterialIDByName("Stone"));
    }
    
    // Bottom platform with a gap
    for (int x = midX - 20; x <= midX + 20; x++) {
        if (x < midX - 5 || x > midX + 5) {
            automaton.setCell(x, 30, automaton.getMaterialIDByName("Stone"));
        }
    }
    
    // Add a column of water
    for (int y = 5; y < 15; y++) {
        automaton.setCell(midX - 15, y, automaton.getMaterialIDByName("Water"));
        automaton.setCell(midX - 14, y, automaton.getMaterialIDByName("Water"));
    }
    
    // Add a column of oil
    for (int y = 5; y < 15; y++) {
        automaton.setCell(midX + 15, y, automaton.getMaterialIDByName("Oil"));
        automaton.setCell(midX + 14, y, automaton.getMaterialIDByName("Oil"));
    }
    
    // Add sand above the funnel
    for (int x = midX - 20; x <= midX + 20; x++) {
        for (int y = 2; y < 8; y++) {
            automaton.setCell(x, y, automaton.getMaterialIDByName("Sand"));
        }
    }
    
    // Create pools at the bottom
    for (int x = 5; x < midX - 5; x++) {
        for (int y = WORLD_HEIGHT - 5; y < WORLD_HEIGHT - 1; y++) {
            automaton.setCell(x, y, automaton.getMaterialIDByName("Water"));
        }
    }
    
    for (int x = midX + 5; x < WORLD_WIDTH - 5; x++) {
        for (int y = WORLD_HEIGHT - 5; y < WORLD_HEIGHT - 1; y++) {
            automaton.setCell(x, y, automaton.getMaterialIDByName("Oil"));
        }
    }
}

int main() {
    std::cout << "Material Interactions Test" << std::endl;
    
    // Initialize the automaton with our world dimensions
    astral::CellularAutomaton automaton(WORLD_WIDTH, WORLD_HEIGHT);
    automaton.initialize();
    
    // Setup test scenario 1
    std::cout << "\n=== Test Scenario 1: Cascade ===\n" << std::endl;
    setupInteractionTest1(automaton);
    
    // Display initial state
    std::cout << "Initial state:" << std::endl;
    displayWorld(automaton);
    
    // Run simulation steps
    astral::Timer timer;
    float deltaTime = 0.1f; // Fixed time step
    
    for (int step = 1; step <= 25; step++) {
        // Update simulation
        timer.reset();
        automaton.update(deltaTime);
        
        // Show progress every few steps
        if (step % 5 == 0 || step == 1) {
            std::cout << "\nStep " << step << ":" << std::endl;
            displayWorld(automaton);
            
            // Show stats
            const auto& stats = automaton.getSimulationStats();
            std::cout << "Active chunks: " << stats.activeChunks 
                      << " | Active cells: " << stats.activeCells 
                      << " | Update time: " << stats.updateTimeMs << "ms" << std::endl;
        }
        
        // Small delay for viewing
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    
    // Setup test scenario 2
    std::cout << "\n=== Test Scenario 2: Funnel ===\n" << std::endl;
    setupInteractionTest2(automaton);
    
    // Display initial state
    std::cout << "Initial state:" << std::endl;
    displayWorld(automaton);
    
    // Run simulation steps
    for (int step = 1; step <= 30; step++) {
        // Update simulation
        timer.reset();
        automaton.update(deltaTime);
        
        // Show progress every few steps
        if (step % 5 == 0 || step == 1) {
            std::cout << "\nStep " << step << ":" << std::endl;
            displayWorld(automaton);
            
            // Show stats
            const auto& stats = automaton.getSimulationStats();
            std::cout << "Active chunks: " << stats.activeChunks 
                      << " | Active cells: " << stats.activeCells 
                      << " | Update time: " << stats.updateTimeMs << "ms" << std::endl;
        }
        
        // Small delay for viewing
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    
    std::cout << "\nMaterial interactions test complete!" << std::endl;
    return 0;
}