#include "astral/core/Profiler.h"
#include <fstream>
#include <iostream>
#include <algorithm>
#include <nlohmann/json.hpp>

namespace astral {

// Static instance
Profiler& Profiler::getInstance() {
    static Profiler instance;
    return instance;
}

Profiler::Profiler()
    : enabled(false)
    , maxHistoryLength(300) // 5 seconds of history at 60 FPS
{
}

Profiler::~Profiler() {
}

void Profiler::initialize(bool enabled) {
    this->enabled = enabled;
    reset();
}

void Profiler::setEnabled(bool enabled) {
    this->enabled = enabled;
}

bool Profiler::isEnabled() const {
    return enabled;
}

void Profiler::beginFrame() {
    if (!enabled) return;
    
    std::lock_guard<std::mutex> lock(mutex);
    
    // Record frame start time
    frameStartTime = std::chrono::high_resolution_clock::now();
    
    // Reset per-frame section timings
    for (auto& section : sections) {
        section.second.totalTime = 0.0;
        section.second.callCount = 0;
    }
}

void Profiler::endFrame() {
    if (!enabled) return;
    
    std::lock_guard<std::mutex> lock(mutex);
    
    // Calculate frame time
    double frameTime = calculateFrameTime();
    currentMetrics.frameTime = frameTime;
    
    // Calculate FPS
    currentMetrics.fps = frameTime > 0.0 ? 1.0 / frameTime : 0.0;
    
    // Update memory metrics
    updateMemoryMetrics();
    
    // Store metrics in history
    metricHistory["FrameTime"].push_back(frameTime);
    metricHistory["FPS"].push_back(currentMetrics.fps);
    metricHistory["PhysicsTime"].push_back(currentMetrics.physicsTime);
    metricHistory["RenderTime"].push_back(currentMetrics.renderTime);
    metricHistory["UpdateTime"].push_back(currentMetrics.updateTime);
    metricHistory["MemoryUsage"].push_back(static_cast<double>(currentMetrics.memoryUsage));
    metricHistory["RenderedCells"].push_back(static_cast<double>(currentMetrics.renderedCells));
    metricHistory["UpdatedCells"].push_back(static_cast<double>(currentMetrics.updatedCells));
    metricHistory["ActiveChunks"].push_back(static_cast<double>(currentMetrics.activeChunks));
    
    // Trim history if it gets too long
    for (auto& history : metricHistory) {
        if (history.second.size() > maxHistoryLength) {
            history.second.erase(history.second.begin());
        }
    }
}

void Profiler::beginSection(const std::string& name) {
    if (!enabled) return;
    
    std::lock_guard<std::mutex> lock(mutex);
    
    // Record start time
    sections[name].startTime = std::chrono::high_resolution_clock::now();
    sections[name].callCount++;
}

void Profiler::endSection(const std::string& name) {
    if (!enabled) return;
    
    std::lock_guard<std::mutex> lock(mutex);
    
    // Find the section
    auto it = sections.find(name);
    if (it != sections.end()) {
        // Calculate elapsed time
        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - it->second.startTime);
        double elapsed = duration.count() / 1000000.0; // Convert to seconds
        
        // Add to total time for this section
        it->second.totalTime += elapsed;
        
        // Update specific metrics
        if (name == "Physics") {
            currentMetrics.physicsTime = it->second.totalTime;
        } else if (name == "Render") {
            currentMetrics.renderTime = it->second.totalTime;
        } else if (name == "Update") {
            currentMetrics.updateTime = it->second.totalTime;
        }
    }
}

void Profiler::recordValue(const std::string& name, double value) {
    if (!enabled) return;
    
    std::lock_guard<std::mutex> lock(mutex);
    
    // Store in history
    metricHistory[name].push_back(value);
    
    // Update specific metrics
    if (name == "RenderedCells") {
        currentMetrics.renderedCells = static_cast<int>(value);
    } else if (name == "UpdatedCells") {
        currentMetrics.updatedCells = static_cast<int>(value);
    } else if (name == "ActiveChunks") {
        currentMetrics.activeChunks = static_cast<int>(value);
    }
    
    // Trim history if it gets too long
    if (metricHistory[name].size() > maxHistoryLength) {
        metricHistory[name].erase(metricHistory[name].begin());
    }
}

void Profiler::recordMemoryUsage(const std::string& name, size_t bytes) {
    if (!enabled) return;
    
    std::lock_guard<std::mutex> lock(mutex);
    
    // Store memory usage
    memoryUsage[name] = bytes;
}

const PerformanceMetrics& Profiler::getMetrics() const {
    return currentMetrics;
}

std::vector<double> Profiler::getMetricHistory(const std::string& name, size_t maxFrames) const {
    std::lock_guard<std::mutex> lock(mutex);
    
    // Find the metric history
    auto it = metricHistory.find(name);
    if (it != metricHistory.end()) {
        const auto& history = it->second;
        
        // Return the requested number of frames
        if (maxFrames > 0 && maxFrames < history.size()) {
            return std::vector<double>(history.end() - maxFrames, history.end());
        }
        
        return history;
    }
    
    // Return empty vector if not found
    return {};
}

void Profiler::reset() {
    std::lock_guard<std::mutex> lock(mutex);
    
    // Reset metrics
    currentMetrics.reset();
    
    // Clear sections
    sections.clear();
    
    // Clear histories
    metricHistory.clear();
    
    // Clear memory usage
    memoryUsage.clear();
}

bool Profiler::saveToFile(const std::string& filepath) const {
    if (!enabled) return false;
    
    std::lock_guard<std::mutex> lock(mutex);
    
    try {
        // Create JSON object
        nlohmann::json json;
        
        // Add current metrics
        json["currentMetrics"]["frameTime"] = currentMetrics.frameTime;
        json["currentMetrics"]["updateTime"] = currentMetrics.updateTime;
        json["currentMetrics"]["physicsTime"] = currentMetrics.physicsTime;
        json["currentMetrics"]["renderTime"] = currentMetrics.renderTime;
        json["currentMetrics"]["fps"] = currentMetrics.fps;
        json["currentMetrics"]["renderedCells"] = currentMetrics.renderedCells;
        json["currentMetrics"]["updatedCells"] = currentMetrics.updatedCells;
        json["currentMetrics"]["activeChunks"] = currentMetrics.activeChunks;
        json["currentMetrics"]["memoryUsage"] = currentMetrics.memoryUsage;
        
        // Add metrics history
        for (const auto& history : metricHistory) {
            json["metricHistory"][history.first] = history.second;
        }
        
        // Add memory usage
        for (const auto& usage : memoryUsage) {
            json["memoryUsage"][usage.first] = usage.second;
        }
        
        // Write to file
        std::ofstream file(filepath);
        if (!file.is_open()) {
            return false;
        }
        
        file << json.dump(4); // Pretty print with 4-space indent
        file.close();
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error saving profiling data: " << e.what() << std::endl;
        return false;
    }
}

double Profiler::calculateFrameTime() {
    auto now = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(now - frameStartTime);
    return duration.count() / 1000000.0; // Convert to seconds
}

void Profiler::updateMemoryMetrics() {
    // Calculate total memory usage
    size_t total = 0;
    for (const auto& usage : memoryUsage) {
        total += usage.second;
    }
    
    currentMetrics.memoryUsage = total;
}

// ScopedTimer implementation
ScopedTimer::ScopedTimer(const std::string& name, Profiler* profiler)
    : name(name)
    , profiler(profiler ? profiler : &Profiler::getInstance())
{
    profiler->beginSection(name);
}

ScopedTimer::~ScopedTimer() {
    profiler->endSection(name);
}

} // namespace astral