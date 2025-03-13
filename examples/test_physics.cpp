#include <iostream>
#include <chrono>
#include <thread>
#include <string>

#include "astral/physics/CellularAutomaton.h"
#include "astral/physics/Material.h"
#include "astral/core/Timer.h"

// Very simple test for the cellular automaton physics
// This example doesn't use any graphical libraries and just outputs text to console

const int WORLD_WIDTH = 15;
const int WORLD_HEIGHT = 10;

// Function to display the world in the console
void displayWorld(const astral::CellularAutomaton& automaton) {
    std::cout << "World state:" << std::endl;
    std::cout << std::string(WORLD_WIDTH + 2, '-') << std::endl;
    
    for (int y = 0; y < WORLD_HEIGHT; y++) {
        std::cout << "|";
        for (int x = 0; x < WORLD_WIDTH; x++) {
            const astral::Cell& cell = automaton.getCell(x, y);
            
            if (cell.material == automaton.getMaterialIDByName("Air")) {
                std::cout << " ";
            } else if (cell.material == automaton.getMaterialIDByName("Sand")) {
                std::cout << "s";
            } else if (cell.material == automaton.getMaterialIDByName("Water")) {
                std::cout << "~";
            } else if (cell.material == automaton.getMaterialIDByName("Stone")) {
                std::cout << "#";
            } else if (cell.material == automaton.getMaterialIDByName("Lava")) {
                std::cout << "*";
            } else if (cell.material == automaton.getMaterialIDByName("Wood")) {
                std::cout << "W";
            } else if (cell.material == automaton.getMaterialIDByName("Oil")) {
                std::cout << "o";
            } else if (cell.material == automaton.getMaterialIDByName("Fire")) {
                std::cout << "F";
            } else if (cell.material == automaton.getMaterialIDByName("Steam")) {
                std::cout << "^";
            } else if (cell.material == automaton.getMaterialIDByName("Smoke")) {
                std::cout << "%";
            } else {
                std::cout << "?";
            }
        }
        std::cout << "|" << std::endl;
    }
    
    std::cout << std::string(WORLD_WIDTH + 2, '-') << std::endl;
}

int main() {
    std::cout << "Testing Cellular Automaton Physics" << std::endl;
    
    // Initialize the automaton with a small world
    astral::CellularAutomaton automaton(WORLD_WIDTH, WORLD_HEIGHT);
    automaton.initialize();
    
    // Clear world to start fresh
    automaton.clearWorld();
    
    // Create walls around the world
    for (int x = 0; x < WORLD_WIDTH; x++) {
        automaton.setCell(x, WORLD_HEIGHT - 1, automaton.getMaterialIDByName("Stone"));
    }
    
    for (int y = 0; y < WORLD_HEIGHT; y++) {
        automaton.setCell(0, y, automaton.getMaterialIDByName("Stone"));
        automaton.setCell(WORLD_WIDTH - 1, y, automaton.getMaterialIDByName("Stone"));
    }
    
    // Add a pile of sand higher up so it has room to fall
    automaton.setCell(WORLD_WIDTH / 2, 1, automaton.getMaterialIDByName("Sand"));
    automaton.setCell(WORLD_WIDTH / 2 - 1, 1, automaton.getMaterialIDByName("Sand"));
    automaton.setCell(WORLD_WIDTH / 2 + 1, 1, automaton.getMaterialIDByName("Sand"));
    
    // Add some water on the side
    automaton.setCell(3, 1, automaton.getMaterialIDByName("Water"));
    automaton.setCell(4, 1, automaton.getMaterialIDByName("Water"));
    
    // Add lava at the bottom
    automaton.setCell(2, WORLD_HEIGHT - 2, automaton.getMaterialIDByName("Lava"));
    automaton.setCell(3, WORLD_HEIGHT - 2, automaton.getMaterialIDByName("Lava"));
    
    // Add wood near lava
    automaton.setCell(4, WORLD_HEIGHT - 3, automaton.getMaterialIDByName("Wood"));
    
    // Add oil
    automaton.setCell(6, WORLD_HEIGHT - 2, automaton.getMaterialIDByName("Oil"));
    
    // Display initial state
    std::cout << "Initial state:" << std::endl;
    displayWorld(automaton);
    
    // Run simulation steps
    astral::Timer timer;
    float deltaTime = 0.1f; // Fixed time step for predictable behavior
    
    for (int step = 0; step < 20; step++) {
        // Update the simulation
        timer.reset();
        automaton.update(deltaTime);
        
        // Display the world
        std::cout << "Step " << step + 1 << ":" << std::endl;
        displayWorld(automaton);
        
        // Pause for visibility
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
    
    std::cout << "Physics test complete!" << std::endl;
    return 0;
}