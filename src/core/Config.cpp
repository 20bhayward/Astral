#include "astral/core/Config.h"
#include <fstream>
#include <iostream>

namespace astral {

Config::Config()
{
}

Config::~Config()
{
}

bool Config::loadFromFile(const std::string& filepath)
{
    try
    {
        std::ifstream file(filepath);
        if (!file.is_open())
        {
            std::cerr << "Failed to open config file: " << filepath << std::endl;
            return false;
        }
        
        // Save the file path for later use in saveToFile
        configFilePath = filepath;
        
        // Parse JSON
        nlohmann::json json;
        file >> json;
        file.close();
        
        // Clear existing values
        values.clear();
        
        // Process all key-value pairs
        for (auto it = json.begin(); it != json.end(); ++it)
        {
            const std::string& key = it.key();
            
            if (it->is_boolean())
            {
                values[key] = it->get<bool>();
            }
            else if (it->is_number_integer())
            {
                values[key] = it->get<int>();
            }
            else if (it->is_number_float())
            {
                values[key] = it->get<double>();
            }
            else if (it->is_string())
            {
                values[key] = it->get<std::string>();
            }
            // Ignore arrays and objects for now
        }
        
        return true;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error loading config file: " << e.what() << std::endl;
        return false;
    }
}

bool Config::saveToFile(const std::string& filepath)
{
    try
    {
        // Use provided filepath or previously loaded file
        std::string saveFilepath = filepath.empty() ? configFilePath : filepath;
        
        if (saveFilepath.empty())
        {
            std::cerr << "No filepath provided for config save" << std::endl;
            return false;
        }
        
        // Create JSON object
        nlohmann::json json;
        
        // Add all values to JSON
        for (const auto& [key, value] : values)
        {
            if (std::holds_alternative<int>(value))
            {
                json[key] = std::get<int>(value);
            }
            else if (std::holds_alternative<float>(value))
            {
                json[key] = std::get<float>(value);
            }
            else if (std::holds_alternative<double>(value))
            {
                json[key] = std::get<double>(value);
            }
            else if (std::holds_alternative<bool>(value))
            {
                json[key] = std::get<bool>(value);
            }
            else if (std::holds_alternative<std::string>(value))
            {
                json[key] = std::get<std::string>(value);
            }
        }
        
        // Write to file
        std::ofstream file(saveFilepath);
        if (!file.is_open())
        {
            std::cerr << "Failed to open config file for writing: " << saveFilepath << std::endl;
            return false;
        }
        
        file << json.dump(4); // Pretty print with 4-space indent
        file.close();
        
        return true;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error saving config file: " << e.what() << std::endl;
        return false;
    }
}

bool Config::hasKey(const std::string& key) const
{
    return values.find(key) != values.end();
}

void Config::removeKey(const std::string& key)
{
    values.erase(key);
}

void Config::clear()
{
    values.clear();
}

} // namespace astral