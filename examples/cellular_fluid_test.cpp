#include <iostream>
#include <chrono>
#include <thread>
#include <string>
#include <vector>
#include <cmath>
#include <limits>

// OpenGL headers - conditionally included
// System-wide definition takes precedence over local one
#if defined(USE_OPENGL) || defined(_USE_OPENGL)
#define ENABLE_OPENGL
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#endif

#include "astral/physics/CellularAutomaton.h"
#include "astral/physics/Material.h"
#include "astral/core/Timer.h"

#ifdef ENABLE_OPENGL
// Window dimensions for OpenGL rendering
int SCREEN_WIDTH = 1280;
int SCREEN_HEIGHT = 720;

// Camera/view settings - start with a properly centered zoomed view
float zoomLevel = 2.0f;  // More zoomed in to see cells better
float viewportX = 70.0f;  // Center on the main action area with adjusted width
float viewportY = 100.0f;

// Mouse state for interaction
bool leftMouseDown = false;
bool rightMouseDown = false;
bool middleMouseDown = false;
int mouseX = 0;
int mouseY = 0;
int lastMouseX = 0;
int lastMouseY = 0;
int brushSize = 5;

// Current selected material for drawing
astral::MaterialID currentMaterial;

// Simulation control
bool simulationPaused = false;
float simulationSpeed = 1.0f;

// Function prototypes for OpenGL rendering
bool initOpenGL();
void drawWorld(const astral::CellularAutomaton& automaton);
void processInput(GLFWwindow* window, astral::CellularAutomaton& automaton);
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
void cursor_position_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);

// Global GLFW window pointer for callbacks
GLFWwindow* gWindow = nullptr;
#endif

// Create a test world with appropriate dimensions
// Use much larger world for OpenGL visualization, smaller for ASCII
#ifdef USE_OPENGL
const int WORLD_WIDTH = 200;  // Made narrower to prevent memory issues
const int WORLD_HEIGHT = 200; // Smaller height for stability
#else
const int WORLD_WIDTH = 100;
const int WORLD_HEIGHT = 50;
#endif

// Flag for graphics mode
bool runningGraphicsMode = false;

// Structure to track a point for visualization
struct Point {
    int x, y;
    astral::MaterialID material;
};

// Display the world with a legend for materials
void displayWorld(const astral::CellularAutomaton& automaton) {
    std::cout << "+" << std::string(WORLD_WIDTH, '-') << "+" << std::endl;
    
    // Print material legend
    std::cout << "| LEGEND: # = Stone, ~ = Water, o = Oil, * = Steam, @ = Smoke, + = Wood, F = Fire, O = Oil Fire, L = Lava |" << std::endl;
    std::cout << "+" << std::string(WORLD_WIDTH, '-') << "+" << std::endl;
    
    for (int y = 0; y < WORLD_HEIGHT; y++) {
        std::cout << "|";
        for (int x = 0; x < WORLD_WIDTH; x++) {
            const astral::Cell& cell = automaton.getCell(x, y);
            
            // Display material with different characters
            if (cell.material == automaton.getMaterialIDByName("Air")) {
                std::cout << " ";
            } else if (cell.material == automaton.getMaterialIDByName("Water")) {
                std::cout << "~";
            } else if (cell.material == automaton.getMaterialIDByName("Oil")) {
                std::cout << "o";
            } else if (cell.material == automaton.getMaterialIDByName("Stone")) {
                std::cout << "#";
            } else if (cell.material == automaton.getMaterialIDByName("Wood")) {
                std::cout << "+";
            } else if (cell.material == automaton.getMaterialIDByName("Steam")) {
                std::cout << "*";
            } else if (cell.material == automaton.getMaterialIDByName("Smoke")) {
                std::cout << "@";
            } else if (cell.material == automaton.getMaterialIDByName("Fire")) {
                // Check if this is an oil fire by examining the cell's velocity marker
                if (cell.velocity.x > 0.5f) {
                    std::cout << "O"; // Oil fire
                } else {
                    std::cout << "F"; // Regular fire
                }
            } else if (cell.material == automaton.getMaterialIDByName("Lava")) {
                std::cout << "L"; // Lava
            } else {
                std::cout << "?";
            }
        }
        std::cout << "|" << std::endl;
    }
    
    std::cout << "+" << std::string(WORLD_WIDTH, '-') << "+" << std::endl;
}

// Create test 1: Water flow test with obstacles
void setupWaterFlowTest(astral::CellularAutomaton& automaton) {
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
    
    // Create a tiered landscape for water to flow over
    for (int x = 10; x < 90; x += 20) {
        int height = 10 + (x / 10) % 5;
        for (int y = WORLD_HEIGHT - height; y < WORLD_HEIGHT; y++) {
            for (int i = 0; i < 10; i++) {
                if (x + i < WORLD_WIDTH - 1) {
                    automaton.setCell(x + i, y, automaton.getMaterialIDByName("Stone"));
                }
            }
        }
    }
    
    // Create a water source at the top left
    for (int x = 2; x < 8; x++) {
        for (int y = 2; y < 7; y++) {
            automaton.setCell(x, y, automaton.getMaterialIDByName("Water"));
        }
    }
}

// Create test 2: Water and oil separation test
void setupWaterOilSeparationTest(astral::CellularAutomaton& automaton) {
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
    
    // Create a container with walls
    for (int y = WORLD_HEIGHT - 20; y < WORLD_HEIGHT; y++) {
        automaton.setCell(30, y, automaton.getMaterialIDByName("Stone"));
        automaton.setCell(70, y, automaton.getMaterialIDByName("Stone"));
    }
    
    // Add mixed water and oil in the middle
    for (int x = 35; x < 65; x++) {
        for (int y = WORLD_HEIGHT - 18; y < WORLD_HEIGHT - 3; y++) {
            // Alternate between water and oil for mixing
            if ((x + y) % 2 == 0) {
                automaton.setCell(x, y, automaton.getMaterialIDByName("Water"));
            } else {
                automaton.setCell(x, y, automaton.getMaterialIDByName("Oil"));
            }
        }
    }
}

// Create test 3: Complex gas physics and interactions test
void setupGasPhysicsTest(astral::CellularAutomaton& automaton) {
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
    
    // Create a multi-chamber setup with different materials
    
    // First chamber - Heat chamber with steam rising
    // Left wall
    for (int y = WORLD_HEIGHT - 20; y < WORLD_HEIGHT; y++) {
        automaton.setCell(25, y, automaton.getMaterialIDByName("Stone"));
    }
    
    // Steam source inside first chamber
    for (int x = 5; x < 15; x++) {
        for (int y = WORLD_HEIGHT - 5; y < WORLD_HEIGHT - 2; y++) {
            automaton.setCell(x, y, automaton.getMaterialIDByName("Steam"));
        }
    }
    
    // Hot water pool that will evaporate over time
    for (int x = 15; x < 24; x++) {
        for (int y = WORLD_HEIGHT - 10; y < WORLD_HEIGHT - 3; y++) {
            // Set cell to water
            automaton.setCell(x, y, automaton.getMaterialIDByName("Water"));
            
            // Get the cell and set its temperature manually
            astral::Cell& cell = automaton.getCell(x, y);
            cell.temperature = 88.0f; // Just under boiling point to trigger evaporation
        }
    }
    
    // Second chamber - Smoke and water interaction
    // Right wall of second chamber
    for (int y = WORLD_HEIGHT - 20; y < WORLD_HEIGHT; y++) {
        automaton.setCell(50, y, automaton.getMaterialIDByName("Stone"));
    }
    
    // Smoke source at the bottom of second chamber
    for (int x = 30; x < 35; x++) {
        for (int y = WORLD_HEIGHT - 5; y < WORLD_HEIGHT - 2; y++) {
            automaton.setCell(x, y, automaton.getMaterialIDByName("Smoke"));
        }
    }
    
    // Water barrier in second chamber - see how smoke passes through water
    for (int x = 35; x < 45; x++) {
        for (int y = WORLD_HEIGHT - 15; y < WORLD_HEIGHT - 10; y++) {
            automaton.setCell(x, y, automaton.getMaterialIDByName("Water"));
        }
    }
    
    // Third chamber - Oil and gas layers
    // Right wall of third chamber
    for (int y = WORLD_HEIGHT - 20; y < WORLD_HEIGHT; y++) {
        automaton.setCell(75, y, automaton.getMaterialIDByName("Stone"));
    }
    
    // Layer of oil in the third chamber
    for (int x = 55; x < 70; x++) {
        for (int y = WORLD_HEIGHT - 12; y < WORLD_HEIGHT - 7; y++) {
            automaton.setCell(x, y, automaton.getMaterialIDByName("Oil"));
        }
    }
    
    // Steam source below the oil in third chamber
    for (int x = 60; x < 65; x++) {
        for (int y = WORLD_HEIGHT - 5; y < WORLD_HEIGHT - 3; y++) {
            automaton.setCell(x, y, automaton.getMaterialIDByName("Steam"));
        }
    }
    
    // Fourth chamber - Mixture of gases
    // Mixed steam and smoke sources
    for (int x = 80; x < 95; x += 3) {
        for (int y = WORLD_HEIGHT - 5; y < WORLD_HEIGHT - 2; y++) {
            if ((x / 3) % 2 == 0) {
                automaton.setCell(x, y, automaton.getMaterialIDByName("Steam"));
            } else {
                automaton.setCell(x, y, automaton.getMaterialIDByName("Smoke"));
            }
        }
    }
    
    // Add horizontal barriers throughout the system to demonstrate gas flow around obstacles
    
    // Top platform spanning all chambers
    for (int x = 5; x < 95; x++) {
        if (x != 35 && x != 65) { // Add gaps for gases to flow through
            automaton.setCell(x, 8, automaton.getMaterialIDByName("Wood"));
        }
    }
    
    // Add stone ceiling in the first and second chambers to see gas pooling behavior
    // First chamber ceiling
    for (int x = 1; x < 25; x++) {
        automaton.setCell(x, 4, automaton.getMaterialIDByName("Stone"));
    }
    
    // Second chamber ceiling - with small gap on right side
    for (int x = 26; x < 48; x++) {
        automaton.setCell(x, 4, automaton.getMaterialIDByName("Stone"));
    }
    
    // Middle platform in the fourth chamber - create turbulence
    for (int x = 80; x < 95; x++) {
        if (x != 85 && x != 90) { // Add gaps
            automaton.setCell(x, 20, automaton.getMaterialIDByName("Wood"));
        }
    }
    
    // Add small platforms in first chamber to demonstrate steam rising around obstacles
    for (int x = 5; x < 20; x++) {
        if (x < 10 || x > 15) { // Create a gap in the middle
            automaton.setCell(x, 25, automaton.getMaterialIDByName("Wood"));
        }
    }
    
    // Create a "chimney" in the second chamber
    for (int y = WORLD_HEIGHT - 20; y < WORLD_HEIGHT - 10; y++) {
        automaton.setCell(35, y, automaton.getMaterialIDByName("Stone"));
        automaton.setCell(45, y, automaton.getMaterialIDByName("Stone"));
    }
}

// Create test 4: Fire interactions test
void setupFireTest(astral::CellularAutomaton& automaton) {
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
    
    // Create different test scenarios for fire
    
    // 1. Wood burning test - simple stack of wood with fire at the bottom
    for (int y = WORLD_HEIGHT - 15; y < WORLD_HEIGHT - 5; y++) {
        for (int x = 10; x < 20; x++) {
            automaton.setCell(x, y, automaton.getMaterialIDByName("Wood"));
        }
    }
    // Add fire at the base of the wood
    for (int x = 12; x < 18; x++) {
        automaton.setCell(x, WORLD_HEIGHT - 5, automaton.getMaterialIDByName("Fire"));
    }
    
    // 2. Fire, water, and steam interactions
    // Two-chamber setup: one for water-fire, one for steam-fire interactions
    
    // Left chamber: Water dripping on fire
    // Create a platform for the water
    for (int x = 30; x < 40; x++) {
        automaton.setCell(x, WORLD_HEIGHT - 10, automaton.getMaterialIDByName("Stone"));
    }
    // Add water reservoir with a leak
    for (int x = 32; x < 38; x++) {
        for (int y = WORLD_HEIGHT - 15; y < WORLD_HEIGHT - 10; y++) {
            automaton.setCell(x, y, automaton.getMaterialIDByName("Water"));
        }
    }
    // Create a hole in the reservoir
    automaton.setCell(35, WORLD_HEIGHT - 10, 0); // Air instead of stone
    
    // Add fire below where water will drip
    for (int x = 33; x < 38; x++) {
        automaton.setCell(x, WORLD_HEIGHT - 5, automaton.getMaterialIDByName("Fire"));
    }
    
    // Right chamber: Steam interacting with fire
    // Create a chamber to contain steam
    for (int x = 40; x < 45; x++) {
        automaton.setCell(x, WORLD_HEIGHT - 15, automaton.getMaterialIDByName("Stone"));
    }
    // Side walls to contain the steam
    for (int y = WORLD_HEIGHT - 15; y < WORLD_HEIGHT - 5; y++) {
        automaton.setCell(40, y, automaton.getMaterialIDByName("Stone"));
        automaton.setCell(45, y, automaton.getMaterialIDByName("Stone"));
    }
    // Add steam in the chamber
    for (int x = 41; x < 45; x++) {
        for (int y = WORLD_HEIGHT - 14; y < WORLD_HEIGHT - 10; y++) {
            automaton.setCell(x, y, automaton.getMaterialIDByName("Steam"));
        }
    }
    // Create a small hole for steam to escape
    automaton.setCell(40, WORLD_HEIGHT - 12, 0); // Air instead of stone
    
    // Add fire near where steam will escape
    for (int x = 36; x < 39; x++) {
        automaton.setCell(x, WORLD_HEIGHT - 12, automaton.getMaterialIDByName("Fire"));
    }
    
    // 3. Oil and fire test - explosive chain reaction
    
    // Create a container with walls for the oil to demonstrate containment
    for (int y = WORLD_HEIGHT - 20; y < WORLD_HEIGHT - 3; y++) {
        automaton.setCell(55, y, automaton.getMaterialIDByName("Stone"));
        automaton.setCell(70, y, automaton.getMaterialIDByName("Stone"));
    }
    // Bottom of container
    for (int x = 55; x < 70; x++) {
        automaton.setCell(x, WORLD_HEIGHT - 3, automaton.getMaterialIDByName("Stone"));
    }
    
    // Add a large pool of oil in the container
    for (int x = 56; x < 70; x++) {
        // Create more layers of oil to make the effect more visible
        for (int y = WORLD_HEIGHT - 18; y < WORLD_HEIGHT - 4; y++) {
            automaton.setCell(x, y, automaton.getMaterialIDByName("Oil"));
        }
    }
    
    // Add a small wooden structure in the oil to show how it reacts
    for (int x = 62; x < 65; x++) {
        for (int y = WORLD_HEIGHT - 8; y < WORLD_HEIGHT - 4; y++) {
            automaton.setCell(x, y, automaton.getMaterialIDByName("Wood"));
        }
    }
    
    // Add a small water pool in one corner
    for (int x = 56; x < 59; x++) {
        automaton.setCell(x, WORLD_HEIGHT - 4, automaton.getMaterialIDByName("Water"));
    }
    
    // Add multiple fire points to trigger the chain reaction more effectively
    for (int i = 0; i < 3; i++) {
        automaton.setCell(56 + i, WORLD_HEIGHT - 6, automaton.getMaterialIDByName("Fire"));
    }
    
    // Add a separate small isolated oil pond with fire
    for (int x = 40; x < 50; x++) {
        automaton.setCell(x, WORLD_HEIGHT - 5, automaton.getMaterialIDByName("Stone"));
    }
    
    // Side walls
    for (int y = WORLD_HEIGHT - 10; y < WORLD_HEIGHT - 5; y++) {
        automaton.setCell(40, y, automaton.getMaterialIDByName("Stone"));
        automaton.setCell(50, y, automaton.getMaterialIDByName("Stone"));
    }
    
    // Fill with oil
    for (int x = 41; x < 50; x++) {
        for (int y = WORLD_HEIGHT - 9; y < WORLD_HEIGHT - 5; y++) {
            automaton.setCell(x, y, automaton.getMaterialIDByName("Oil"));
        }
    }
    
    // Add fire to ignite it
    automaton.setCell(41, WORLD_HEIGHT - 9, automaton.getMaterialIDByName("Fire"));
    
    // Also add isolated fires for comparison
    // Line of isolated fire - should die out quickly
    for (int x = 55; x < 65; x += 2) {
        automaton.setCell(x, WORLD_HEIGHT - 18, automaton.getMaterialIDByName("Fire"));
    }
    
    // Add a cluster of fire that should also die out quickly
    for (int x = 65; x < 68; x++) {
        for (int y = WORLD_HEIGHT - 20; y < WORLD_HEIGHT - 17; y++) {
            automaton.setCell(x, y, automaton.getMaterialIDByName("Fire"));
        }
    }
    
    // 4. Smoke collection chamber - stone ceiling to trap smoke
    // Create a chamber with a wooden structure at the bottom and stone ceiling
    // Stone ceiling
    for (int x = 75; x < 95; x++) {
        automaton.setCell(x, 15, automaton.getMaterialIDByName("Stone"));
    }
    // Side walls
    for (int y = 15; y < WORLD_HEIGHT - 1; y++) {
        automaton.setCell(75, y, automaton.getMaterialIDByName("Stone"));
        automaton.setCell(95, y, automaton.getMaterialIDByName("Stone"));
    }
    // Wooden structure at the bottom to burn
    for (int x = 80; x < 90; x++) {
        for (int y = WORLD_HEIGHT - 8; y < WORLD_HEIGHT - 3; y++) {
            automaton.setCell(x, y, automaton.getMaterialIDByName("Wood"));
        }
    }
    // Add fire to start burning the wood
    for (int x = 82; x < 88; x++) {
        automaton.setCell(x, WORLD_HEIGHT - 3, automaton.getMaterialIDByName("Fire"));
    }
}

// Create test 6: Large-scale lava lake test (Noita-like scale)
void setupLargeScaleLavaTest(astral::CellularAutomaton& automaton) {
    // Clear the world
    automaton.clearWorld();
    
    // Explicitly set active area to the full world
    automaton.setActiveArea(0, 0, WORLD_WIDTH, WORLD_HEIGHT);
    
    // Create solid bottom and walls
    for (int x = 0; x < WORLD_WIDTH; x++) {
        automaton.setCell(x, WORLD_HEIGHT - 1, automaton.getMaterialIDByName("Stone"));
    }
    
    for (int y = 0; y < WORLD_HEIGHT; y++) {
        automaton.setCell(0, y, automaton.getMaterialIDByName("Stone"));
        automaton.setCell(WORLD_WIDTH - 1, y, automaton.getMaterialIDByName("Stone"));
    }
    
    // LARGE LAVA LAKE - covering most of the bottom of the world
    for (int x = 5; x < WORLD_WIDTH - 5; x++) {
        for (int y = WORLD_HEIGHT - 8; y < WORLD_HEIGHT - 1; y++) {
            // Create variations in the lava lake depth
            if ((x > 25 && x < 40) || (x > 60 && x < 75)) {
                // Deeper areas
                if (y >= WORLD_HEIGHT - 12) {
                    // Just use the direct material ID method, the cell will be properly initialized
                    automaton.setCell(x, y, automaton.getMaterialIDByName("Lava"));
                }
            } else {
                // Standard depth
                automaton.setCell(x, y, automaton.getMaterialIDByName("Lava"));
            }
        }
    }
    
    // Add stone islands in the lava for visual interest
    // First island
    for (int x = 15; x < 25; x++) {
        int height = 3 + (int)(2 * sin((x - 15) * 0.6));
        for (int y = WORLD_HEIGHT - height - 1; y < WORLD_HEIGHT - 1; y++) {
            automaton.setCell(x, y, automaton.getMaterialIDByName("Stone"));
        }
    }
    
    // Second island
    for (int x = 45; x < 60; x++) {
        int height = 4 + (int)(3 * sin((x - 45) * 0.4));
        for (int y = WORLD_HEIGHT - height - 1; y < WORLD_HEIGHT - 1; y++) {
            automaton.setCell(x, y, automaton.getMaterialIDByName("Stone"));
        }
    }
    
    // Add a MASSIVE wooden structure above the lava
    // Central tower
    for (int x = 40; x < 45; x++) {
        for (int y = WORLD_HEIGHT - 25; y < WORLD_HEIGHT - 8; y++) {
            automaton.setCell(x, y, automaton.getMaterialIDByName("Wood"));
        }
    }
    
    // Horizontal spans
    for (int x = 20; x < 65; x++) {
        for (int y = WORLD_HEIGHT - 25; y < WORLD_HEIGHT - 22; y++) {
            automaton.setCell(x, y, automaton.getMaterialIDByName("Wood"));
        }
    }
    
    // Additional support columns
    for (int x = 25; x < 28; x++) {
        for (int y = WORLD_HEIGHT - 25; y < WORLD_HEIGHT - 12; y++) {
            automaton.setCell(x, y, automaton.getMaterialIDByName("Wood"));
        }
    }
    
    for (int x = 57; x < 60; x++) {
        for (int y = WORLD_HEIGHT - 25; y < WORLD_HEIGHT - 12; y++) {
            automaton.setCell(x, y, automaton.getMaterialIDByName("Wood"));
        }
    }
    
    // Add large oil pool on one side
    for (int x = 65; x < 95; x++) {
        for (int y = WORLD_HEIGHT - 15; y < WORLD_HEIGHT - 8; y++) {
            automaton.setCell(x, y, automaton.getMaterialIDByName("Oil"));
        }
    }
    
    // Add water pool on the other side
    for (int x = 5; x < 35; x++) {
        for (int y = WORLD_HEIGHT - 20; y < WORLD_HEIGHT - 12; y++) {
            automaton.setCell(x, y, automaton.getMaterialIDByName("Water"));
        }
    }
    
    // Add stone barrier to separate water from lava initially
    for (int y = WORLD_HEIGHT - 20; y < WORLD_HEIGHT - 8; y++) {
        automaton.setCell(35, y, automaton.getMaterialIDByName("Stone"));
    }
    
    // Add small gap in the stone barrier that will eventually let water through
    automaton.setCell(35, WORLD_HEIGHT - 12, automaton.getMaterialIDByName("Wood"));
    
    // Add initial fire to set off chain reactions
    for (int x = 42; x < 44; x++) {
        automaton.setCell(x, WORLD_HEIGHT - 9, automaton.getMaterialIDByName("Fire"));
    }
    
    // Add small pool of oil near the central wooden tower to help spread fire
    for (int x = 47; x < 52; x++) {
        for (int y = WORLD_HEIGHT - 12; y < WORLD_HEIGHT - 8; y++) {
            automaton.setCell(x, y, automaton.getMaterialIDByName("Oil"));
        }
    }
}

// Create test 5: Advanced water flow and pooling test
void setupAdvancedWaterTest(astral::CellularAutomaton& automaton) {
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
    
    // Create a series of containers at different levels
    
    // Left container
    for (int y = WORLD_HEIGHT - 15; y < WORLD_HEIGHT; y++) {
        automaton.setCell(20, y, automaton.getMaterialIDByName("Stone"));
    }
    // Add water to left container
    for (int x = 5; x < 18; x++) {
        for (int y = WORLD_HEIGHT - 12; y < WORLD_HEIGHT - 2; y++) {
            automaton.setCell(x, y, automaton.getMaterialIDByName("Water"));
        }
    }
    
    // Middle container with a hole
    for (int y = WORLD_HEIGHT - 25; y < WORLD_HEIGHT; y++) {
        if (y != WORLD_HEIGHT - 10) { // gap for water to flow through
            automaton.setCell(40, y, automaton.getMaterialIDByName("Stone"));
        }
    }
    
    // Right container - U-shaped to test water leveling
    for (int y = WORLD_HEIGHT - 20; y < WORLD_HEIGHT; y++) {
        automaton.setCell(60, y, automaton.getMaterialIDByName("Stone"));
        automaton.setCell(80, y, automaton.getMaterialIDByName("Stone"));
    }
    
    // Additional horizontal barriers to create interesting flow patterns
    for (int x = 45; x < 60; x++) {
        automaton.setCell(x, WORLD_HEIGHT - 15, automaton.getMaterialIDByName("Stone"));
    }
    
    for (int x = 60; x < 80; x++) {
        automaton.setCell(x, WORLD_HEIGHT - 5, automaton.getMaterialIDByName("Stone"));
    }
}

#ifdef USE_OPENGL
// OpenGL implementation functions
bool initOpenGL() {
    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return false;
    }
    
    // Configure GLFW
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);  // Using OpenGL 2.1 for simpler rendering
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    
    // Create a window
    gWindow = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Astral Fluid Dynamics - OpenGL Renderer", NULL, NULL);
    if (gWindow == NULL) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return false;
    }
    
    // Make the window's context current
    glfwMakeContextCurrent(gWindow);
    
    // Set framebuffer size callback
    glfwSetFramebufferSizeCallback(gWindow, framebuffer_size_callback);
    
    // Initialize GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return false;
    }
    
    // Set up the viewport
    glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
    
    // Set up orthographic projection
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(viewportX, viewportX + WORLD_WIDTH / zoomLevel, 
            viewportY + WORLD_HEIGHT / zoomLevel, viewportY, -1, 1);
    
    // Switch back to modelview
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    
    // Simple bare-bones setup - only what we need
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    
    return true;
}
#endif

#ifdef USE_OPENGL
void drawWorld(const astral::CellularAutomaton& automaton) {
    // One-time debug output
    static bool firstRun = true;
    if (firstRun) {
        std::cout << "DEBUG - OpenGL rendering initialized" << std::endl;
        firstRun = false;
    }
    
    // Set point size based on zoom level (but ensure it's visible)
    glPointSize(std::max(1.5f * zoomLevel, 1.0f));
    
    // Begin rendering quads instead of points for square cells
    glBegin(GL_QUADS);
    
    // Calculate visible region of the world
    int startX = std::max(0, static_cast<int>(viewportX));
    int endX = std::min(WORLD_WIDTH, static_cast<int>(viewportX + WORLD_WIDTH / zoomLevel) + 1);
    int startY = std::max(0, static_cast<int>(viewportY));
    int endY = std::min(WORLD_HEIGHT, static_cast<int>(viewportY + WORLD_HEIGHT / zoomLevel) + 1);
    
    // Draw only cells in the visible region
    for (int y = startY; y < endY; y++) {
        for (int x = startX; x < endX; x++) {
            const astral::Cell& cell = automaton.getCell(x, y);
            
            // Skip empty cells
            if (cell.material == automaton.getMaterialIDByName("Air")) {
                continue;
            }
            
            // Simple material coloring with no alpha channel
            switch (cell.material) {
                case 0: // Air - transparent, should be skipped
                    continue;
                case 1: // Stone - Gray
                    glColor3f(0.5f, 0.5f, 0.5f);
                    break;
                case 2: // Sand - Yellow
                    glColor3f(0.76f, 0.7f, 0.5f);
                    break;
                case 3: // Water - Blue
                    glColor3f(0.0f, 0.4f, 0.8f);
                    break;
                case 4: // Oil - Dark Brown
                    glColor3f(0.25f, 0.15f, 0.0f);
                    break;
                case 5: // Lava - Red/Orange
                    glColor3f(1.0f, 0.3f, 0.0f);
                    break;
                case 6: // Fire - Orange
                    glColor3f(1.0f, 0.6f, 0.1f);
                    break;
                case 7: // Steam - Light Blue
                    glColor3f(0.8f, 0.9f, 1.0f);
                    break;
                case 8: // Smoke - Dark Gray
                    glColor3f(0.2f, 0.2f, 0.2f);
                    break;
                case 9: // Wood - Brown
                    glColor3f(0.6f, 0.4f, 0.2f);
                    break;
                default: // Unknown - Magenta
                    glColor3f(1.0f, 0.0f, 1.0f);
                    break;
            }
            
            // Draw a simple quad without any overlap tricks
            float size = 1.0f; // Size of each cell
            glVertex2f(x, y);
            glVertex2f(x + size, y);
            glVertex2f(x + size, y + size);
            glVertex2f(x, y + size);
        }
    }
    
    // End rendering
    glEnd();
    
    // Draw brush outline at mouse position when mouse is in window
    if (leftMouseDown || rightMouseDown) {
        // Convert screen coordinates to world coordinates
        int worldX = viewportX + mouseX * (WORLD_WIDTH / zoomLevel) / SCREEN_WIDTH;
        int worldY = viewportY + mouseY * (WORLD_HEIGHT / zoomLevel) / SCREEN_HEIGHT;
        
        // Draw brush outline
        glColor4f(1.0f, 1.0f, 1.0f, 0.5f);
        glPointSize(1.0f);
        glBegin(GL_LINE_LOOP);
        for (int i = 0; i < 20; i++) {
            float angle = 2.0f * 3.14159f * i / 20;
            float x = worldX + brushSize * cos(angle);
            float y = worldY + brushSize * sin(angle);
            glVertex2f(x, y);
        }
        glEnd();
    }
}
#endif

#ifdef USE_OPENGL
void processInput(GLFWwindow* window, astral::CellularAutomaton& automaton) {
    // Check if escape is pressed to exit
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }
    
    // Handle mouse painting
    if (leftMouseDown) {
        // Convert screen coordinates to world coordinates
        int worldX = viewportX + mouseX * (WORLD_WIDTH / zoomLevel) / SCREEN_WIDTH;
        int worldY = viewportY + mouseY * (WORLD_HEIGHT / zoomLevel) / SCREEN_HEIGHT;
        
        // Paint with current material using a circular brush
        for (int dy = -brushSize; dy <= brushSize; dy++) {
            for (int dx = -brushSize; dx <= brushSize; dx++) {
                if (dx*dx + dy*dy <= brushSize*brushSize) {
                    int x = worldX + dx;
                    int y = worldY + dy;
                    
                    // Check bounds
                    if (x > 0 && x < WORLD_WIDTH - 1 && y > 0 && y < WORLD_HEIGHT - 1) {
                        automaton.setCell(x, y, currentMaterial);
                    }
                }
            }
        }
    }
    
    // Handle erasing with right mouse button
    if (rightMouseDown) {
        // Convert screen coordinates to world coordinates
        int worldX = viewportX + mouseX * (WORLD_WIDTH / zoomLevel) / SCREEN_WIDTH;
        int worldY = viewportY + mouseY * (WORLD_HEIGHT / zoomLevel) / SCREEN_HEIGHT;
        
        // Erase (set to air) using a circular brush
        for (int dy = -brushSize; dy <= brushSize; dy++) {
            for (int dx = -brushSize; dx <= brushSize; dx++) {
                if (dx*dx + dy*dy <= brushSize*brushSize) {
                    int x = worldX + dx;
                    int y = worldY + dy;
                    
                    // Check bounds and don't erase walls
                    if (x > 1 && x < WORLD_WIDTH - 2 && y > 0 && y < WORLD_HEIGHT - 2) {
                        automaton.setCell(x, y, automaton.getMaterialIDByName("Air"));
                    }
                }
            }
        }
    }
    
    // Handle panning with middle mouse button
    if (middleMouseDown) {
        int dx = mouseX - lastMouseX;
        int dy = mouseY - lastMouseY;
        
        if (dx != 0 || dy != 0) {
            // Convert screen movement to world movement based on zoom
            float worldDx = -dx * (WORLD_WIDTH / zoomLevel) / SCREEN_WIDTH;
            float worldDy = -dy * (WORLD_HEIGHT / zoomLevel) / SCREEN_HEIGHT;
            
            // Update viewport position
            viewportX += worldDx;
            viewportY += worldDy;
            
            // Clamp viewport to valid range
            viewportX = std::max(0.0f, std::min(viewportX, static_cast<float>(WORLD_WIDTH - WORLD_WIDTH / zoomLevel)));
            viewportY = std::max(0.0f, std::min(viewportY, static_cast<float>(WORLD_HEIGHT - WORLD_HEIGHT / zoomLevel)));
            
            // Update projection matrix for new viewport
            glMatrixMode(GL_PROJECTION);
            glLoadIdentity();
            glOrtho(viewportX, viewportX + WORLD_WIDTH / zoomLevel, 
                    viewportY + WORLD_HEIGHT / zoomLevel, viewportY, -1, 1);
            glMatrixMode(GL_MODELVIEW);
            glLoadIdentity();
        }
        
        // Update last mouse position
        lastMouseX = mouseX;
        lastMouseY = mouseY;
    }
    
    // Handle brush size adjustments with + and - keys
    if (glfwGetKey(window, GLFW_KEY_EQUAL) == GLFW_PRESS) { // '+' key
        static bool plusWasPressed = false;
        if (!plusWasPressed) {
            brushSize++;
            if (brushSize > 20) brushSize = 20;
            std::cout << "Brush size: " << brushSize << std::endl;
            plusWasPressed = true;
        }
    } else {
        static bool plusWasPressed = false;
        plusWasPressed = false;
    }
    
    if (glfwGetKey(window, GLFW_KEY_MINUS) == GLFW_PRESS) { // '-' key
        static bool minusWasPressed = false;
        if (!minusWasPressed) {
            brushSize--;
            if (brushSize < 1) brushSize = 1;
            std::cout << "Brush size: " << brushSize << std::endl;
            minusWasPressed = true;
        }
    } else {
        static bool minusWasPressed = false;
        minusWasPressed = false;
    }
    
    // Handle simulation speed adjustments with [ and ] keys
    if (glfwGetKey(window, GLFW_KEY_LEFT_BRACKET) == GLFW_PRESS) { // '[' key
        static bool bracketWasPressed = false;
        if (!bracketWasPressed) {
            simulationSpeed -= 0.25f;
            if (simulationSpeed < 0.25f) simulationSpeed = 0.25f;
            std::cout << "Simulation speed: " << simulationSpeed << "x" << std::endl;
            bracketWasPressed = true;
        }
    } else {
        static bool bracketWasPressed = false;
        bracketWasPressed = false;
    }
    
    if (glfwGetKey(window, GLFW_KEY_RIGHT_BRACKET) == GLFW_PRESS) { // ']' key
        static bool bracketWasPressed = false;
        if (!bracketWasPressed) {
            simulationSpeed += 0.25f;
            if (simulationSpeed > 5.0f) simulationSpeed = 5.0f;
            std::cout << "Simulation speed: " << simulationSpeed << "x" << std::endl;
            bracketWasPressed = true;
        }
    } else {
        static bool bracketWasPressed = false;
        bracketWasPressed = false;
    }
    
    // Handle space to pause/resume simulation
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
        static bool spaceWasPressed = false;
        if (!spaceWasPressed) {
            simulationPaused = !simulationPaused;
            if (simulationPaused) {
                std::cout << "Simulation paused" << std::endl;
            } else {
                std::cout << "Simulation resumed" << std::endl;
            }
            spaceWasPressed = true;
        }
    } else {
        static bool spaceWasPressed = false;
        spaceWasPressed = false;
    }
}
#endif

#ifdef USE_OPENGL
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
    SCREEN_WIDTH = width;
    SCREEN_HEIGHT = height;
    
    // Update projection matrix for new aspect ratio
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(viewportX, viewportX + WORLD_WIDTH / zoomLevel, 
            viewportY + WORLD_HEIGHT / zoomLevel, viewportY, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        leftMouseDown = (action == GLFW_PRESS);
    } else if (button == GLFW_MOUSE_BUTTON_RIGHT) {
        rightMouseDown = (action == GLFW_PRESS);
    } else if (button == GLFW_MOUSE_BUTTON_MIDDLE) {
        middleMouseDown = (action == GLFW_PRESS);
        if (middleMouseDown) {
            lastMouseX = mouseX;
            lastMouseY = mouseY;
        }
    }
}

void cursor_position_callback(GLFWwindow* window, double xpos, double ypos) {
    mouseX = static_cast<int>(xpos);
    mouseY = static_cast<int>(ypos);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    // Get mouse position before zoom for zoom-to-cursor functionality
    int worldMouseX = viewportX + mouseX * (WORLD_WIDTH / zoomLevel) / SCREEN_WIDTH;
    int worldMouseY = viewportY + mouseY * (WORLD_HEIGHT / zoomLevel) / SCREEN_HEIGHT;
    
    // Adjust zoom level with mouse wheel
    float oldZoom = zoomLevel;
    zoomLevel += static_cast<float>(yoffset) * 0.1f * zoomLevel; // Scale zoom speed with current level
    
    // Clamp zoom level
    if (zoomLevel < 0.1f) zoomLevel = 0.1f;
    if (zoomLevel > 4.0f) zoomLevel = 4.0f;
    
    // Adjust viewport to zoom toward mouse position
    if (oldZoom != zoomLevel) {
        // Calculate new viewport position
        float mouseRelativeX = (mouseX / (float)SCREEN_WIDTH);
        float mouseRelativeY = (mouseY / (float)SCREEN_HEIGHT);
        
        float viewportWidth = WORLD_WIDTH / zoomLevel;
        float viewportHeight = WORLD_HEIGHT / zoomLevel;
        
        viewportX = worldMouseX - mouseRelativeX * viewportWidth;
        viewportY = worldMouseY - mouseRelativeY * viewportHeight;
        
        // Clamp viewport to valid range
        viewportX = std::max(0.0f, std::min(viewportX, static_cast<float>(WORLD_WIDTH - viewportWidth)));
        viewportY = std::max(0.0f, std::min(viewportY, static_cast<float>(WORLD_HEIGHT - viewportHeight)));
        
        // Update projection matrix for new zoom level
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(viewportX, viewportX + viewportWidth, 
                viewportY + viewportHeight, viewportY, -1, 1);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        
        std::cout << "Zoom level: " << zoomLevel << "x" << std::endl;
    }
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    // Only process on key press (not release)
    if (action == GLFW_PRESS) {
        astral::CellularAutomaton* automaton = static_cast<astral::CellularAutomaton*>(glfwGetWindowUserPointer(window));
        if (!automaton) return;
        
        // Materials selection
        switch (key) {
            case GLFW_KEY_1:
                currentMaterial = automaton->getMaterialIDByName("Sand");
                std::cout << "Selected: Sand" << std::endl;
                break;
            case GLFW_KEY_2:
                currentMaterial = automaton->getMaterialIDByName("Water");
                std::cout << "Selected: Water" << std::endl;
                break;
            case GLFW_KEY_3:
                currentMaterial = automaton->getMaterialIDByName("Stone");
                std::cout << "Selected: Stone" << std::endl;
                break;
            case GLFW_KEY_4:
                currentMaterial = automaton->getMaterialIDByName("Wood");
                std::cout << "Selected: Wood" << std::endl;
                break;
            case GLFW_KEY_5:
                currentMaterial = automaton->getMaterialIDByName("Oil");
                std::cout << "Selected: Oil" << std::endl;
                break;
            case GLFW_KEY_6:
                currentMaterial = automaton->getMaterialIDByName("Fire");
                std::cout << "Selected: Fire" << std::endl;
                break;
            case GLFW_KEY_7:
                currentMaterial = automaton->getMaterialIDByName("Lava");
                std::cout << "Selected: Lava" << std::endl;
                break;
            case GLFW_KEY_8:
                currentMaterial = automaton->getMaterialIDByName("Steam");
                std::cout << "Selected: Steam" << std::endl;
                break;
            case GLFW_KEY_9:
                currentMaterial = automaton->getMaterialIDByName("Smoke");
                std::cout << "Selected: Smoke" << std::endl;
                break;
        }
    }
}
#endif

#ifdef USE_OPENGL
// OpenGL main loop for running the large lava test
void runOpenGLLavaTest(astral::CellularAutomaton& automaton) {
    // Set up the lava lake test scene
    setupLargeScaleLavaTest(automaton);
    
    // Set default material to lava
    currentMaterial = automaton.getMaterialIDByName("Lava");
    
    // Store automaton pointer in window user data for callbacks
    glfwSetWindowUserPointer(gWindow, &automaton);
    
    // Set callbacks
    glfwSetMouseButtonCallback(gWindow, mouse_button_callback);
    glfwSetCursorPosCallback(gWindow, cursor_position_callback);
    glfwSetScrollCallback(gWindow, scroll_callback);
    glfwSetKeyCallback(gWindow, key_callback);
    
    // Main timing variables
    astral::Timer timer;
    float deltaTime = 0.0f;
    
    // Performance metrics
    int frameCount = 0;
    float frameTimeAccumulator = 0.0f;
    float frameTimeAverage = 0.0f;
    
    // Main loop
    while (!glfwWindowShouldClose(gWindow)) {
        // Calculate delta time
        timer.update();
        deltaTime = static_cast<float>(timer.getDeltaTime());
        
        // Update FPS counter
        frameCount++;
        frameTimeAccumulator += deltaTime;
        if (frameTimeAccumulator >= 1.0f) {
            frameTimeAverage = frameTimeAccumulator / frameCount;
            std::cout << "FPS: " << frameCount << " | Frame Time: " << (frameTimeAverage * 1000.0f) << "ms";
            
            // Show simulation stats
            auto stats = automaton.getSimulationStats();
            std::cout << " | Active Cells: " << stats.activeCells << "/" << (WORLD_WIDTH * WORLD_HEIGHT);
            std::cout << " | Simulation Speed: " << simulationSpeed << "x";
            if (simulationPaused) std::cout << " (PAUSED)";
            std::cout << std::endl;
            
            frameCount = 0;
            frameTimeAccumulator = 0.0f;
        }
        
        // Process input
        processInput(gWindow, automaton);
        
        // Ensure active area is set
        automaton.setActiveArea(0, 0, WORLD_WIDTH, WORLD_HEIGHT);
        
        // Update simulation (unless paused)
        if (!simulationPaused) {
            // Apply simulation speed multiplier
            float adjustedDeltaTime = deltaTime * simulationSpeed;
            // Can update multiple times per frame for higher speeds
            int iterationCount = static_cast<int>(simulationSpeed);
            float remainingTime = adjustedDeltaTime - static_cast<float>(iterationCount);
            
            // Full iterations
            for (int i = 0; i < iterationCount; i++) {
                // Force cells to be active occasionally
                if (frameCount % 10 == 0) {
                    static bool triggered = false;
                    
                    // Add some movement to random areas to kickstart activity
                    for (int i = 0; i < 20; i++) {
                        int x = 10 + (rand() % (WORLD_WIDTH - 20));
                        int y = WORLD_HEIGHT - 10 + (rand() % 5);
                        
                        // Push random cells
                        automaton.applyForce(x, y, 
                            glm::vec2((rand() % 100) / 50.0f - 1.0f, -0.5f), 
                            2.0f, 5.0f);
                    }
                }
                
                automaton.update(1.0f/60.0f); // Fixed time step for stability
            }
            
            // Remaining partial iteration
            if (remainingTime > 0.0f) {
                automaton.update(remainingTime);
            }
        }
        
        // Clear the screen
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);  // Pure black background
        glClear(GL_COLOR_BUFFER_BIT);
        
        // Reset modelview matrix
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        
        // Draw the world
        drawWorld(automaton);
        
        // Swap buffers and poll events
        glfwSwapBuffers(gWindow);
        glfwPollEvents();
    }
    
    // Clean up
    glfwTerminate();
}
#endif

int main(int argc, char* argv[]) {
    std::cout << "Astral Fluid Dynamics Test" << std::endl;
    
    // Flag for continuous simulation (will run until interrupted)
    bool runContinuously = false;
    // Default starting test
    int startingTest = 1;
    int endingTest = 6; // Default run all tests
    
    // Check command line arguments
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--continuous" || arg == "-c") {
            runContinuously = true;
            std::cout << "Running in continuous mode. Press Ctrl+C to stop." << std::endl;
        } else if (arg == "--test" || arg == "-t") {
            if (i + 1 < argc) {
                try {
                    startingTest = std::stoi(argv[i + 1]);
                    endingTest = startingTest; // Only run the specified test
                    if (startingTest < 1 || startingTest > 6) {
                        std::cout << "Invalid test number: " << startingTest << ". Using default (1)." << std::endl;
                        startingTest = 1;
                        endingTest = 6;
                    } else {
                        std::cout << "Running test " << startingTest << " only." << std::endl;
                    }
                } catch (...) {
                    std::cout << "Invalid test number. Using default (all tests)." << std::endl;
                }
                i++; // Skip the next parameter
            }
        } else if (arg == "--graphics" || arg == "-g") {
            runningGraphicsMode = true;
            std::cout << "Running in OpenGL graphics mode" << std::endl;
        } else if (arg == "--help" || arg == "-h") {
            std::cout << "Usage: cellular_fluid_test [options]" << std::endl;
            std::cout << "Options:" << std::endl;
            std::cout << "  -c, --continuous     Run simulation continuously until interrupted" << std::endl;
            std::cout << "  -t, --test <num>     Run only the specified test (1-6)" << std::endl;
            std::cout << "  -g, --graphics       Run in OpenGL graphics mode (only works with test 6)" << std::endl;
            std::cout << "  -h, --help           Show this help message" << std::endl;
            std::cout << "Available tests:" << std::endl;
            std::cout << "  1: Water Flow Test" << std::endl;
            std::cout << "  2: Water and Oil Separation Test" << std::endl;
            std::cout << "  3: Gas Physics Test (Steam and Smoke)" << std::endl;
            std::cout << "  4: Fire Interactions Test" << std::endl;
            std::cout << "  5: Advanced Water Flow and Pooling Test" << std::endl;
            std::cout << "  6: Large-Scale Lava Lake Test" << std::endl;
            return 0;
        }
    }
    
    // Check if graphics mode is requested and test is set to 6 (Lava test)
    if (runningGraphicsMode) {
#ifdef USE_OPENGL
        if (startingTest != 6) {
            std::cout << "Graphics mode only works with test 6 (Lava Test). Forcing test 6." << std::endl;
            startingTest = 6;
            endingTest = 6;
        }
        
        // Initialize OpenGL and render the lava test
        std::cout << "Starting OpenGL renderer for large-scale lava test..." << std::endl;
        
        // Initialize the automaton with the larger world dimensions
        astral::CellularAutomaton automaton(WORLD_WIDTH, WORLD_HEIGHT);
        automaton.initialize();
        
        // Set active area to full world explicitly
        automaton.setActiveArea(0, 0, WORLD_WIDTH, WORLD_HEIGHT);
        
        // Initialize OpenGL
        if (!initOpenGL()) {
            std::cerr << "Failed to initialize OpenGL. Exiting." << std::endl;
            return -1;
        }
        
        // Run the lava test with OpenGL
        runOpenGLLavaTest(automaton);
        
        return 0;
#else
        std::cerr << "OpenGL support not available. Compile with OpenGL/GLFW to enable graphics mode." << std::endl;
        std::cerr << "Continuing with ASCII display mode..." << std::endl;
        runningGraphicsMode = false;
#endif
    }
    
    // If not running in graphics mode, continue with the ASCII version
    // Initialize the automaton
    astral::CellularAutomaton automaton(WORLD_WIDTH, WORLD_HEIGHT);
    automaton.initialize();
    
    // Test types (no longer needed since we use startingTest and endingTest)
    
    for (int currentTest = startingTest; currentTest <= endingTest; currentTest++) {
        std::cout << "\n=== Test " << currentTest << " ===\n" << std::endl;
        
        // Setup the appropriate test
        switch (currentTest) {
            case 1:
                std::cout << "Water Flow Test" << std::endl;
                setupWaterFlowTest(automaton);
                break;
            case 2:
                std::cout << "Water and Oil Separation Test" << std::endl;
                setupWaterOilSeparationTest(automaton);
                break;
            case 3:
                std::cout << "Gas Physics Test (Steam and Smoke)" << std::endl;
                setupGasPhysicsTest(automaton);
                break;
            case 4:
                std::cout << "Fire Interactions Test" << std::endl;
                setupFireTest(automaton);
                break;
            case 5:
                std::cout << "Advanced Water Flow and Pooling Test" << std::endl;
                setupAdvancedWaterTest(automaton);
                break;
            case 6:
                std::cout << "Large-Scale Lava Lake Test" << std::endl;
                setupLargeScaleLavaTest(automaton);
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
        const int displayInterval = 1; // Display every step
        
        // Material counter variables
        int initialWaterCount = 0;
        int initialOilCount = 0;
        int initialSteamCount = 0;
        int initialSmokeCount = 0;
        int initialWoodCount = 0;
        int initialStoneCount = 0;
        int initialLavaCount = 0;
        bool isFirstStep = true;
        
        for (int step = 1; step <= maxSteps; step++) {
            // Update simulation every step
            timer.update();
            automaton.update(deltaTime);
            
            // Display every step
            if (step % displayInterval == 0) {
                std::cout << "\nStep " << step << ":" << std::endl;
                displayWorld(automaton);
                
                // Count materials for debugging
                int waterCount = 0;
                int oilCount = 0;
                int steamCount = 0;
                int smokeCount = 0;
                int woodCount = 0;
                int stoneCount = 0;
                int lavaCount = 0;
                
                // Count various materials in the world
                for (int y = 0; y < WORLD_HEIGHT; y++) {
                    for (int x = 0; x < WORLD_WIDTH; x++) {
                        const astral::Cell& cell = automaton.getCell(x, y);
                        if (cell.material == automaton.getMaterialIDByName("Water")) {
                            waterCount++;
                        } else if (cell.material == automaton.getMaterialIDByName("Oil")) {
                            oilCount++;
                        } else if (cell.material == automaton.getMaterialIDByName("Steam")) {
                            steamCount++;
                        } else if (cell.material == automaton.getMaterialIDByName("Smoke")) {
                            smokeCount++;
                        } else if (cell.material == automaton.getMaterialIDByName("Wood")) {
                            woodCount++;
                        } else if (cell.material == automaton.getMaterialIDByName("Stone")) {
                            stoneCount++;
                        } else if (cell.material == automaton.getMaterialIDByName("Lava")) {
                            lavaCount++;
                        }
                    }
                }
                
                // Store initial counts for comparing later
                if (isFirstStep) {
                    initialWaterCount = waterCount;
                    initialOilCount = oilCount;
                    initialSteamCount = steamCount;
                    initialSmokeCount = smokeCount;
                    initialWoodCount = woodCount;
                    initialStoneCount = stoneCount;
                    initialLavaCount = lavaCount;
                    isFirstStep = false;
                }
                
                // Display stats
                auto stats = automaton.getSimulationStats();
                std::cout << "Active cells: " << stats.activeCells
                          << " | Update time: " << (timer.getDeltaTime() * 1000.0) << "ms" << std::endl;
                
                // Display liquid material counts
                std::cout << "Water: " << waterCount << " (initial: " << initialWaterCount << ")"
                          << " | Oil: " << oilCount << " (initial: " << initialOilCount << ")";
                          
                // Display gas material counts if in gas test
                if (currentTest == 3) {
                    std::cout << "\nSteam: " << steamCount << " (initial: " << initialSteamCount << ")"
                              << " | Smoke: " << smokeCount << " (initial: " << initialSmokeCount << ")";
                    
                    // Note about gas dissipation
                    if (steamCount < initialSteamCount || smokeCount < initialSmokeCount) {
                        std::cout << "\nNote: Gas dissipation is an intended feature to simulate real-world gas behavior";
                    }
                }
                
                // Display fire counts and info if in fire test
                if (currentTest == 4) {
                    // Count fire cells
                    int fireCount = 0;
                    for (int y = 0; y < WORLD_HEIGHT; y++) {
                        for (int x = 0; x < WORLD_WIDTH; x++) {
                            const astral::Cell& cell = automaton.getCell(x, y);
                            if (cell.material == automaton.getMaterialIDByName("Fire")) {
                                fireCount++;
                            }
                        }
                    }
                    
                    std::cout << "\nFire: " << fireCount;
                    std::cout << " | Smoke: " << smokeCount << " | Steam: " << steamCount;
                    
                    // Note about fire mechanics
                    if (step > 1) {
                        std::cout << "\nNote: Fire naturally burns out over time and requires fuel to sustain";
                    }
                }
                
                // Display lava information if in lava test
                if (currentTest == 6) {
                    std::cout << "\nLava: " << lavaCount << " (initial: " << initialLavaCount << ")";
                    
                    // Count fire and solidified stone
                    int fireCount = 0;
                    int solidifiedStoneCount = stoneCount - initialStoneCount;
                    
                    for (int y = 0; y < WORLD_HEIGHT; y++) {
                        for (int x = 0; x < WORLD_WIDTH; x++) {
                            const astral::Cell& cell = automaton.getCell(x, y);
                            if (cell.material == automaton.getMaterialIDByName("Fire")) {
                                fireCount++;
                            }
                        }
                    }
                    
                    std::cout << " | Fire: " << fireCount;
                    std::cout << " | Solidified Stone: " << (solidifiedStoneCount > 0 ? solidifiedStoneCount : 0);
                    
                    if (step > 1) {
                        std::cout << "\nNote: Lava naturally solidifies over time, especially near edges";
                    }
                }
                
                std::cout << std::endl;
                
                // Add a small delay to allow viewing the progress
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                
                // For continuous mode, reset counter after 1000 steps
                if (runContinuously && step >= 1000) {
                    std::cout << "\nResetting step counter..." << std::endl;
                    step = 0;
                }
            }
        }
        
        // In continuous mode, only run the specified test
        // This is already handled by setting startingTest == endingTest
    }
    
    std::cout << "\nFluid dynamics test complete!" << std::endl;
    return 0;
}