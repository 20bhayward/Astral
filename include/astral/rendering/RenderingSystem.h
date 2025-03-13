#pragma once

namespace astral {

// Forward declaration to fix build issues
class RenderingSystem {
public:
    RenderingSystem() = default;
    virtual ~RenderingSystem() = default;
    
    virtual void render() {}
};

} // namespace astral