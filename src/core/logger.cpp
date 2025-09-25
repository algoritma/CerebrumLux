#include "logger.h"
#include <iostream>
#include <chrono>
#include <iomanip> // std::put_time için
#include <ctime>   // std::localtime, std::gmtime için
#include <algorithm> // std::find_if için

namespace CerebrumLux { // TÜM İMPLEMENTASYON BU NAMESPACE İÇİNDE OLACAK

// Singleton örneğinin başlatılması
Logger& Logger::get_instance() {
    static Logger instance;
    return instance;
}

// Kurucu (private)
Logger::Logger()
    : level_(LogLevel::INFO), m_guiLogTextEdit(nullptr), log_counter_(0), log_source_("SYSTEM") {}

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

// Mesajı loglar
void Logger::log(LogLevel level, const std::string& message, const char* file, int line) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (level < level_) { // Belirlenen seviyenin altındaki logları yoksay
        return;
    }

    std::string formatted_message = format_log_message(level, message, file, line);

    if (m_guiLogTextEdit) {
        m_guiLogTextEdit->append(QString::fromStdString(formatted_message));
    } else {
        log_buffer_.push_back(formatted_message); // GUI'ye bağlanana kadar buffer'da tut
    }

    if (log_file_.is_open()) {
        log_file_ << formatted_message << std::endl;
        log_file_.flush();
    } else {
        std::cout << formatted_message << std::endl; // Dosya açılamazsa konsola yaz
    }
}

void Logger::log_error_to_cerr(LogLevel level, const std::string& message, const char* file, int line) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (level < level_) { // Belirlenen seviyenin altındaki logları yoksay
        return;
    }

    std::string formatted_message = format_log_message(level, message, file, line);

    if (m_guiLogTextEdit) {
        m_guiLogTextEdit->append(QString::fromStdString(formatted_message));
    } else {
        log_buffer_.push_back(formatted_message);
    }
    
    if (log_file_.is_open()) {
        log_file_ << formatted_message << std::endl;
        log_file_.flush();
    } else {
        std::cerr << formatted_message << std::endl; // Hata loglarını her zaman stderr'e yaz
    }
}


void Logger::set_log_panel_text_edit(QTextEdit* text_edit) {
    std::lock_guard<std::mutex> lock(mutex_);
    m_guiLogTextEdit = text_edit;
    flush_buffered_logs(); // Buffer'daki logları hemen GUI'ye aktar
}

void Logger::flush_buffered_logs() {
    if (m_guiLogTextEdit) {
        for (const auto& msg : log_buffer_) {
            m_guiLogTextEdit->append(QString::fromStdString(msg));
        }
        log_buffer_.clear();
    }
}

LogLevel Logger::get_level() const {
    return level_;
}


std::string Logger::format_log_message(LogLevel level, const std::string& message, const char* file, int line) {
    auto now = std::chrono::system_clock::now();
    auto milli = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
    std::time_t timer = std::chrono::system_clock::to_time_t(now);
    std::tm bt = *std::localtime(&timer);

    std::stringstream ss;
    ss << std::put_time(&bt, "%Y-%m-%d %H:%M:%S");
    ss << '.' << std::setfill('0') << std::setw(3) << milli.count();
    
    ss << " [" << std::setw(3) << ++log_counter_ << "] ";
    ss << "[" << log_source_ << ":" << level_to_string(level) << "] ";
    ss << "[" << file << ":" << line << "] ";
    ss << message;
    
    return ss.str();
}

} // namespace CerebrumLux