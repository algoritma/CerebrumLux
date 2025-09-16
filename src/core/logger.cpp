#include "logger.h"
#include "../core/utils.h" // For convert_wstring_to_string
#include <chrono>
#include <ctime>
#include <iomanip> // std::put_time
#include <string>  // For std::string
#include <locale>


// LogLevel'ı string'e çeviren yardımcı fonksiyon
// Logger sınıfının bir üye fonksiyonu olarak LogLevel'ı string'e çeviren metot
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

void Logger::init(LogLevel level, const std::string& log_file) {
    level_ = level;
    if (!log_file.empty()) {
        // log_file zaten string, doğrudan aç
        file_stream_.open(log_file.c_str(), std::ios_base::app);
        if (!file_stream_.is_open()) {
            // Hata durumunda konsola yaz (std::cerr kullan)
            std::cerr << "Hata: Log dosyası açılamadı: " << log_file << std::endl;
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

// Yeni log fonksiyonu: bir ostream referansı alır
void Logger::log(LogLevel level, std::ostream& os, const std::stringstream& message_stream, const char* file, int line) {
    if (level > level_) { // Sadece belirlenen seviye ve altındaki mesajları logla
        return;
    }

    std::lock_guard<std::mutex> lock(mutex_); // Thread-safe loglama için

    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);

    // Zamanı formatla
    std::stringstream time_ss;
    time_ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %H:%M:%S");

    // Dosya ve satır bilgisini ekle
    std::stringstream file_line_ss;
    file_line_ss << " (" << file << ":" << line << ")";

    // Konsol çıktısı
    os << "[" << time_ss.str() << "] [" << Logger::get_instance().level_to_string(level) << "] "
       << message_stream.str() << file_line_ss.str() << std::endl;

    if (file_stream_.is_open()) {
        // Dosya çıktısı
        file_stream_ << "[" << time_ss.str() << "] [" << Logger::get_instance().level_to_string(level) << "] "
                     << message_stream.str() << file_line_ss.str() << std::endl;
        file_stream_.flush();
    }
}

// Eski log fonksiyonu (varsayılan olarak std::cout'a yazar)
void Logger::log(LogLevel level, const std::stringstream& message_stream, const char* file, int line) {
    log(level, std::cout, message_stream, file, line); // std::cout'a yönlendir
}