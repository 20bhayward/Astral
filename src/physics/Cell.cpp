#include "astral/physics/Cell.h"

namespace astral {

Cell::Cell()
    : material(0)  // 0 is typically EMPTY
    , temperature(0.0f)
    , velocity(0.0f, 0.0f)
    , metadata(0)
{
}

Cell::Cell(MaterialID material)
    : material(material)
    , temperature(0.0f)
    , velocity(0.0f, 0.0f)
    , metadata(0)
{
}

bool Cell::operator==(const Cell& other) const
{
    return material == other.material &&
           temperature == other.temperature &&
           velocity == other.velocity &&
           metadata == other.metadata;
}

bool Cell::operator!=(const Cell& other) const
{
    return !(*this == other);
}

} // namespace astral