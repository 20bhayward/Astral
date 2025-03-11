#include "astral/core/Timer.h"
#include <gtest/gtest.h>
#include <thread>
#include <chrono>

namespace astral {
namespace test {

TEST(TimerTest, Constructor) {
    Timer timer;
    
    // Initial values should be zero
    EXPECT_DOUBLE_EQ(0.0, timer.getDeltaTime());
    EXPECT_DOUBLE_EQ(0.0, timer.getTotalTime());
}

TEST(TimerTest, Reset) {
    Timer timer;
    
    // Let some time pass
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    timer.update();
    
    // Verify time has elapsed
    EXPECT_GT(timer.getDeltaTime(), 0.0);
    EXPECT_GT(timer.getTotalTime(), 0.0);
    
    // Reset timer
    timer.reset();
    
    // Values should be zero again
    EXPECT_DOUBLE_EQ(0.0, timer.getDeltaTime());
    EXPECT_DOUBLE_EQ(0.0, timer.getTotalTime());
}

TEST(TimerTest, Update) {
    Timer timer;
    
    // First update - delta time should be close to the time since construction
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    double delta1 = timer.update();
    
    // Verify delta time is positive
    EXPECT_GT(delta1, 0.0);
    
    // Delta time from update should match the stored delta time
    EXPECT_DOUBLE_EQ(delta1, timer.getDeltaTime());
    
    // Total time should equal delta time after first update
    EXPECT_DOUBLE_EQ(delta1, timer.getTotalTime());
    
    // Second update - new delta time
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    double delta2 = timer.update();
    
    // Verify new delta time is positive
    EXPECT_GT(delta2, 0.0);
    
    // Total time should be sum of delta times
    EXPECT_NEAR(delta1 + delta2, timer.getTotalTime(), 0.001);
}

// Test timing accuracy
TEST(TimerTest, Accuracy) {
    Timer timer;
    
    // Sleep for 100ms
    const double sleepTime = 0.1; // 100ms
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    double elapsed = timer.update();
    
    // Verify elapsed time is close to sleep time
    // Allow 20% tolerance for scheduling variations
    const double tolerance = 0.02; // 20ms
    EXPECT_NEAR(sleepTime, elapsed, tolerance);
}

} // namespace test
} // namespace astral