#include "logger.h"
#include "utils.h"       // get_current_timestamp_str için
#include "../gui/panels/LogPanel.h" // YENİ: LogPanel'in tam tanımı için dahil edildi
#include <chrono>
#include <ctime>
#include <iomanip> // std::put_time
#include <string>  // For std::string
#include <locale>  // std::locale
#include <cerrno>  // errno


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

// YENİ: init metodu log_source parametresi aldı
void Logger::init(LogLevel level, const std::string& log_file_path, const std::string& log_source) {
    level_ = level;
    log_source_ = log_source; // YENİ: log_source başlatıldı
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

// YENİ: LogPanel'i kaydetme metodu implementasyonu
void Logger::set_log_panel(LogPanel* panel) {
    log_panel_ = panel;
    if (log_panel_) {
        LOG(LogLevel::INFO, "Logger: LogPanel entegrasyonu sağlandı.");
    } else {
        LOG(LogLevel::WARNING, "Logger: LogPanel null olarak ayarlandı.");
    }
}

// YENİ: log metodu: doğrudan string mesaj alır, sıra numarası ve kaynak ekler
void Logger::log(LogLevel level, const std::string& message, const char* file, int line) {
    if (level > level_) {
        return;
    }

    std::lock_guard<std::mutex> lock(mutex_);

    // Log sayacını artır
    log_counter_++;

    // Zamanı formatla
    std::string time_str = get_current_timestamp_str();

    // Dosya ve satır bilgisini ekle
    std::stringstream file_line_ss;
    file_line_ss << " (" << file << ":" << line << ")";

    // Oluşturulan tam log mesajı (Sıra numarası ve kaynak eklendi)
    std::string full_log_message = "[" + time_str + "] [" + level_to_string(level) + "] [" + log_source_ + ":" + std::to_string(log_counter_) + "] " +
                                   message + file_line_ss.str();

    // Konsol çıktısı (sadece LogPanel ayarlı değilse)
    if (!log_panel_) {
        std::cout << full_log_message << std::endl;
    }
    
    // Dosya çıktısı
    if (file_stream_.is_open()) {
        file_stream_ << full_log_message << std::endl;
    }

    // LogPanel çıktısı (eğer LogPanel ayarlıysa)
    if (log_panel_) {
        log_panel_->updatePanel(QStringList() << QString::fromStdString(full_log_message));
    }
}

// YENİ: log_error_to_cerr metodu implementasyonu (std::cerr'e özel)
void Logger::log_error_to_cerr(LogLevel level, const std::string& message, const char* file, int line) {
    if (level > level_) {
        return;
    }

    std::lock_guard<std::mutex> lock(mutex_);

    log_counter_++; // Sayacı artır

    std::string time_str = get_current_timestamp_str();
    std::stringstream file_line_ss;
    file_line_ss << " (" << file << ":" << line << ")";

    std::string full_log_message = "[" + time_str + "] [" + level_to_string(level) + "] [" + log_source_ + ":" + std::to_string(log_counter_) + "] " +
                                   message + file_line_ss.str();

    std::cerr << full_log_message << std::endl; // Her zaman std::cerr'e yaz

    if (file_stream_.is_open()) {
        file_stream_ << full_log_message << std::endl;
    }

    if (log_panel_) {
        log_panel_->updatePanel(QStringList() << QString::fromStdString(full_log_message));
    }
}

// ESKİ: std::ostream referansı alan log metodu - KALDIRILIYOR veya yeniden düzenleniyor
// Bu metodu kaldırmak, LOG makrosunun yeni log metodunu kullanmasını zorlayacak.
/*
void Logger::log(LogLevel level, std::ostream& os, const std::stringstream& message_stream, const char* file, int line) {
    // Bu metodun implementasyonu, artık doğrudan string mesaj alan log metoduna yönlendirilecek.
    log(level, message_stream.str(), file, line);
}
*/

// ESKİ: Varsayılan olarak std::cout'a yazar - KALDIRILIYOR veya yeniden düzenleniyor
/*
void Logger::log(LogLevel level, const std::stringstream& message_stream, const char* file, int line) {
    // Bu metodun implementasyonu, artık doğrudan string mesaj alan log metoduna yönlendirilecek.
    log(level, message_stream.str(), file, line);
}
*/