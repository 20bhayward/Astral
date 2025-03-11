#include "astral/core/Logger.h"
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <iostream>
#include <vector>

namespace astral {

Logger::Logger(const std::string& name)
    : name(name), logger(nullptr)
{
}

Logger::~Logger()
{
    // spdlog will handle cleanup
}

bool Logger::initialize(const std::string& logFilePath)
{
    try
    {
        std::vector<spdlog::sink_ptr> sinks;
        
        // Always add console sink
        auto consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        consoleSink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [%n] %v");
        sinks.push_back(consoleSink);
        
        // Add file sink if requested
        if (!logFilePath.empty())
        {
            auto fileSink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(logFilePath, true);
            fileSink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] [%n] %v");
            sinks.push_back(fileSink);
        }
        
        // Create logger with all sinks
        logger = std::make_shared<spdlog::logger>(name, sinks.begin(), sinks.end());
        logger->set_level(spdlog::level::debug);
        logger->flush_on(spdlog::level::info);
        
        // Register logger in spdlog registry
        spdlog::register_logger(logger);
        
        // Log initialization
        logger->info("Logger initialized");
        return true;
    }
    catch (const spdlog::spdlog_ex& ex)
    {
        std::cerr << "Logger initialization failed: " << ex.what() << std::endl;
        return false;
    }
}

void Logger::debug(const std::string& message)
{
    if (logger)
    {
        logger->debug(message);
    }
}

void Logger::info(const std::string& message)
{
    if (logger)
    {
        logger->info(message);
    }
}

void Logger::warn(const std::string& message)
{
    if (logger)
    {
        logger->warn(message);
    }
}

void Logger::error(const std::string& message)
{
    if (logger)
    {
        logger->error(message);
    }
}

void Logger::critical(const std::string& message)
{
    if (logger)
    {
        logger->critical(message);
    }
}

} // namespace astral