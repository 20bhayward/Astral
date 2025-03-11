#include "astral/physics/Material.h"
#include <gtest/gtest.h>

namespace astral {
namespace test {

TEST(MaterialTest, DefaultConstructor) {
    MaterialProperties props;
    EXPECT_EQ(props.type, MaterialType::EMPTY);
    EXPECT_EQ(props.name, "");
    EXPECT_FALSE(props.movable);
}

TEST(MaterialTest, ParameterizedConstructor) {
    MaterialProperties props(MaterialType::SOLID, "Stone", glm::vec4(0.5f, 0.5f, 0.5f, 1.0f));
    EXPECT_EQ(props.type, MaterialType::SOLID);
    EXPECT_EQ(props.name, "Stone");
    EXPECT_EQ(props.color, glm::vec4(0.5f, 0.5f, 0.5f, 1.0f));
}

} // namespace test
} // namespace astral