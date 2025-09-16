#include "logger.h"
#include "utils.h"       // get_current_timestamp_str için
#include <chrono>
#include <ctime>
#include <iomanip> // std::put_time
#include <string>  // For std::string
#include <locale>  // std::locale
#include <cerrno>  // errno

// Global logger değişkenlerinin tanımı (Artık Logger sınıfının içinde yönetilecek)
// LogLevel g_current_log_level = LogLevel::INFO; // Kaldırıldı
// std::wofstream g_log_file_stream; // Kaldırıldı


// === Logger Sınıfı Implementasyonları ===

// LogLevel'ı string'e çeviren yardımcı fonksiyon (Logger sınıfının üyesi)
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

void Logger::init(LogLevel level, const std::string& log_file_path) {
    level_ = level;
    if (!log_file_path.empty()) {
        // Eğer dosya zaten açıksa kapat
        if (file_stream_.is_open()) {
            file_stream_.close();
        }
        file_stream_.open(log_file_path, std::ios_base::app); // std::string ile aç
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

// Log fonksiyonu: bir ostream referansı alır (std::string tabanlı)
void Logger::log(LogLevel level, std::ostream& os, const std::stringstream& message_stream, const char* file, int line) {
    if (level > level_) { // Sadece belirlenen seviye ve altındaki mesajları logla
        return;
    }

    std::lock_guard<std::mutex> lock(mutex_); // Thread-safe loglama için

    // Zamanı formatla
    std::string time_str = get_current_timestamp_str(); // utils.h'den gelir

    // Dosya ve satır bilgisini ekle
    std::stringstream file_line_ss;
    file_line_ss << " (" << file << ":" << line << ")";

    // Konsol çıktısı
    os << "[" << time_str << "] [" << level_to_string(level) << "] "
       << message_stream.str() << file_line_ss.str() << std::endl;

    if (file_stream_.is_open()) {
        // Dosya çıktısı
        file_stream_ << "[" << time_str << "] [" << level_to_string(level) << "] "
                     << message_stream.str() << file_line_ss.str() << std::endl;
        file_stream_.flush();
    }
}

// Varsayılan olarak std::cout'a yazar (std::string tabanlı)
void Logger::log(LogLevel level, const std::stringstream& message_stream, const char* file, int line) {
    log(level, std::cout, message_stream, file, line);
}