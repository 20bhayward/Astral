#pragma once

#include <memory>
#include <string>

namespace spdlog {
    class logger;
}

namespace astral {

/**
 * Logger class for handling application logging.
 * Uses spdlog as the underlying logging library.
 */
class Logger 
{
public:
    /**
     * Create a new logger with the given name.
     * 
     * @param name The name of the logger.
     */
    explicit Logger(const std::string& name);
    ~Logger();
    
    /**
     * Initialize the logger.
     * 
     * @param logFilePath Optional path to log file. If empty, logs only to console.
     * @return True if initialization succeeded, false otherwise.
     */
    bool initialize(const std::string& logFilePath = "");
    
    /**
     * Log a debug message.
     * 
     * @param message The message to log.
     */
    void debug(const std::string& message);
    
    /**
     * Log an info message.
     * 
     * @param message The message to log.
     */
    void info(const std::string& message);
    
    /**
     * Log a warning message.
     * 
     * @param message The message to log.
     */
    void warn(const std::string& message);
    
    /**
     * Log an error message.
     * 
     * @param message The message to log.
     */
    void error(const std::string& message);
    
    /**
     * Log a critical message.
     * 
     * @param message The message to log.
     */
    void critical(const std::string& message);

private:
    std::string name;
    std::shared_ptr<spdlog::logger> logger;
};

} // namespace astral