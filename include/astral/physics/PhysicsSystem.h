#pragma once

namespace astral {

// Forward declaration to fix build issues
class PhysicsSystem {
public:
    PhysicsSystem() = default;
    virtual ~PhysicsSystem() = default;
    
    virtual void update(double deltaTime) {}
};

} // namespace astral