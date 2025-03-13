#include <iostream>
#include <chrono>
#include <thread>
#include <cmath>
#include <string>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "astral/physics/CellularAutomaton.h"
#include "astral/physics/Material.h"
#include "astral/core/Timer.h"

// Window dimensions
int SCREEN_WIDTH = 800;
int SCREEN_HEIGHT = 600;

// World dimensions - keep relatively small for better performance
const int WORLD_WIDTH = 400;
const int WORLD_HEIGHT = 300;

// Mouse state
bool leftMouseDown = false;
bool rightMouseDown = false;
int mouseX = 0;
int mouseY = 0;
int brushSize = 3;

// Current selected material
astral::MaterialID currentMaterial;

// Function prototypes
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

// Main entry point
int main() {
    std::cout << "Basic Sand Physics Example" << std::endl;
    std::cout << "Controls:" << std::endl;
    std::cout << "  Left Mouse: Draw sand" << std::endl;
    std::cout << "  Right Mouse: Erase" << std::endl;
    std::cout << "  1: Sand" << std::endl;
    std::cout << "  2: Water" << std::endl;
    std::cout << "  3: Stone" << std::endl;
    std::cout << "  Space: Pause/Resume simulation" << std::endl;
    std::cout << "  R: Reset world" << std::endl;
    std::cout << "  Mouse Wheel: Change brush size" << std::endl;
    std::cout << "  Esc: Exit" << std::endl;
    
    // Initialize the cellular automaton with our world dimensions
    astral::CellularAutomaton automaton(WORLD_WIDTH, WORLD_HEIGHT);
    automaton.initialize();
    
    // Create an empty world with walls on the sides and bottom
    automaton.clearWorld();
    
    // Create bottom wall
    for (int x = 0; x < WORLD_WIDTH; x++) {
        automaton.setCell(x, WORLD_HEIGHT - 1, automaton.getMaterialIDByName("Stone"));
    }
    
    // Create side walls
    for (int y = 0; y < WORLD_HEIGHT; y++) {
        automaton.setCell(0, y, automaton.getMaterialIDByName("Stone"));
        automaton.setCell(WORLD_WIDTH - 1, y, automaton.getMaterialIDByName("Stone"));
    }
    
    // Set default material to sand
    currentMaterial = automaton.getMaterialIDByName("Sand");
    
    // Initialize OpenGL and create window
    if (!initOpenGL()) {
        std::cerr << "Failed to initialize OpenGL" << std::endl;
        return -1;
    }
    
    // Store automaton pointer in window user data for callbacks
    glfwSetWindowUserPointer(gWindow, &automaton);
    
    // Set mouse callbacks
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
            std::cout << "FPS: " << frameCount << " | Frame Time: " << (frameTimeAverage * 1000.0f) << "ms" << std::endl;
            frameCount = 0;
            frameTimeAccumulator = 0.0f;
        }
        
        // Process input
        processInput(gWindow, automaton);
        
        // Update simulation
        automaton.update(deltaTime);
        
        // Clear the screen
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        
        // Draw the world
        drawWorld(automaton);
        
        // Swap buffers and poll events
        glfwSwapBuffers(gWindow);
        glfwPollEvents();
        
        // Cap frame rate to approximately 60 FPS
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
    
    // Clean up
    glfwTerminate();
    return 0;
}

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
    gWindow = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Astral Basic Sand Physics", NULL, NULL);
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
    glOrtho(0, WORLD_WIDTH, WORLD_HEIGHT, 0, -1, 1);  // Match world coordinates (y-inverted)
    
    // Switch back to modelview
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    
    // Simple alpha blending
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    return true;
}

void drawWorld(const astral::CellularAutomaton& automaton) {
    // Set point size
    glPointSize(2.0f);
    
    // Begin rendering points
    glBegin(GL_POINTS);
    
    // Draw each cell in the world
    for (int y = 0; y < WORLD_HEIGHT; y++) {
        for (int x = 0; x < WORLD_WIDTH; x++) {
            const astral::Cell& cell = automaton.getCell(x, y);
            
            // Skip empty cells
            if (cell.material == automaton.getMaterialIDByName("Air")) {
                continue;
            }
            
            // Set color based on material
            if (cell.material == automaton.getMaterialIDByName("Sand")) {
                // Sand - yellow
                glColor3f(0.76f, 0.7f, 0.5f);
            } else if (cell.material == automaton.getMaterialIDByName("Water")) {
                // Water - blue
                glColor4f(0.0f, 0.4f, 0.8f, 0.8f);
            } else if (cell.material == automaton.getMaterialIDByName("Stone")) {
                // Stone - gray
                glColor3f(0.5f, 0.5f, 0.5f);
            } else {
                // Unknown material - magenta
                glColor3f(1.0f, 0.0f, 1.0f);
            }
            
            // Draw the point
            glVertex2i(x, y);
        }
    }
    
    // End rendering
    glEnd();
}

void processInput(GLFWwindow* window, astral::CellularAutomaton& automaton) {
    // Check if escape is pressed to exit
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }
    
    // Handle mouse painting
    if (leftMouseDown) {
        // Convert screen coordinates to world coordinates
        int worldX = mouseX * WORLD_WIDTH / SCREEN_WIDTH;
        int worldY = mouseY * WORLD_HEIGHT / SCREEN_HEIGHT;
        
        // Paint with current material using a small circle brush
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
        int worldX = mouseX * WORLD_WIDTH / SCREEN_WIDTH;
        int worldY = mouseY * WORLD_HEIGHT / SCREEN_HEIGHT;
        
        // Erase (set to air) using a small circle brush
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
    
    // Handle pause/resume
    static bool spaceWasPressed = false;
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
        if (!spaceWasPressed) {
            if (automaton.isSimulationPaused()) {
                automaton.resume();
                std::cout << "Simulation resumed" << std::endl;
            } else {
                automaton.pause();
                std::cout << "Simulation paused" << std::endl;
            }
            spaceWasPressed = true;
        }
    } else {
        spaceWasPressed = false;
    }
    
    // Handle reset
    static bool resetWasPressed = false;
    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
        if (!resetWasPressed) {
            // Clear the world
            automaton.clearWorld();
            
            // Recreate walls
            for (int x = 0; x < WORLD_WIDTH; x++) {
                automaton.setCell(x, WORLD_HEIGHT - 1, automaton.getMaterialIDByName("Stone"));
            }
            
            for (int y = 0; y < WORLD_HEIGHT; y++) {
                automaton.setCell(0, y, automaton.getMaterialIDByName("Stone"));
                automaton.setCell(WORLD_WIDTH - 1, y, automaton.getMaterialIDByName("Stone"));
            }
            
            std::cout << "World reset" << std::endl;
            resetWasPressed = true;
        }
    } else {
        resetWasPressed = false;
    }
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
    SCREEN_WIDTH = width;
    SCREEN_HEIGHT = height;
    
    // Update projection matrix for new aspect ratio
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, WORLD_WIDTH, WORLD_HEIGHT, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        leftMouseDown = (action == GLFW_PRESS);
    } else if (button == GLFW_MOUSE_BUTTON_RIGHT) {
        rightMouseDown = (action == GLFW_PRESS);
    }
}

void cursor_position_callback(GLFWwindow* window, double xpos, double ypos) {
    mouseX = static_cast<int>(xpos);
    mouseY = static_cast<int>(ypos);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    // Adjust brush size with mouse wheel
    brushSize += static_cast<int>(yoffset);
    if (brushSize < 1) brushSize = 1;
    if (brushSize > 10) brushSize = 10;
    std::cout << "Brush size: " << brushSize << std::endl;
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    // Only process on key press (not release)
    if (action == GLFW_PRESS) {
        astral::CellularAutomaton* automaton = static_cast<astral::CellularAutomaton*>(glfwGetWindowUserPointer(window));
        if (!automaton) return;
        
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
        }
    }
}