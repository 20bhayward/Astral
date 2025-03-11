#pragma once

#include <chrono>

namespace astral {

/**
 * Timer class for tracking time and calculating delta time between frames.
 */
class Timer 
{
public:
    Timer();
    ~Timer() = default;
    
    /**
     * Reset the timer to the current time.
     */
    void reset();
    
    /**
     * Update the timer, calculating new delta time.
     * @return The time elapsed since the last update in seconds.
     */
    double update();
    
    /**
     * Get the time elapsed since last update in seconds.
     * @return Delta time in seconds.
     */
    double getDeltaTime() const;
    
    /**
     * Get the total time elapsed since creation or last reset in seconds.
     * @return Total time in seconds.
     */
    double getTotalTime() const;
    
private:
    std::chrono::time_point<std::chrono::high_resolution_clock> startTime;
    std::chrono::time_point<std::chrono::high_resolution_clock> lastUpdateTime;
    double deltaTime;
    double totalTime;
};

} // namespace astral