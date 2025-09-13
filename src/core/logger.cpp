#include "logger.h"
#include "utils.h" // For get_current_timestamp_wstr and log_level_to_string
#include <iostream>

Logger& Logger::get_instance() {
    static Logger instance;
    return instance;
}

Logger::~Logger() {
    if (file_stream_.is_open()) {
        file_stream_.close();
    }
}

void Logger::init(LogLevel level, const std::wstring& log_file) {
    std::lock_guard<std::mutex> lock(mutex_);
    level_ = level;
    if (!log_file.empty()) {
        file_stream_.open(log_file, std::ios_base::out | std::ios_base::trunc);
        if (!file_stream_.is_open()) {
            std::wcerr << L"Error: Could not open log file: " << log_file << std::endl;
        }
    }
}

LogLevel Logger::get_level() const {
    return level_;
}

void Logger::log(LogLevel level, const std::wstringstream& message, const char* file, int line) {
    if (level > level_) {
        return;
    }

    std::lock_guard<std::mutex> lock(mutex_);

    std::wstringstream log_stream;
    log_stream << L"[" << log_level_to_string(level) << L"]";
    log_stream << L"[" << get_current_timestamp_wstr() << L"]";
    
    std::string narrow_file(file);
    std::wstring wide_file(narrow_file.begin(), narrow_file.end());
    log_stream << L"[" << wide_file << L":" << line << L"] ";
    
    log_stream << message.str() << L'\n';

    // Write to console
    if (level <= LogLevel::ERR_CRITICAL) {
        std::wcerr << log_stream.str();
    } else {
        std::wcout << log_stream.str();
    }

    // Write to file
    if (file_stream_.is_open()) {
        file_stream_ << log_stream.str();
        if (level <= LogLevel::ERR_CRITICAL) {
            file_stream_.flush();
        }
    }
}
