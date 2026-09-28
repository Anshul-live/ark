#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <string>
#include <fstream>
#include <mutex>
#include <memory>
#include <chrono>
#include <iomanip>
#include <sstream>

enum class LogLevel {
    DEBUG,
    INFO,
    WARNING,
    ERROR,
    FATAL
};

class Logger {
public:
    static Logger& instance();
    
    void init(const std::string& logPath);
    void setLevel(LogLevel level);
    void setConsoleOutput(bool enable);
    
    void debug(const std::string& message);
    void info(const std::string& message);
    void warning(const std::string& message);
    void error(const std::string& message);
    void fatal(const std::string& message);
    
    void log(LogLevel level, const std::string& message);
    void logException(const std::string& context, const std::exception& e);

private:
    Logger();
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    
    std::string getCurrentTimestamp() const;
    std::string levelToString(LogLevel level) const;
    
    std::unique_ptr<std::ofstream> fileStream;
    std::mutex mutex;
    LogLevel minLevel;
    bool consoleOutput;
    bool initialized;
};

#define LOG_DEBUG(msg) Logger::instance().debug(msg)
#define LOG_INFO(msg) Logger::instance().info(msg)
#define LOG_WARNING(msg) Logger::instance().warning(msg)
#define LOG_ERROR(msg) Logger::instance().error(msg)
#define LOG_FATAL(msg) Logger::instance().fatal(msg)
#define LOG_EXCEPTION(ctx, e) Logger::instance().logException(ctx, e)

#endif
