#pragma once

#include <string>
#include <chrono>
#include <unordered_map>
#include <vector>
#include <mutex>
#include <memory>
#include <functional>

namespace astral {

/**
 * Performance metrics for different aspects of the engine.
 */
struct PerformanceMetrics {
    // Frame timing
    double frameTime = 0.0;     // Total frame time in seconds
    double updateTime = 0.0;    // Time spent in update logic
    double physicsTime = 0.0;   // Time spent in physics simulation
    double renderTime = 0.0;    // Time spent in rendering
    
    // Derived metrics
    double fps = 0.0;           // Frames per second
    
    // Statistics
    int renderedCells = 0;      // Number of cells rendered this frame
    int updatedCells = 0;       // Number of cells updated this frame
    int activeChunks = 0;       // Number of active chunks
    
    // Memory
    size_t memoryUsage = 0;     // Total memory usage in bytes
    
    // Reset metrics for a new frame
    void reset() {
        frameTime = 0.0;
        updateTime = 0.0;
        physicsTime = 0.0;
        renderTime = 0.0;
        fps = 0.0;
        renderedCells = 0;
        updatedCells = 0;
        activeChunks = 0;
        memoryUsage = 0;
    }
};

/**
 * Simple profiling system for measuring performance of different parts of the engine.
 * Provides tools for timing sections of code and tracking resource usage.
 */
class Profiler {
public:
    /**
     * Get the singleton instance of the profiler.
     */
    static Profiler& getInstance();
    
    /**
     * Initialize the profiler.
     * @param enabled Whether the profiler should start enabled
     */
    void initialize(bool enabled = true);
    
    /**
     * Enable or disable profiling.
     * @param enabled Whether profiling should be enabled
     */
    void setEnabled(bool enabled);
    
    /**
     * Check if profiling is enabled.
     * @return True if profiling is enabled, false otherwise
     */
    bool isEnabled() const;
    
    /**
     * Begin a new frame of profiling.
     */
    void beginFrame();
    
    /**
     * End the current frame of profiling.
     */
    void endFrame();
    
    /**
     * Begin timing a section of code.
     * @param name Name of the section to time
     */
    void beginSection(const std::string& name);
    
    /**
     * End timing a section of code.
     * @param name Name of the section to stop timing
     */
    void endSection(const std::string& name);
    
    /**
     * Record a value for a named metric.
     * @param name Name of the metric
     * @param value Value to record
     */
    void recordValue(const std::string& name, double value);
    
    /**
     * Record memory usage for a named subsystem.
     * @param name Name of the subsystem
     * @param bytes Memory usage in bytes
     */
    void recordMemoryUsage(const std::string& name, size_t bytes);
    
    /**
     * Get the current performance metrics.
     * @return Current performance metrics
     */
    const PerformanceMetrics& getMetrics() const;
    
    /**
     * Get the history of a specific metric over recent frames.
     * @param name Name of the metric to retrieve history for
     * @param maxFrames Maximum number of frames to retrieve (0 for all available)
     * @return Vector of metric values, most recent first
     */
    std::vector<double> getMetricHistory(const std::string& name, size_t maxFrames = 0) const;
    
    /**
     * Reset all metrics and timers.
     */
    void reset();
    
    /**
     * Save profiling data to a file.
     * @param filepath Path to save the profiling data to
     * @return True if successful, false otherwise
     */
    bool saveToFile(const std::string& filepath) const;
    
private:
    Profiler();
    ~Profiler();
    
    // Disable copy/move
    Profiler(const Profiler&) = delete;
    Profiler& operator=(const Profiler&) = delete;
    Profiler(Profiler&&) = delete;
    Profiler& operator=(Profiler&&) = delete;
    
    struct ProfileSection {
        std::chrono::high_resolution_clock::time_point startTime;
        double totalTime;
        int callCount;
    };
    
    bool enabled;
    PerformanceMetrics currentMetrics;
    
    std::chrono::high_resolution_clock::time_point frameStartTime;
    std::unordered_map<std::string, ProfileSection> sections;
    std::unordered_map<std::string, std::vector<double>> metricHistory;
    std::unordered_map<std::string, size_t> memoryUsage;
    
    // Maximum history length
    size_t maxHistoryLength;
    
    // Thread safety
    mutable std::mutex mutex;
    
    // Helper methods
    double calculateFrameTime();
    void updateMemoryMetrics();
};

/**
 * Utility class for automatically timing a section of code using RAII.
 */
class ScopedTimer {
public:
    /**
     * Start timing a section.
     * @param name Name of the section to time
     * @param profiler Profiler to use (defaults to singleton instance)
     */
    explicit ScopedTimer(const std::string& name, Profiler* profiler = nullptr);
    
    /**
     * Stop timing the section.
     */
    ~ScopedTimer();
    
private:
    std::string name;
    Profiler* profiler;
};

/**
 * Macro for easily timing a section of code.
 * Usage: PROFILE_SCOPE("SectionName")
 */
#define PROFILE_SCOPE(name) astral::ScopedTimer scopedTimer##__LINE__(name)

/**
 * Macro for timing a function.
 * Usage: PROFILE_FUNCTION()
 */
#define PROFILE_FUNCTION() astral::ScopedTimer scopedTimer##__LINE__(__FUNCTION__)

} // namespace astral