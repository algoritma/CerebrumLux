#include "logger.h"
#include "utils.h"       
// #include "../gui/panels/LogPanel.h" // Artık doğrudan LogPanel'e erişmiyoruz.
#include <chrono>
#include <ctime>
#include <iomanip> 
#include <string>  
#include <locale>  
#include <cerrno>  
#include <QMetaObject> 
#include <QString> 
#include <QTextEdit> 

std::string Logger::level_to_string(LogLevel level) const {
    switch (level) {
        case LogLevel::SILENT: return "SILENT";
        case LogLevel::ERR_CRITICAL: return "CRITICAL";
        case LogLevel::WARNING: return "WARNING";
        case LogLevel::INFO: return "INFO";
        case LogLevel::DEBUG: return "DEBUG";
        case LogLevel::TRACE: return "TRACE";
        default: return "UNKNOWN";
    }
}

Logger& Logger::get_instance() {
    static Logger instance;
    return instance;
}

void Logger::init(LogLevel level, const std::string& log_file_path, const std::string& log_source) {
    std::lock_guard<std::mutex> lock(mutex_); 
    level_ = level;
    log_source_ = log_source; 
    if (!log_file_path.empty()) {
        if (file_stream_.is_open()) {
            file_stream_.close();
        }
        file_stream_.open(log_file_path, std::ios_base::app); 
        if (!file_stream_.is_open()) {
            std::cerr << "Hata: Log dosyası açılamadı: " << log_file_path << " (errno: " << errno << ")\n";
        }
    }
}

Logger::~Logger() {
    if (file_stream_.is_open()) {
        file_stream_.close();
    }
}

LogLevel Logger::get_level() const {
    return level_;
}

void Logger::set_log_panel_text_edit(QTextEdit* textEdit) {
    std::lock_guard<std::mutex> lock(buffer_mutex); 
    m_guiLogTextEdit = textEdit;
    if (m_guiLogTextEdit) {
        for (const std::string& buffered_log : initial_log_buffer) {
            QMetaObject::invokeMethod(m_guiLogTextEdit, "append", Qt::QueuedConnection, Q_ARG(QString, QString::fromStdString(buffered_log)));
        }
        initial_log_buffer.clear(); 
    }
}

void Logger::log(LogLevel level, const std::string& message, const char* file, int line) {
    if (level > level_) {
        return;
    }

    std::lock_guard<std::mutex> lock(mutex_); 

    log_counter_++;

    std::string time_str = get_current_timestamp_str();
    std::stringstream file_line_ss;
    file_line_ss << " (" << file << ":" << line << ")";

    std::string full_log_message = "[" + time_str + "] [" + level_to_string(level) + "] [" + log_source_ + ":" + std::to_string(log_counter_) + "] " +
                                   message + file_line_ss.str();

    {
        std::lock_guard<std::mutex> buffer_lock(buffer_mutex); 
        if (m_guiLogTextEdit) {
            QMetaObject::invokeMethod(m_guiLogTextEdit, "append", Qt::QueuedConnection,
                                      Q_ARG(QString, QString::fromStdString(full_log_message)));
        } else {
            initial_log_buffer.push_back(full_log_message);
        }
    }
    
    if (file_stream_.is_open()) {
        file_stream_ << full_log_message << std::endl;
    }
}

void Logger::log_error_to_cerr(LogLevel level, const std::string& message, const char* file, int line) {
    if (level > level_) {
        return;
    }

    std::lock_guard<std::mutex> lock(mutex_); 

    log_counter_++; 

    std::string time_str = get_current_timestamp_str();
    std::stringstream file_line_ss;
    file_line_ss << " (" << file << ":" << line << ")";

    std::string full_log_message = "[" + time_str + "] [" + level_to_string(level) + "] [" + log_source_ + ":" + std::to_string(log_counter_) + "] " +
                                   message + file_line_ss.str();

    {
        std::lock_guard<std::mutex> buffer_lock(buffer_mutex); 
        if (m_guiLogTextEdit) {
            QMetaObject::invokeMethod(m_guiLogTextEdit, "append", Qt::QueuedConnection,
                                      Q_ARG(QString, QString::fromStdString(full_log_message)));
        } else {
            initial_log_buffer.push_back(full_log_message);
        }
    }

    if (file_stream_.is_open()) {
        file_stream_ << full_log_message << std::endl;
    }
}