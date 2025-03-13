#pragma once

#include <cstdint>
#include <glm/glm.hpp>

namespace astral {

using MaterialID = uint16_t;

struct Cell {
    // Flags for cell state
    enum CellFlags {
        FLAG_NONE       = 0,
        FLAG_BURNING    = 1 << 0,
        FLAG_FROZEN     = 1 << 1,
        FLAG_PRESSURIZED = 1 << 2,
        FLAG_CHARGED    = 1 << 3,
        FLAG_DISSOLVING = 1 << 4
    };

    MaterialID material;
    float temperature;
    glm::vec2 velocity;
    uint8_t metadata;
    float pressure;   // Added for liquid/gas simulation
    bool updated;     // Flag to track if cell was updated this frame
    
    // Expanded properties for enhanced simulation
    float health;     // Structural integrity (0-1)
    uint8_t lifetime; // Remaining lifetime for temporary cells
    float energy;     // Generic energy value for various effects
    float charge;     // Electrical charge
    uint8_t stateFlags; // Bitfield for various state flags
    
    Cell() 
        : material(0)
        , temperature(20.0f)
        , velocity(0.0f, 0.0f)
        , metadata(0)
        , pressure(0.0f)
        , updated(false)
        , health(1.0f)
        , lifetime(0)
        , energy(0.0f)
        , charge(0.0f)
        , stateFlags(0) 
    {}
    
    Cell(MaterialID mat) 
        : material(mat)
        , temperature(20.0f)
        , velocity(0.0f, 0.0f)
        , metadata(0)
        , pressure(0.0f)
        , updated(false)
        , health(1.0f)
        , lifetime(0)
        , energy(0.0f)
        , charge(0.0f)
        , stateFlags(0)
    {}
    
    // Flag operations
    inline bool hasFlag(CellFlags flag) const { return (stateFlags & flag) != 0; }
    inline void setFlag(CellFlags flag) { stateFlags |= static_cast<uint8_t>(flag); }
    inline void clearFlag(CellFlags flag) { stateFlags &= ~static_cast<uint8_t>(flag); }
    inline void toggleFlag(CellFlags flag) { stateFlags ^= static_cast<uint8_t>(flag); }
    
    bool operator==(const Cell& other) const {
        return material == other.material &&
               temperature == other.temperature &&
               velocity == other.velocity &&
               metadata == other.metadata &&
               pressure == other.pressure;
    }
    
    bool operator!=(const Cell& other) const {
        return !(*this == other);
    }
};

} // namespace astral