#pragma once

#include <memory>
#include <string>

namespace astral {

class Logger;
class Config;
class Timer;
class PhysicsSystem;
class RenderingSystem;

class Engine {
public:
    Engine();
    ~Engine();

    bool initialize(const std::string& configFile = "config.json");
    void shutdown();

    void run();
    void stop();

    double getDeltaTime() const;
    double getTime() const;

private:
    bool running;
    std::unique_ptr<Logger> logger;
    std::unique_ptr<Config> config;
    std::unique_ptr<Timer> timer;
    std::unique_ptr<PhysicsSystem> physics;
    std::unique_ptr<RenderingSystem> renderer;

    double deltaTime;
    double time;

    void update();
    void render();
};

} // namespace astral