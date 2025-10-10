#include "logger.h"
#include <iostream>
#include <chrono>
#include <iomanip> // std::put_time için
#include <ctime>   // std::localtime için
#include <algorithm> // std::find_if için
#include <QThread> // QThread::currentThreadId için
#include <QDateTime> // QDateTime için

namespace CerebrumLux { // TÜM İMPLEMENTASYON BU NAMESPACE İÇİNDE OLACAK

// Singleton örneğinin başlatılması
Logger& Logger::getInstance() { // get_instance yerine getInstance
    static Logger instance; // Tek bir örnek oluştur
    return instance;
}

// Kurucu (private)
Logger::Logger()
    : QObject(nullptr), level_(LogLevel::INFO), log_counter_(0), log_source_("SYSTEM") {} // QObject constructor'ı eklendi, m_guiLogTextEdit kaldırıldı

// Yıkıcı (private)
Logger::~Logger() {
    if (log_file_.is_open()) {
        log_file_.close();
    }
}

// LogLevel'ı string'e dönüştürür
std::string Logger::level_to_string(LogLevel level) const {
    switch (level) {
        case LogLevel::TRACE:       return "TRACE";
        case LogLevel::DEBUG:       return "DEBUG";
        case LogLevel::INFO:        return "INFO";
        case LogLevel::WARNING:     return "WARNING";
        case LogLevel::ERR_CRITICAL: return "CRITICAL_ERROR";
        default:                    return "UNKNOWN";
    }
}

void Logger::init(LogLevel level, const std::string& log_file_path, const std::string& log_source) {
    std::lock_guard<std::mutex> lock(mutex_);
    level_ = level;
    log_source_ = log_source;
    if (!log_file_path.empty()) {
        log_file_.open(log_file_path, std::ios_base::app);
        if (!log_file_.is_open()) {
            std::cerr << "ERROR: Failed to open log file: " << log_file_path << std::endl;
        }
    }
}

void Logger::log(LogLevel level, const std::string& message, const char* file, int line) {
    if (!shouldLog(level)) {
        return;
    }

    std::string formatted_message = format_log_message(level, message, file, line);

    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (log_file_.is_open()) {
            log_file_ << formatted_message << std::endl;
            log_file_.flush();
        } else {
            std::cout << formatted_message << std::endl;
        }
    }

    // GUI'ye sinyal gönder
    emit messageLogged(level, QString::fromStdString(message), QString(file), line);
}

void Logger::log_error_to_cerr(LogLevel level, const std::string& message, const char* file, int line) {
    if (!shouldLog(level)) {
        return;
    }

    std::string formatted_message = format_log_message(level, message, file, line);

    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (log_file_.is_open()) {
            log_file_ << formatted_message << std::endl;
            log_file_.flush();
        } else {
            std::cerr << formatted_message << std::endl;
        }
    }

    // GUI'ye sinyal gönder
    emit messageLogged(level, QString::fromStdString(message), QString(file), line);
}

LogLevel Logger::get_level() const {
    return level_;
}

std::string Logger::format_log_message(LogLevel level, const std::string& message, const char* file, int line) {
    std::stringstream ss;
    ss << QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss.zzz").toStdString(); // QString'i std::string'e dönüştür
    ss << std::string(" [") << std::setw(3) << ++log_counter_ << std::string("] ");
    ss << std::string("[") << log_source_ << std::string(":") << level_to_string(level) << std::string("] ");
    ss << std::string("[") << file << std::string(":") << line << std::string("] ");
     ss << message;
    
    return ss.str();
}

} // namespace CerebrumLux