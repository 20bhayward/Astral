#include <gtest/gtest.h>
#include <memory>
#include "astral/physics/CellularAutomaton.h"

using namespace astral;

class CellularAutomatonTest : public ::testing::Test {
protected:
    std::unique_ptr<CellularAutomaton> automaton;
    const int testWorldWidth = 200;
    const int testWorldHeight = 200;

    void SetUp() override {
        automaton = std::make_unique<CellularAutomaton>(testWorldWidth, testWorldHeight);
    }

    void TearDown() override {
        automaton.reset();
    }
};

TEST_F(CellularAutomatonTest, Initialization) {
    // Check that the world is initialized with the correct dimensions
    EXPECT_EQ(automaton->getWorldWidth(), testWorldWidth);
    EXPECT_EQ(automaton->getWorldHeight(), testWorldHeight);
    
    // Check that the simulation starts unpaused
    EXPECT_FALSE(automaton->isSimulationPaused());
    
    // Check that the time scale starts at 1.0
    EXPECT_FLOAT_EQ(automaton->getTimeScale(), 1.0f);
}

TEST_F(CellularAutomatonTest, PauseResume) {
    // Check initial state
    EXPECT_FALSE(automaton->isSimulationPaused());
    
    // Pause the simulation
    automaton->pause();
    EXPECT_TRUE(automaton->isSimulationPaused());
    
    // Resume the simulation
    automaton->resume();
    EXPECT_FALSE(automaton->isSimulationPaused());
}

TEST_F(CellularAutomatonTest, TimeScale) {
    // Check initial time scale
    EXPECT_FLOAT_EQ(automaton->getTimeScale(), 1.0f);
    
    // Set a new time scale
    automaton->setTimeScale(2.5f);
    EXPECT_FLOAT_EQ(automaton->getTimeScale(), 2.5f);
    
    // Set another time scale
    automaton->setTimeScale(0.5f);
    EXPECT_FLOAT_EQ(automaton->getTimeScale(), 0.5f);
}

TEST_F(CellularAutomatonTest, CellAccess) {
    // Get material IDs
    MaterialID airId = automaton->getMaterialIDByName("Air");
    MaterialID stoneId = automaton->getMaterialIDByName("Stone");
    
    // Get cell before setting to verify we can get cells
    Cell defaultCell = automaton->getCell(10, 10);
    
    // Set a cell to stone
    automaton->setCell(10, 10, stoneId);
    
    // Get the cell back and check its material
    Cell modifiedCell = automaton->getCell(10, 10);
    EXPECT_EQ(modifiedCell.material, stoneId);
}

TEST_F(CellularAutomatonTest, MaterialManagement) {
    // Register a new material
    MaterialProperties goldProps;
    goldProps.name = "Gold";
    goldProps.type = MaterialType::SOLID;
    goldProps.color = {255, 215, 0, 255};
    goldProps.density = 19300.0f;
    goldProps.thermalConductivity = 0.8f;
    goldProps.specificHeat = 0.129f;
    goldProps.movable = false;
    
    MaterialID goldId = automaton->registerMaterial(goldProps);
    EXPECT_NE(goldId, MaterialID(0));
    
    // Get the material back
    MaterialID goldIdByName = automaton->getMaterialIDByName("Gold");
    EXPECT_EQ(goldId, goldIdByName);
    
    // Get the properties and check they match
    MaterialProperties retrievedProps = automaton->getMaterial(goldId);
    EXPECT_EQ(retrievedProps.name, "Gold");
    EXPECT_EQ(retrievedProps.type, MaterialType::SOLID);
    EXPECT_FLOAT_EQ(retrievedProps.density, 19300.0f);
}

TEST_F(CellularAutomatonTest, PaintingTools) {
    // Get materials
    MaterialID stoneId = automaton->getMaterialIDByName("Stone");
    MaterialID sandId = automaton->getMaterialIDByName("Sand");
    
    // Paint a single cell
    automaton->paintCell(50, 50, stoneId);
    EXPECT_EQ(automaton->getCell(50, 50).material, stoneId);
    
    // Paint a line
    automaton->paintLine(60, 60, 70, 70, sandId);
    EXPECT_EQ(automaton->getCell(60, 60).material, sandId);
    EXPECT_EQ(automaton->getCell(65, 65).material, sandId);
    EXPECT_EQ(automaton->getCell(70, 70).material, sandId);
    
    // Paint a circle
    automaton->paintCircle(100, 100, 5, stoneId);
    EXPECT_EQ(automaton->getCell(100, 100).material, stoneId);
    EXPECT_EQ(automaton->getCell(103, 103).material, stoneId);
    EXPECT_EQ(automaton->getCell(100, 105).material, stoneId);
    
    // Paint a rectangle
    automaton->fillRectangle(120, 120, 10, 10, sandId);
    EXPECT_EQ(automaton->getCell(120, 120).material, sandId);
    EXPECT_EQ(automaton->getCell(125, 125).material, sandId);
    EXPECT_EQ(automaton->getCell(129, 129).material, sandId);
}

TEST_F(CellularAutomatonTest, WorldGeneration) {
    // Test that world templates can be generated without error
    
    // EMPTY template - should be all empty cells
    automaton->generateWorld(WorldTemplate::EMPTY);
    
    // Default cells should have material 0
    EXPECT_EQ(automaton->getCell(50, 50).material, 0u);
    
    // Generate the other templates without assertions
    automaton->generateWorld(WorldTemplate::FLAT_TERRAIN);
    automaton->generateWorld(WorldTemplate::TERRAIN_WITH_CAVES);
    automaton->generateWorld(WorldTemplate::TERRAIN_WITH_WATER);
    automaton->generateWorld(WorldTemplate::RANDOM_MATERIALS);
    automaton->generateWorld(WorldTemplate::SANDBOX);
}

TEST_F(CellularAutomatonTest, SimulationUpdate) {
    // Register a new material
    MaterialProperties powderProps;
    powderProps.name = "TestPowder";
    powderProps.type = MaterialType::POWDER;
    powderProps.color = {200, 200, 100, 255};
    powderProps.density = 1500.0f;
    powderProps.movable = true;
    MaterialID powderMaterialId = automaton->registerMaterial(powderProps);
    
    // Place the cell at the top
    automaton->paintCell(100, 50, powderMaterialId);
    
    // Initial position
    const MaterialProperties& initialCellProps = automaton->getMaterial(
        automaton->getCell(100, 50).material);
    EXPECT_EQ(initialCellProps.type, MaterialType::POWDER);
    
    // Run a few simulation steps - just testing that update runs without crashing
    for (int i = 0; i < 10; i++) {
        automaton->update(0.016f); // ~60 FPS
    }
}

TEST_F(CellularAutomatonTest, SpecialEffects) {
    // Test that we can call special effects methods without errors
    
    // Apply an explosion effect
    automaton->createExplosion(100, 100, 10.0f, 5.0f);
    
    // Run a few simulation steps
    for (int i = 0; i < 5; i++) {
        automaton->update(0.016f);
    }
    
    // Apply heat source effect
    automaton->createHeatSource(100, 100, 200.0f, 10.0f);
    
    // Run simulation steps
    for (int i = 0; i < 5; i++) {
        automaton->update(0.016f);
    }
    
    // Apply force field
    automaton->applyForce(100, 100, glm::vec2(1.0f, 0.0f), 5.0f, 10.0f);
    
    // Run simulation steps
    for (int i = 0; i < 5; i++) {
        automaton->update(0.016f);
    }
    
    // No assertions - just checking that the methods run without errors
}

TEST_F(CellularAutomatonTest, ActiveArea) {
    // Set a smaller active area
    int areaX = 50;
    int areaY = 50;
    int areaWidth = 100;
    int areaHeight = 100;
    automaton->setActiveArea(areaX, areaY, areaWidth, areaHeight);
    
    // Simply verify that setting the active area doesn't cause errors
    // Run a few simulation steps
    for (int i = 0; i < 10; i++) {
        automaton->update(0.016f);
    }
    
    // No assertions - just checking the method runs without errors
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}