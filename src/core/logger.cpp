#include "logger.h"
#include <chrono>
#include <ctime>
#include <iomanip> // std::put_time
#include <locale>  // std::use_facet, std::numpunct

// LogLevel'ı string'e çeviren yardımcı fonksiyon
std::wstring log_level_to_string(LogLevel level) {
    switch (level) {
        case LogLevel::SILENT: return L"SILENT";
        case LogLevel::ERR_CRITICAL: return L"CRITICAL";
        case LogLevel::WARNING: return L"WARNING";
        case LogLevel::INFO: return L"INFO";
        case LogLevel::DEBUG: return L"DEBUG";
        case LogLevel::TRACE: return L"TRACE";
        default: return L"UNKNOWN";
    }
}

Logger& Logger::get_instance() {
    static Logger instance;
    return instance;
}

void Logger::init(LogLevel level, const std::wstring& log_file) {
    level_ = level;
    if (!log_file.empty()) {
        file_stream_.open(log_file, std::ios_base::app);
        if (!file_stream_.is_open()) {
            // Hata durumunda konsola yaz
            std::wcerr << L"Hata: Log dosyası açılamadı: " << log_file << std::endl;
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
void Logger::log(LogLevel level, std::wostream& os, const std::wstringstream& message_stream, const char* file, int line) {
    if (level > level_) { // Sadece belirlenen seviye ve altındaki mesajları logla
        return;
    }

    std::lock_guard<std::mutex> lock(mutex_); // Thread-safe loglama için

    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);

    // Zamanı formatla
    std::wstringstream time_ss;
    time_ss << std::put_time(std::localtime(&in_time_t), L"%Y-%m-%d %H:%M:%S");

    // Dosya ve satır bilgisini ekle
    std::wstringstream file_line_ss;
    file_line_ss << L" (" << file << L":" << line << L")";

    os << L"[" << time_ss.str() << L"] [" << log_level_to_string(level) << L"] "
       << message_stream.str() << file_line_ss.str() << std::endl;

    if (file_stream_.is_open()) {
        file_stream_ << L"[" << time_ss.str() << L"] [" << log_level_to_string(level) << L"] "
                     << message_stream.str() << file_line_ss.str() << std::endl;
        file_stream_.flush();
    }
}

// Eski log fonksiyonu (varsayılan olarak std::wcout'a yazar)
void Logger::log(LogLevel level, const std::wstringstream& message_stream, const char* file, int line) {
    log(level, std::wcout, message_stream, file, line); // std::wcout'a yönlendir
}