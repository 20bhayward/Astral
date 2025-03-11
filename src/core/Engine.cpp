#include "astral/core/Engine.h"
#include "astral/core/Logger.h"
#include "astral/core/Config.h"
#include "astral/core/Timer.h"
#include <iostream>
#include <thread>
#include <chrono>

namespace astral {

Engine::Engine()
    : running(false)
    , deltaTime(0.0)
    , time(0.0)
    , logger(nullptr)
    , config(nullptr)
    , timer(nullptr)
    , physics(nullptr)
    , renderer(nullptr)
{
}

Engine::~Engine()
{
    shutdown();
}

bool Engine::initialize(const std::string& configFile)
{
    // Create logger first for error reporting
    logger = std::make_unique<Logger>("AstralEngine");
    if (!logger->initialize("astral_log.txt"))
    {
        std::cerr << "Failed to initialize logger" << std::endl;
        return false;
    }
    
    logger->info("Initializing Astral Engine...");
    
    // Load configuration
    config = std::make_unique<Config>();
    if (!config->loadFromFile(configFile))
    {
        logger->warn("Failed to load config file: " + configFile + ", using defaults");
        
        // Set default configuration values
        config->set("window_width", 1280);
        config->set("window_height", 720);
        config->set("window_title", "Astral Engine");
        config->set("vsync", true);
        config->set("fullscreen", false);
        config->set("target_fps", 60);
        
        // Save default config for next time
        config->saveToFile(configFile);
    }
    
    // Initialize timer
    timer = std::make_unique<Timer>();
    timer->reset();
    
    // TODO: Initialize physics system
    
    // TODO: Initialize rendering system
    
    logger->info("Engine initialized successfully");
    return true;
}

void Engine::shutdown()
{
    if (running)
    {
        stop();
    }
    
    // Clean up in reverse order of initialization
    logger->info("Shutting down Astral Engine...");
    
    // Clean up systems
    renderer.reset();
    physics.reset();
    timer.reset();
    config.reset();
    logger.reset();
}

void Engine::run()
{
    if (!logger)
    {
        std::cerr << "Cannot run engine: not initialized" << std::endl;
        return;
    }
    
    logger->info("Engine running...");
    running = true;
    
    // Get target FPS from config
    int targetFPS = config->get<int>("target_fps", 60);
    double targetFrameTime = 1.0 / targetFPS;
    
    // Main loop
    while (running)
    {
        // Update time using the timer
        deltaTime = timer->update();
        time = timer->getTotalTime();
        
        // Update and render
        update();
        render();
        
        // Cap frame rate if needed
        double frameTime = timer->getDeltaTime();
        if (frameTime < targetFrameTime)
        {
            double sleepTime = (targetFrameTime - frameTime) * 1000.0;
            std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(sleepTime)));
        }
    }
    
    logger->info("Engine stopped");
}

void Engine::stop()
{
    running = false;
}

double Engine::getDeltaTime() const
{
    return deltaTime;
}

double Engine::getTime() const
{
    return time;
}

void Engine::update()
{
    // Skip if not initialized
    if (!timer || !logger)
    {
        return;
    }
    
    // Update physics system if available
    if (physics)
    {
        // physics->update(deltaTime);
    }
}

void Engine::render()
{
    // Skip if not initialized
    if (!timer || !logger)
    {
        return;
    }
    
    // Render using the rendering system if available
    if (renderer)
    {
        // renderer->render();
    }
}

} // namespace astral