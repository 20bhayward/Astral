#include "astral/physics/Cell.h"
#include <gtest/gtest.h>

namespace astral {
namespace test {

TEST(CellTest, DefaultConstructor) {
    Cell cell;
    
    // Default constructor should initialize to zeros
    EXPECT_EQ(0, cell.material);
    EXPECT_FLOAT_EQ(0.0f, cell.temperature);
    EXPECT_EQ(glm::vec2(0.0f, 0.0f), cell.velocity);
    EXPECT_EQ(0, cell.metadata);
}

TEST(CellTest, MaterialConstructor) {
    const MaterialID testMaterial = 42;
    Cell cell(testMaterial);
    
    // Material constructor should set material and default other values
    EXPECT_EQ(testMaterial, cell.material);
    EXPECT_FLOAT_EQ(0.0f, cell.temperature);
    EXPECT_EQ(glm::vec2(0.0f, 0.0f), cell.velocity);
    EXPECT_EQ(0, cell.metadata);
}

TEST(CellTest, EqualityOperator) {
    // Create two identical cells
    Cell cell1;
    cell1.material = 1;
    cell1.temperature = 100.0f;
    cell1.velocity = glm::vec2(1.0f, 2.0f);
    cell1.metadata = 5;
    
    Cell cell2;
    cell2.material = 1;
    cell2.temperature = 100.0f;
    cell2.velocity = glm::vec2(1.0f, 2.0f);
    cell2.metadata = 5;
    
    // Identical cells should be equal
    EXPECT_TRUE(cell1 == cell2);
    
    // Change one property
    cell2.material = 2;
    EXPECT_FALSE(cell1 == cell2);
    
    // Reset and change another property
    cell2.material = 1;
    cell2.temperature = 200.0f;
    EXPECT_FALSE(cell1 == cell2);
    
    // Reset and change another property
    cell2.temperature = 100.0f;
    cell2.velocity = glm::vec2(3.0f, 4.0f);
    EXPECT_FALSE(cell1 == cell2);
    
    // Reset and change another property
    cell2.velocity = glm::vec2(1.0f, 2.0f);
    cell2.metadata = 10;
    EXPECT_FALSE(cell1 == cell2);
}

TEST(CellTest, InequalityOperator) {
    // Create two identical cells
    Cell cell1;
    cell1.material = 1;
    cell1.temperature = 100.0f;
    cell1.velocity = glm::vec2(1.0f, 2.0f);
    cell1.metadata = 5;
    
    Cell cell2;
    cell2.material = 1;
    cell2.temperature = 100.0f;
    cell2.velocity = glm::vec2(1.0f, 2.0f);
    cell2.metadata = 5;
    
    // Identical cells should not be unequal
    EXPECT_FALSE(cell1 != cell2);
    
    // Change one property
    cell2.material = 2;
    EXPECT_TRUE(cell1 != cell2);
}

} // namespace test
} // namespace astral