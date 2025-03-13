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
const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;

// World dimensions
const int WORLD_WIDTH = 400;  // Reduced world width
const int WORLD_HEIGHT = 400; // Reduced world height

// Camera parameters
float cameraX = 200.0f;  // Center camera on the world
float cameraY = 200.0f;  // Center camera on the world
float cameraZoom = 3.0f;  // Further increased default zoom level

// Mouse state
bool leftMouseDown = false;
bool rightMouseDown = false;
int mouseX = 0;
int mouseY = 0;
int prevMouseX = 0;
int prevMouseY = 0;
int brushSize = 5;

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
glm::vec4 getMaterialColor(astral::MaterialID material, const astral::CellularAutomaton& automaton);

// Global GLFW window pointer for callbacks
GLFWwindow* gWindow = nullptr;

// Forward declaration needed for getMaterialRegistry
namespace astral {
    class MaterialRegistry;
    CellularAutomaton* getAutomatonFromWindow(GLFWwindow* window) {
        return static_cast<CellularAutomaton*>(glfwGetWindowUserPointer(window));
    }
}

// Main entry point
int main() {
    std::cout << "Physics Sandbox Example" << std::endl;
    
    // Initialize the cellular automaton with our world dimensions
    astral::CellularAutomaton automaton(WORLD_WIDTH, WORLD_HEIGHT);
    automaton.initialize();
    
    // Generate an empty sandbox world to start
    automaton.generateWorld(astral::WorldTemplate::SANDBOX);
    
    // Set default material to sand
    currentMaterial = automaton.getMaterialIDByName("Sand");
    
    // Initialize OpenGL and create window
    if (!initOpenGL()) {
        std::cerr << "Failed to initialize OpenGL" << std::endl;
        return -1;
    }
    
    // Set automaton pointer in window user data
    glfwSetWindowUserPointer(gWindow, &automaton);
    
    // Set mouse callbacks
    glfwSetMouseButtonCallback(gWindow, mouse_button_callback);
    glfwSetCursorPosCallback(gWindow, cursor_position_callback);
    glfwSetScrollCallback(gWindow, scroll_callback);
    glfwSetKeyCallback(gWindow, key_callback);
    
    // Main timing variables
    astral::Timer timer;
    float deltaTime = 0.0f;
    int frameCounter = 0;
    
    // Main loop
    while (!glfwWindowShouldClose(gWindow)) {
        // Calculate delta time
        timer.update();
        deltaTime = static_cast<float>(timer.getDeltaTime());
        
        // Limit and print FPS information
        const auto& stats = automaton.getSimulationStats();
        if (frameCounter++ % 30 == 0) {
            int fps = static_cast<int>(1.0f / deltaTime);
            std::cout << "FPS: " << fps << " | Frame Time: " 
                      << (deltaTime * 1000.0f) << "ms | Active Cells: " 
                      << stats.activeCells << "/" << stats.totalCells 
                      << " | Simulation Speed: " << automaton.getTimeScale() << "x" << std::endl;
        }
        
        // Process input
        processInput(gWindow, automaton);
        
        // Set active area based on camera position to optimize simulation
        int visibleStartX = static_cast<int>(cameraX - (SCREEN_WIDTH / 2) / cameraZoom);
        int visibleStartY = static_cast<int>(cameraY - (SCREEN_HEIGHT / 2) / cameraZoom);
        int visibleWidth = static_cast<int>(SCREEN_WIDTH / cameraZoom);
        int visibleHeight = static_cast<int>(SCREEN_HEIGHT / cameraZoom);
        automaton.setActiveArea(visibleStartX, visibleStartY, visibleWidth, visibleHeight);
        
        // Update simulation
        automaton.update(deltaTime);
        
        // Clear the screen
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        
        // Draw the world
        drawWorld(automaton);
        
        // Display FPS and material info
        // (In a real application, we would use text rendering for this)
        
        // Print some debug info
        if (automaton.isSimulationPaused()) {
            static int frameCounter = 0;
            if (frameCounter++ % 60 == 0) { // Only print every 60 frames when paused
                std::cout << "Current materials: "
                          << "1: Sand, 2: Water, 3: Stone, 4: Wood, 5: Oil, 6-9: More materials\n"
                          << "Controls: WASD - Move, Space - Pause, F - Create water dam test, R - Reset\n"
                          << "         LMB - Place material, RMB - Delete, E - Explosion, H - Heat source\n"
                          << "Enhanced liquid simulation: Watch for pressure visualization (brighter = higher pressure)\n"
                          << "Brush size: " << brushSize << std::endl;
            }
        }
        
        // Swap buffers and poll events
        glfwSwapBuffers(gWindow);
        glfwPollEvents();
        
        // Cap frame rate to ~60 FPS
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
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
    // Create a window
    gWindow = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Astral Physics Sandbox", NULL, NULL);
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
    
    // Set up basic OpenGL state
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // DISABLE POINT SMOOTHING to ensure square points
    glDisable(GL_POINT_SMOOTH);
    
    // Disable other smoothing/antialiasing
    glDisable(GL_LINE_SMOOTH);
    glDisable(GL_POLYGON_SMOOTH);
    
    // Set pixel storage mode for better performance
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    
    return true;
}

void drawWorld(const astral::CellularAutomaton& automaton) {
    // This is a very simple and inefficient rendering approach
    // In a real application, we would use instanced rendering or batching
    
    // Calculate visible area based on camera
    int startX = static_cast<int>(cameraX - (SCREEN_WIDTH / 2) / cameraZoom);
    int startY = static_cast<int>(cameraY - (SCREEN_HEIGHT / 2) / cameraZoom);
    int endX = static_cast<int>(cameraX + (SCREEN_WIDTH / 2) / cameraZoom);
    int endY = static_cast<int>(cameraY + (SCREEN_HEIGHT / 2) / cameraZoom);
    
    // Clamp to world boundaries
    startX = std::max(0, startX);
    startY = std::max(0, startY);
    endX = std::min(WORLD_WIDTH - 1, endX);
    endY = std::min(WORLD_HEIGHT - 1, endY);
    
    // Use a different method for rendering squares
    // First set the point size to 0 to make sure we're not drawing circles
    glPointSize(1.0f);
    
    // Begin rendering quads - explicitly use GL_QUADS to ensure rectangular shapes
    glBegin(GL_QUADS);
    
    // Calculate cell size based on zoom (render as squares not points)
    float cellSize = cameraZoom;
    
    // Disable GL_POINT_SMOOTH to ensure square points
    glDisable(GL_POINT_SMOOTH);
    
    // Draw each cell in the visible area
    for (int y = startY; y <= endY; y++) {
        for (int x = startX; x <= endX; x++) {
            const astral::Cell& cell = automaton.getCell(x, y);
            
            // Skip empty cells
            if (cell.material == automaton.getMaterialIDByName("Air")) {
                continue;
            }
            
            // Get material color
            glm::vec4 color = getMaterialColor(cell.material, automaton);
            
            // Modify color based on temperature if needed
            if (cell.temperature > 50.0f) {
                // Add redness for hot materials
                float heatFactor = std::min(1.0f, (cell.temperature - 50.0f) / 950.0f);
                color.r = std::min(1.0f, color.r + heatFactor * 0.5f);
                color.g = std::max(0.0f, color.g - heatFactor * 0.3f);
                color.b = std::max(0.0f, color.b - heatFactor * 0.5f);
            }
            
            // Set color
            glColor4f(color.r, color.g, color.b, color.a);
            
            // Visualize pressure for liquids
            const astral::MaterialProperties& props = automaton.getMaterial(cell.material);
            if (props.type == astral::MaterialType::LIQUID) {
                // Display pressure through color intensity
                float pressureIndicator = std::min(1.0f, cell.pressure * 2.0f);
                glColor4f(
                    color.r * (0.7f + pressureIndicator * 0.3f),
                    color.g * (0.7f + pressureIndicator * 0.3f),
                    color.b * (0.7f + pressureIndicator * 0.3f),
                    color.a
                );
            }
            
            // Calculate screen position
            float screenX = (x - startX) * cellSize;
            float screenY = (y - startY) * cellSize;
            
            // Draw a quad instead of a point
            glVertex2f(screenX, screenY);
            glVertex2f(screenX + cellSize, screenY);
            glVertex2f(screenX + cellSize, screenY + cellSize);
            glVertex2f(screenX, screenY + cellSize);
        }
    }
    
    // End rendering
    glEnd();
}

glm::vec4 getMaterialColor(astral::MaterialID material, const astral::CellularAutomaton& automaton) {
    // For a real implementation, this would get the color from the material properties
    // For now, we'll use a simple color mapping
    
    // Check common materials by ID or name
    if (material == automaton.getMaterialIDByName("Sand")) {
        return glm::vec4(0.76f, 0.7f, 0.5f, 1.0f); // Sand color
    } else if (material == automaton.getMaterialIDByName("Water")) {
        return glm::vec4(0.0f, 0.2f, 0.8f, 0.8f); // Water color
    } else if (material == automaton.getMaterialIDByName("Stone")) {
        return glm::vec4(0.5f, 0.5f, 0.5f, 1.0f); // Stone color
    } else if (material == automaton.getMaterialIDByName("Wood")) {
        return glm::vec4(0.6f, 0.4f, 0.2f, 1.0f); // Wood color
    } else if (material == automaton.getMaterialIDByName("Oil")) {
        return glm::vec4(0.4f, 0.2f, 0.0f, 0.8f); // Oil color
    } else if (material == automaton.getMaterialIDByName("Lava")) {
        return glm::vec4(0.8f, 0.3f, 0.0f, 1.0f); // Lava color
    } else if (material == automaton.getMaterialIDByName("Fire")) {
        return glm::vec4(0.9f, 0.4f, 0.1f, 0.9f); // Fire color
    } else if (material == automaton.getMaterialIDByName("Steam")) {
        return glm::vec4(0.8f, 0.8f, 0.8f, 0.3f); // Steam color
    } else if (material == automaton.getMaterialIDByName("Smoke")) {
        return glm::vec4(0.2f, 0.2f, 0.2f, 0.5f); // Smoke color
    }
    
    // Default color for unknown materials
    return glm::vec4(1.0f, 0.0f, 1.0f, 1.0f); // Magenta for unknown
}

void processInput(GLFWwindow* window, astral::CellularAutomaton& automaton) {
    // Key state tracking variables
    static bool spaceWasPressed = false;
    static bool fWasPressed = false;
    static bool rWasPressed = false;

    // Check if escape is pressed to exit
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }
    
    // Handle mouse painting
    if (leftMouseDown) {
        // Convert screen coordinates to world coordinates
        int worldX = static_cast<int>(cameraX + (mouseX - SCREEN_WIDTH / 2) / cameraZoom);
        int worldY = static_cast<int>(cameraY + (mouseY - SCREEN_HEIGHT / 2) / cameraZoom);
        
        // Paint with current material
        automaton.paintCircle(worldX, worldY, brushSize, currentMaterial);
    }
    
    // Handle right mouse button for erasing (setting to air)
    if (rightMouseDown) {
        // Convert screen coordinates to world coordinates
        int worldX = static_cast<int>(cameraX + (mouseX - SCREEN_WIDTH / 2) / cameraZoom);
        int worldY = static_cast<int>(cameraY + (mouseY - SCREEN_HEIGHT / 2) / cameraZoom);
        
        // Erase by setting to air
        automaton.paintCircle(worldX, worldY, brushSize, automaton.getMaterialIDByName("Air"));
    }
    
    // Handle camera movement
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        cameraY -= 5.0f / cameraZoom;
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        cameraY += 5.0f / cameraZoom;
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        cameraX -= 5.0f / cameraZoom;
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        cameraX += 5.0f / cameraZoom;
    }
    
    // Handle pause/resume
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
    
    // Add a dam of water to test liquid physics
    if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS) {
        if (!fWasPressed) {
            // Create a dam of water and a barrier
            const int barrierX = WORLD_WIDTH / 2;
            const int waterStartX = WORLD_WIDTH / 3;
            const int waterWidth = WORLD_WIDTH / 6;
            const int waterHeight = WORLD_HEIGHT / 3;
            const int barrierY = WORLD_HEIGHT / 2;
            const int barrierHeight = WORLD_HEIGHT / 6;
            
            // Create stone barrier
            automaton.fillRectangle(barrierX, barrierY, 3, barrierHeight, 
                                    automaton.getMaterialIDByName("Stone"));
                                    
            // Create water reservoir
            automaton.fillRectangle(waterStartX, barrierY, waterWidth, waterHeight, 
                                    automaton.getMaterialIDByName("Water"));
                                    
            // Create oil layer on top of water to show fluid interactions
            automaton.fillRectangle(waterStartX, barrierY - waterHeight/3, waterWidth, waterHeight/3, 
                                    automaton.getMaterialIDByName("Oil"));
                                    
            std::cout << "Created water dam test setup" << std::endl;
            fWasPressed = true;
        }
    } else {
        fWasPressed = false;
    }
    
    // Handle world regeneration
    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
        if (!rWasPressed) {
            automaton.generateWorld(astral::WorldTemplate::SANDBOX);
            rWasPressed = true;
        }
    } else {
        rWasPressed = false;
    }
    
    // Handle special effects
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {
        // Create explosion at mouse position
        int worldX = static_cast<int>(cameraX + (mouseX - SCREEN_WIDTH / 2) / cameraZoom);
        int worldY = static_cast<int>(cameraY + (mouseY - SCREEN_HEIGHT / 2) / cameraZoom);
        
        automaton.createExplosion(worldX, worldY, 20.0f, 10.0f);
    }
    
    if (glfwGetKey(window, GLFW_KEY_H) == GLFW_PRESS) {
        // Create heat source at mouse position
        int worldX = static_cast<int>(cameraX + (mouseX - SCREEN_WIDTH / 2) / cameraZoom);
        int worldY = static_cast<int>(cameraY + (mouseY - SCREEN_HEIGHT / 2) / cameraZoom);
        
        automaton.createHeatSource(worldX, worldY, 500.0f, 15.0f);
    }
}

// GLFW Callbacks
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        leftMouseDown = (action == GLFW_PRESS);
    } else if (button == GLFW_MOUSE_BUTTON_RIGHT) {
        rightMouseDown = (action == GLFW_PRESS);
    }
}

void cursor_position_callback(GLFWwindow* window, double xpos, double ypos) {
    prevMouseX = mouseX;
    prevMouseY = mouseY;
    mouseX = static_cast<int>(xpos);
    mouseY = static_cast<int>(ypos);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    // Adjust brush size with scroll
    brushSize += static_cast<int>(yoffset);
    if (brushSize < 1) brushSize = 1;
    if (brushSize > 50) brushSize = 50;
    
    // If shift is held, adjust zoom instead
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS || 
        glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS) {
        cameraZoom += static_cast<float>(yoffset) * 0.1f;
        if (cameraZoom < 1.0f) cameraZoom = 1.0f; // Minimum zoom of 1.0 to prevent zooming out too far
        if (cameraZoom > 5.0f) cameraZoom = 5.0f; // Maximum zoom of 5.0 to prevent zooming in too much
        std::cout << "Zoom level: " << cameraZoom << "x" << std::endl;
    }
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    // Material selection keys
    if (action == GLFW_PRESS) {
        astral::CellularAutomaton* automaton = static_cast<astral::CellularAutomaton*>(glfwGetWindowUserPointer(window));
        if (!automaton) return;
        
        switch (key) {
            case GLFW_KEY_1:
                currentMaterial = automaton->getMaterialIDByName("Sand");
                std::cout << "Selected Sand" << std::endl;
                break;
            case GLFW_KEY_2:
                currentMaterial = automaton->getMaterialIDByName("Water");
                std::cout << "Selected Water" << std::endl;
                break;
            case GLFW_KEY_3:
                currentMaterial = automaton->getMaterialIDByName("Stone");
                std::cout << "Selected Stone" << std::endl;
                break;
            case GLFW_KEY_4:
                currentMaterial = automaton->getMaterialIDByName("Wood");
                std::cout << "Selected Wood" << std::endl;
                break;
            case GLFW_KEY_5:
                currentMaterial = automaton->getMaterialIDByName("Oil");
                std::cout << "Selected Oil" << std::endl;
                break;
            case GLFW_KEY_6:
                currentMaterial = automaton->getMaterialIDByName("Lava");
                std::cout << "Selected Lava" << std::endl;
                break;
            case GLFW_KEY_7:
                currentMaterial = automaton->getMaterialIDByName("Fire");
                std::cout << "Selected Fire" << std::endl;
                break;
            case GLFW_KEY_8:
                currentMaterial = automaton->getMaterialIDByName("Steam");
                std::cout << "Selected Steam" << std::endl;
                break;
            case GLFW_KEY_9:
                currentMaterial = automaton->getMaterialIDByName("Smoke");
                std::cout << "Selected Smoke" << std::endl;
                break;
        }
    }
}