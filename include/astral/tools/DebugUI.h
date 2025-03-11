#pragma once

#include <string>
#include <functional>
#include <vector>
#include <memory>
#include "astral/core/Profiler.h"

namespace astral {

/**
 * Debug UI for visualizing engine state and performance metrics.
 * Uses Dear ImGui for rendering.
 */
class DebugUI {
public:
    /**
     * Create a new debug UI instance.
     */
    DebugUI();
    
    /**
     * Destroy the debug UI.
     */
    ~DebugUI();
    
    /**
     * Initialize the debug UI.
     * 
     * @param window Pointer to GLFW window
     * @return True if initialization was successful, false otherwise
     */
    bool initialize(void* window);
    
    /**
     * Shutdown the debug UI.
     */
    void shutdown();
    
    /**
     * Start a new frame for the debug UI.
     */
    void beginFrame();
    
    /**
     * End the current frame and render the debug UI.
     */
    void endFrame();
    
    /**
     * Show or hide the debug UI.
     * 
     * @param visible Whether the debug UI should be visible
     */
    void setVisible(bool visible);
    
    /**
     * Check if the debug UI is visible.
     * 
     * @return True if the debug UI is visible, false otherwise
     */
    bool isVisible() const;
    
    /**
     * Toggle visibility of the debug UI.
     */
    void toggleVisibility();
    
    /**
     * Render the performance metrics panel.
     * 
     * @param metrics Performance metrics to display
     */
    void renderPerformancePanel(const PerformanceMetrics& metrics);
    
    /**
     * Render the physics debug panel.
     * 
     * @param activeChunks Number of active chunks
     * @param totalChunks Total number of chunks
     * @param activeCells Number of active cells
     * @param updatedCells Number of cells updated in the last frame
     */
    void renderPhysicsPanel(int activeChunks, int totalChunks, int activeCells, int updatedCells);
    
    /**
     * Render the memory usage panel.
     * 
     * @param memoryUsage Total memory usage in bytes
     * @param allocations Number of allocations
     */
    void renderMemoryPanel(size_t memoryUsage, int allocations);
    
    /**
     * Render a custom debug panel.
     * 
     * @param title Title of the panel
     * @param contentFunc Function to render the panel's content
     */
    void renderCustomPanel(const std::string& title, std::function<void()> contentFunc);
    
    /**
     * Render a plot of a metric's history.
     * 
     * @param label Label for the plot
     * @param values Values to plot
     * @param overlay Text overlay to display on the plot
     * @param scaleMin Minimum scale value
     * @param scaleMax Maximum scale value
     * @param size Size of the plot
     */
    void renderPlot(const std::string& label, const std::vector<float>& values, 
                   const std::string& overlay = "", 
                   float scaleMin = FLT_MAX, float scaleMax = FLT_MAX,
                   const float size[2] = nullptr);

private:
    bool visible;
    void* window;
    
    // Helper methods
    void setupDockspace();
    void renderMainMenuBar();
    void renderMetricsWindow();
};

} // namespace astral