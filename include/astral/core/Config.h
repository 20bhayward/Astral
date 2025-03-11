#pragma once

#include <string>
#include <unordered_map>
#include <variant>
#include <nlohmann/json.hpp>

namespace astral {

/**
 * Configuration management system for loading, saving, and accessing
 * engine and game settings.
 */
class Config
{
public:
    Config();
    ~Config();
    
    /**
     * Load configuration from a JSON file.
     * 
     * @param filepath Path to the JSON configuration file.
     * @return True if the configuration was loaded successfully, false otherwise.
     */
    bool loadFromFile(const std::string& filepath);
    
    /**
     * Save the current configuration to a JSON file.
     * 
     * @param filepath Path where to save the configuration file. If empty,
     *                 uses the same file as loaded from.
     * @return True if the configuration was saved successfully, false otherwise.
     */
    bool saveToFile(const std::string& filepath = "");
    
    /**
     * Get a configuration value by key.
     * 
     * @tparam T Type of the value to retrieve.
     * @param key The configuration key.
     * @param defaultValue Default value to return if the key is not found.
     * @return The configuration value, or defaultValue if not found.
     */
    template<typename T>
    T get(const std::string& key, const T& defaultValue = T()) const;
    
    /**
     * Set a configuration value.
     * 
     * @tparam T Type of the value to set.
     * @param key The configuration key.
     * @param value The value to set.
     */
    template<typename T>
    void set(const std::string& key, const T& value);
    
    /**
     * Check if a configuration key exists.
     * 
     * @param key The key to check.
     * @return True if the key exists in the configuration, false otherwise.
     */
    bool hasKey(const std::string& key) const;
    
    /**
     * Remove a configuration key and its value.
     * 
     * @param key The key to remove.
     */
    void removeKey(const std::string& key);
    
    /**
     * Clear all configuration values.
     */
    void clear();
    
private:
    using ConfigValue = std::variant<int, float, double, bool, std::string>;
    std::unordered_map<std::string, ConfigValue> values;
    std::string configFilePath;
    
    // Helper methods for variant access
    template<typename T>
    T getValue(const ConfigValue& var, const T& defaultValue) const;
};

// Template specializations for getValue
template<>
inline int Config::getValue(const ConfigValue& var, const int& defaultValue) const
{
    if (std::holds_alternative<int>(var)) return std::get<int>(var);
    if (std::holds_alternative<float>(var)) return static_cast<int>(std::get<float>(var));
    if (std::holds_alternative<double>(var)) return static_cast<int>(std::get<double>(var));
    return defaultValue;
}

template<>
inline float Config::getValue(const ConfigValue& var, const float& defaultValue) const
{
    if (std::holds_alternative<float>(var)) return std::get<float>(var);
    if (std::holds_alternative<int>(var)) return static_cast<float>(std::get<int>(var));
    if (std::holds_alternative<double>(var)) return static_cast<float>(std::get<double>(var));
    return defaultValue;
}

template<>
inline double Config::getValue(const ConfigValue& var, const double& defaultValue) const
{
    if (std::holds_alternative<double>(var)) return std::get<double>(var);
    if (std::holds_alternative<int>(var)) return static_cast<double>(std::get<int>(var));
    if (std::holds_alternative<float>(var)) return static_cast<double>(std::get<float>(var));
    return defaultValue;
}

template<>
inline bool Config::getValue(const ConfigValue& var, const bool& defaultValue) const
{
    if (std::holds_alternative<bool>(var)) return std::get<bool>(var);
    return defaultValue;
}

template<>
inline std::string Config::getValue(const ConfigValue& var, const std::string& defaultValue) const
{
    if (std::holds_alternative<std::string>(var)) return std::get<std::string>(var);
    return defaultValue;
}

// Template implementation for get
template<typename T>
T Config::get(const std::string& key, const T& defaultValue) const
{
    auto it = values.find(key);
    if (it != values.end())
    {
        return getValue<T>(it->second, defaultValue);
    }
    return defaultValue;
}

// Template implementation for set
template<typename T>
void Config::set(const std::string& key, const T& value)
{
    values[key] = value;
}

} // namespace astral