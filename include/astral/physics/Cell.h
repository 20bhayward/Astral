#pragma once

#include <cstdint>
#include <glm/glm.hpp>

namespace astral {

using MaterialID = uint16_t;

struct Cell {
    MaterialID material;
    float temperature;
    glm::vec2 velocity;
    uint8_t metadata;
    
    Cell();
    Cell(MaterialID material);
    
    bool operator==(const Cell& other) const;
    bool operator!=(const Cell& other) const;
};

} // namespace astral