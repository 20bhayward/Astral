#include "astral/core/Engine.h"
#include <iostream>

int main(int argc, char** argv) {
    try {
        astral::Engine engine;
        
        if (!engine.initialize()) {
            std::cerr << "Failed to initialize engine" << std::endl;
            return 1;
        }
        
        engine.run();
        engine.shutdown();
        
        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }
}