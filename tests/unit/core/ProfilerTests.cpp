#include "astral/core/Profiler.h"
#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include <filesystem>

namespace astral {
namespace test {

class ProfilerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize profiler
        Profiler::getInstance().initialize(true);
        Profiler::getInstance().reset();
    }
    
    void TearDown() override {
        // Reset profiler after each test
        Profiler::getInstance().reset();
        
        // Clean up test file if it exists
        if (std::filesystem::exists("test_profile.json")) {
            std::filesystem::remove("test_profile.json");
        }
    }
};

TEST_F(ProfilerTest, SingletonInstance) {
    // Test that we always get the same instance
    Profiler& instance1 = Profiler::getInstance();
    Profiler& instance2 = Profiler::getInstance();
    
    EXPECT_EQ(&instance1, &instance2);
}

TEST_F(ProfilerTest, EnableDisable) {
    Profiler& profiler = Profiler::getInstance();
    
    // Test initial state
    EXPECT_TRUE(profiler.isEnabled());
    
    // Disable
    profiler.setEnabled(false);
    EXPECT_FALSE(profiler.isEnabled());
    
    // Re-enable
    profiler.setEnabled(true);
    EXPECT_TRUE(profiler.isEnabled());
}

TEST_F(ProfilerTest, BeginEndFrame) {
    Profiler& profiler = Profiler::getInstance();
    
    // Begin a frame
    profiler.beginFrame();
    
    // Wait a bit
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    
    // End the frame
    profiler.endFrame();
    
    // Get metrics
    const PerformanceMetrics& metrics = profiler.getMetrics();
    
    // Check frame time is positive
    EXPECT_GT(metrics.frameTime, 0.0);
    
    // Check FPS is calculated
    EXPECT_GT(metrics.fps, 0.0);
}

TEST_F(ProfilerTest, BeginEndSection) {
    Profiler& profiler = Profiler::getInstance();
    
    // Begin a frame
    profiler.beginFrame();
    
    // Begin a section
    profiler.beginSection("TestSection");
    
    // Wait a bit
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    
    // End the section
    profiler.endSection("TestSection");
    
    // End the frame
    profiler.endFrame();
    
    // Try to get section timing from history
    auto history = profiler.getMetricHistory("TestSection");
    
    // We don't store section timing in history by default, so this should be empty
    EXPECT_TRUE(history.empty());
}

TEST_F(ProfilerTest, ScopedTimer) {
    Profiler& profiler = Profiler::getInstance();
    
    // Begin a frame
    profiler.beginFrame();
    
    {
        // Create a scoped timer
        ScopedTimer timer("ScopedTest");
        
        // Wait a bit
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    } // Timer automatically ends here
    
    // End the frame
    profiler.endFrame();
    
    // Section timing isn't stored in history by default
    auto history = profiler.getMetricHistory("ScopedTest");
    EXPECT_TRUE(history.empty());
}

TEST_F(ProfilerTest, RecordValue) {
    Profiler& profiler = Profiler::getInstance();
    
    // Begin a frame
    profiler.beginFrame();
    
    // Record a value
    profiler.recordValue("TestMetric", 42.0);
    
    // End the frame
    profiler.endFrame();
    
    // Check the recorded value is in history
    auto history = profiler.getMetricHistory("TestMetric");
    EXPECT_FALSE(history.empty());
    EXPECT_DOUBLE_EQ(42.0, history.back());
}

TEST_F(ProfilerTest, RecordMemoryUsage) {
    Profiler& profiler = Profiler::getInstance();
    
    // Begin a frame
    profiler.beginFrame();
    
    // Record memory usage
    profiler.recordMemoryUsage("TestSubsystem", 1024);
    
    // End the frame
    profiler.endFrame();
    
    // Check the recorded memory is in metrics
    const PerformanceMetrics& metrics = profiler.getMetrics();
    EXPECT_EQ(1024, metrics.memoryUsage);
}

TEST_F(ProfilerTest, ProfileMacros) {
    Profiler& profiler = Profiler::getInstance();
    
    // Begin a frame
    profiler.beginFrame();
    
    // Use the PROFILE_SCOPE macro
    {
        PROFILE_SCOPE("MacroTest");
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    // End the frame
    profiler.endFrame();
    
    // Section timing isn't stored in history by default
    auto history = profiler.getMetricHistory("MacroTest");
    EXPECT_TRUE(history.empty());
}

TEST_F(ProfilerTest, SaveToFile) {
    Profiler& profiler = Profiler::getInstance();
    
    // Begin a frame
    profiler.beginFrame();
    
    // Record some data
    profiler.recordValue("TestMetric", 42.0);
    profiler.recordMemoryUsage("TestSubsystem", 1024);
    
    // End the frame
    profiler.endFrame();
    
    // Save to file
    EXPECT_TRUE(profiler.saveToFile("test_profile.json"));
    
    // Check file exists
    EXPECT_TRUE(std::filesystem::exists("test_profile.json"));
    
    // Check file is not empty
    EXPECT_GT(std::filesystem::file_size("test_profile.json"), 0);
}

} // namespace test
} // namespace astral