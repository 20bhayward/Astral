#include "astral/core/Config.h"
#include <gtest/gtest.h>
#include <fstream>
#include <filesystem>

namespace astral {
namespace test {

class ConfigTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a temporary test file
        testFilePath = "test_config.json";
        
        // Create test JSON data
        std::ofstream file(testFilePath);
        file << R"({
            "integer_value": 42,
            "float_value": 3.14,
            "string_value": "test string",
            "boolean_value": true
        })";
        file.close();
    }
    
    void TearDown() override {
        // Clean up test file
        if (std::filesystem::exists(testFilePath)) {
            std::filesystem::remove(testFilePath);
        }
    }
    
    std::string testFilePath;
};

TEST_F(ConfigTest, LoadFromFile) {
    Config config;
    
    // Test loading from file
    EXPECT_TRUE(config.loadFromFile(testFilePath));
    
    // Test loaded values
    EXPECT_EQ(42, config.get<int>("integer_value"));
    EXPECT_FLOAT_EQ(3.14f, config.get<float>("float_value"));
    EXPECT_EQ("test string", config.get<std::string>("string_value"));
    EXPECT_TRUE(config.get<bool>("boolean_value"));
}

TEST_F(ConfigTest, DefaultValues) {
    Config config;
    
    // Test default values for non-existent keys
    EXPECT_EQ(0, config.get<int>("non_existent_int", 0));
    EXPECT_FLOAT_EQ(1.0f, config.get<float>("non_existent_float", 1.0f));
    EXPECT_EQ("default", config.get<std::string>("non_existent_string", "default"));
    EXPECT_FALSE(config.get<bool>("non_existent_bool", false));
}

TEST_F(ConfigTest, HasKey) {
    Config config;
    
    // Load test config
    EXPECT_TRUE(config.loadFromFile(testFilePath));
    
    // Test has key
    EXPECT_TRUE(config.hasKey("integer_value"));
    EXPECT_TRUE(config.hasKey("float_value"));
    EXPECT_TRUE(config.hasKey("string_value"));
    EXPECT_TRUE(config.hasKey("boolean_value"));
    
    // Test does not have key
    EXPECT_FALSE(config.hasKey("non_existent_key"));
}

TEST_F(ConfigTest, SetAndGet) {
    Config config;
    
    // Set values
    config.set("int_value", 123);
    config.set("float_value", 2.5f);
    config.set("string_value", "hello");
    config.set("bool_value", true);
    
    // Get values
    EXPECT_EQ(123, config.get<int>("int_value"));
    EXPECT_FLOAT_EQ(2.5f, config.get<float>("float_value"));
    EXPECT_EQ("hello", config.get<std::string>("string_value"));
    EXPECT_TRUE(config.get<bool>("bool_value"));
}

TEST_F(ConfigTest, RemoveKey) {
    Config config;
    
    // Load test config
    EXPECT_TRUE(config.loadFromFile(testFilePath));
    
    // Remove key
    config.removeKey("integer_value");
    
    // Verify key is removed
    EXPECT_FALSE(config.hasKey("integer_value"));
    
    // Other keys should still exist
    EXPECT_TRUE(config.hasKey("float_value"));
}

TEST_F(ConfigTest, Clear) {
    Config config;
    
    // Load test config
    EXPECT_TRUE(config.loadFromFile(testFilePath));
    
    // Clear config
    config.clear();
    
    // Verify all keys are removed
    EXPECT_FALSE(config.hasKey("integer_value"));
    EXPECT_FALSE(config.hasKey("float_value"));
    EXPECT_FALSE(config.hasKey("string_value"));
    EXPECT_FALSE(config.hasKey("boolean_value"));
}

TEST_F(ConfigTest, SaveToFile) {
    Config config;
    
    // Set values
    config.set("int_value", 123);
    config.set("float_value", 2.5);
    config.set("string_value", "hello");
    config.set("bool_value", true);
    
    // Save to file
    std::string saveFilePath = "test_config_save.json";
    EXPECT_TRUE(config.saveToFile(saveFilePath));
    
    // Load from saved file
    Config loadedConfig;
    EXPECT_TRUE(loadedConfig.loadFromFile(saveFilePath));
    
    // Verify loaded values
    EXPECT_EQ(123, loadedConfig.get<int>("int_value"));
    EXPECT_DOUBLE_EQ(2.5, loadedConfig.get<double>("float_value"));
    EXPECT_EQ("hello", loadedConfig.get<std::string>("string_value"));
    EXPECT_TRUE(loadedConfig.get<bool>("bool_value"));
    
    // Clean up
    if (std::filesystem::exists(saveFilePath)) {
        std::filesystem::remove(saveFilePath);
    }
}

} // namespace test
} // namespace astral