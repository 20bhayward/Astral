#include "astral/core/Timer.h"

namespace astral {

Timer::Timer()
    : deltaTime(0.0)
    , totalTime(0.0)
{
    startTime = std::chrono::high_resolution_clock::now();
    lastUpdateTime = startTime;
}

void Timer::reset() 
{
    startTime = std::chrono::high_resolution_clock::now();
    lastUpdateTime = startTime;
    deltaTime = 0.0;
    totalTime = 0.0;
}

double Timer::update() 
{
    auto currentTime = std::chrono::high_resolution_clock::now();
    
    // Calculate elapsed time since last update
    std::chrono::duration<double> elapsedTime = currentTime - lastUpdateTime;
    deltaTime = elapsedTime.count();
    
    // Calculate total elapsed time since start
    std::chrono::duration<double> totalElapsedTime = currentTime - startTime;
    totalTime = totalElapsedTime.count();
    
    // Update last update time
    lastUpdateTime = currentTime;
    
    return deltaTime;
}

double Timer::getDeltaTime() const 
{
    return deltaTime;
}

double Timer::getTotalTime() const 
{
    return totalTime;
}

} // namespace astral