#include "astral/core/Engine.h"
#include <iostream>

// Check for optional components
#ifdef ASTRAL_HAS_RENDERING
#include "astral/rendering/Renderer.h"
#endif

#ifdef ASTRAL_HAS_TOOLS
#include "astral/tools/DebugUI.h"
#endif

int main(int argc, char** argv) {
    try {
        // Print build configuration
        std::cout << "Astral Engine starting up with components:" << std::endl;
        std::cout << "- Core: YES" << std::endl;
        std::cout << "- Physics: YES" << std::endl;
        
#ifdef ASTRAL_HAS_RENDERING
        std::cout << "- Rendering: YES" << std::endl;
#else
        std::cout << "- Rendering: NO (missing dependencies)" << std::endl;
#endif

#ifdef ASTRAL_HAS_TOOLS
        std::cout << "- Tools: YES" << std::endl;
#else
        std::cout << "- Tools: NO (missing dependencies)" << std::endl;
#endif

        // Initialize engine
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