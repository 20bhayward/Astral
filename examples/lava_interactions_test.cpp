#include <iostream>
#include <chrono>
#include <thread>
#include <string>
#include <iomanip>
#include <cmath>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "astral/physics/CellularAutomaton.h"
#include "astral/physics/Material.h"
#include "astral/core/Timer.h"

// Test for lava interactions with other materials
// This version uses OpenGL for rendering to scale the simulation 100x

// Window dimensions
int SCREEN_WIDTH = 1280;
int SCREEN_HEIGHT = 720;

// World dimensions - much larger than the ASCII version (100x)
const int WORLD_WIDTH = 1000;
const int WORLD_HEIGHT = 500;

// Camera/view settings
float zoomLevel = 0.2f;  // Start zoomed out to see most of the world
int viewportX = 0;
int viewportY = 0;

// Mouse state
bool leftMouseDown = false;
bool rightMouseDown = false;
bool middleMouseDown = false;
int mouseX = 0;
int mouseY = 0;
int lastMouseX = 0;
int lastMouseY = 0;
int brushSize = 5;

// Current selected material
astral::MaterialID currentMaterial;

// Simulation control
bool simulationPaused = false;
float simulationSpeed = 1.0f;

// Function prototypes
bool initOpenGL();
void drawWorld(const astral::CellularAutomaton& automaton);
void processInput(GLFWwindow* window, astral::CellularAutomaton& automaton);
void setupLavaLakeTest(astral::CellularAutomaton& automaton);
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
void cursor_position_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);

// Global GLFW window pointer for callbacks
GLFWwindow* gWindow = nullptr;

// Function to display the world in the console with colors (via terminal escape codes)
void displayWorld(const astral::CellularAutomaton& automaton) {
    std::cout << "World state:" << std::endl;
    std::cout << std::string(WORLD_WIDTH + 2, '-') << std::endl;
    
    for (int y = 0; y < WORLD_HEIGHT; y++) {
        std::cout << "|";
        for (int x = 0; x < WORLD_WIDTH; x++) {
            const astral::Cell& cell = automaton.getCell(x, y);
            
            // Different characters for different materials
            if (cell.material == automaton.getMaterialIDByName("Air")) {
                std::cout << " ";
            } else if (cell.material == automaton.getMaterialIDByName("Sand")) {
                std::cout << "s";
            } else if (cell.material == automaton.getMaterialIDByName("Water")) {
                std::cout << "~";
            } else if (cell.material == automaton.getMaterialIDByName("Stone")) {
                std::cout << "#";
            } else if (cell.material == automaton.getMaterialIDByName("Wood")) {
                std::cout << "W";
            } else if (cell.material == automaton.getMaterialIDByName("Oil")) {
                std::cout << "o";
            } else if (cell.material == automaton.getMaterialIDByName("Lava")) {
                std::cout << "*";
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

void createLavaLake(astral::CellularAutomaton& automaton) {
    // Create a lava lake in the left section of the world
    for (int x = 2; x < 15; x++) {
        for (int y = WORLD_HEIGHT - 4; y < WORLD_HEIGHT - 1; y++) {
            automaton.setCell(x, y, automaton.getMaterialIDByName("Lava"));
        }
    }
}

void createWaterLake(astral::CellularAutomaton& automaton) {
    // Create a water lake in the right section of the world
    for (int x = 25; x < 38; x++) {
        for (int y = WORLD_HEIGHT - 4; y < WORLD_HEIGHT - 1; y++) {
            automaton.setCell(x, y, automaton.getMaterialIDByName("Water"));
        }
    }
}

void createWoodTower(astral::CellularAutomaton& automaton) {
    // Create a wood tower above the lava lake
    for (int y = WORLD_HEIGHT - 10; y < WORLD_HEIGHT - 4; y++) {
        automaton.setCell(7, y, automaton.getMaterialIDByName("Wood"));
        automaton.setCell(8, y, automaton.getMaterialIDByName("Wood"));
    }
}

void createOilPool(astral::CellularAutomaton& automaton) {
    // Create a small oil pool near the center
    for (int x = 17; x < 23; x++) {
        for (int y = WORLD_HEIGHT - 3; y < WORLD_HEIGHT - 1; y++) {
            automaton.setCell(x, y, automaton.getMaterialIDByName("Oil"));
        }
    }
}

void createStonePlatform(astral::CellularAutomaton& automaton) {
    // Create stone platforms and barriers
    // Center divider
    for (int y = WORLD_HEIGHT - 5; y < WORLD_HEIGHT - 1; y++) {
        automaton.setCell(20, y, automaton.getMaterialIDByName("Stone"));
    }
    
    // Upper platform near lava
    for (int x = 3; x < 13; x++) {
        automaton.setCell(x, WORLD_HEIGHT - 7, automaton.getMaterialIDByName("Stone"));
    }
}

int main() {
    std::cout << "Testing Lava Interactions Physics\n" << std::endl;
    
    // Display legend
    std::cout << "Legend:" << std::endl;
    std::cout << "  ' ' - Air" << std::endl;
    std::cout << "  '#' - Stone" << std::endl;
    std::cout << "  '*' - Lava" << std::endl;
    std::cout << "  '~' - Water" << std::endl;
    std::cout << "  'W' - Wood" << std::endl;
    std::cout << "  'o' - Oil" << std::endl;
    std::cout << "  'F' - Fire" << std::endl;
    std::cout << "  '^' - Steam" << std::endl;
    std::cout << "  '%' - Smoke" << std::endl;
    std::cout << "  's' - Sand" << std::endl;
    std::cout << "\n";
    
    // Initialize the automaton with a larger world
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
    
    // Create test scene
    createLavaLake(automaton);
    createWaterLake(automaton);
    createWoodTower(automaton);
    createOilPool(automaton);
    createStonePlatform(automaton);
    
    // Add some sand to test with lava
    for (int x = 5; x < 8; x++) {
        automaton.setCell(x, WORLD_HEIGHT - 10, automaton.getMaterialIDByName("Sand"));
    }
    
    // Display initial state
    std::cout << "Initial state:" << std::endl;
    displayWorld(automaton);
    
    // Run simulation steps
    astral::Timer timer;
    float deltaTime = 0.1f; // Fixed time step for predictable behavior
    
    const int totalSteps = 50;
    
    for (int step = 0; step < totalSteps; step++) {
        // Update the simulation
        timer.reset();
        automaton.update(deltaTime);
        
        // Display the world every few steps to avoid overwhelming the console
        if (step % 5 == 0 || step == totalSteps - 1) {
            std::cout << "Step " << step + 1 << ":" << std::endl;
            displayWorld(automaton);
            
            // Display progress indicator
            std::cout << "Progress: [";
            int progressBarWidth = 30;
            int pos = progressBarWidth * (step + 1) / totalSteps;
            for (int i = 0; i < progressBarWidth; ++i) {
                if (i < pos) std::cout << "=";
                else if (i == pos) std::cout << ">";
                else std::cout << " ";
            }
            std::cout << "] " << int((step + 1) * 100.0 / totalSteps) << "%\n" << std::endl;
        }
        
        // Pause for visibility
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    
    std::cout << "Lava interactions test complete!" << std::endl;
    return 0;
}