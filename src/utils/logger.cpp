#include <logger.h>
#include <repository.h>
#include <iostream>
#include <filesystem>

Logger::Logger() : minLevel(LogLevel::INFO), consoleOutput(true), initialized(false) {}

Logger& Logger::instance() {
    static Logger logger;
    return logger;
}

void Logger::init(const std::string& logPath) {
    std::lock_guard<std::mutex> lock(mutex);
    
    if (initialized) {
        return;
    }
    
    try {
        std::filesystem::path logDir = std::filesystem::path(logPath).parent_path();
        if (!logDir.empty() && logDir != ".") {
            std::filesystem::create_directories(logDir);
        }
        
        fileStream = std::make_unique<std::ofstream>(logPath, std::ios::app);
        if (!fileStream->is_open()) {
            std::cerr << "Failed to open log file: " << logPath << std::endl;
            consoleOutput = true;
            initialized = true;
            return;
        }
        
        initialized = true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to initialize logger: " << e.what() << std::endl;
        consoleOutput = true;
        initialized = true;
    }
}

void Logger::setLevel(LogLevel level) {
    std::lock_guard<std::mutex> lock(mutex);
    minLevel = level;
}

void Logger::setConsoleOutput(bool enable) {
    std::lock_guard<std::mutex> lock(mutex);
    consoleOutput = enable;
}

void Logger::debug(const std::string& message) {
    log(LogLevel::DEBUG, message);
}

void Logger::info(const std::string& message) {
    log(LogLevel::INFO, message);
}

void Logger::warning(const std::string& message) {
    log(LogLevel::WARNING, message);
}

void Logger::error(const std::string& message) {
    log(LogLevel::ERROR, message);
}

void Logger::fatal(const std::string& message) {
    log(LogLevel::FATAL, message);
}

void Logger::log(LogLevel level, const std::string& message) {
    if (level < minLevel) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(mutex);
    
    std::string timestamp = getCurrentTimestamp();
    std::string levelStr = levelToString(level);
    std::ostringstream oss;
    oss << "[" << timestamp << "] [" << levelStr << "] " << message;
    
    std::string logLine = oss.str();
    
    if (consoleOutput) {
        if (level >= LogLevel::ERROR) {
            std::cerr << logLine << std::endl;
        } else {
            std::cout << logLine << std::endl;
        }
    }
    
    if (fileStream && fileStream->is_open()) {
        *fileStream << logLine << std::endl;
        fileStream->flush();
    }
}

void Logger::logException(const std::string& context, const std::exception& e) {
    std::ostringstream oss;
    oss << context << ": " << e.what();
    error(oss.str());
}

std::string Logger::getCurrentTimestamp() const {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::tm tm = *std::localtime(&time);
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    oss << '.' << std::setfill('0') << std::setw(3) << ms.count();
    return oss.str();
}

std::string Logger::levelToString(LogLevel level) const {
    switch (level) {
        case LogLevel::DEBUG:   return "DEBUG";
        case LogLevel::INFO:    return "INFO";
        case LogLevel::WARNING: return "WARN";
        case LogLevel::ERROR:   return "ERROR";
        case LogLevel::FATAL:   return "FATAL";
        default:                return "UNKNOWN";
    }
}
