#ifndef LOGGER_H
#define LOGGER_H

#include <string>
#include <fstream>
#include <sstream>
#include <mutex>
#include <memory> // std::unique_ptr için
#include <QTextEdit> // Qt GUI log paneli için

#include "enums.h" // LogLevel enum'ı için

namespace CerebrumLux { // Logger sınıfı bu namespace içine alınacak

// Logger sınıfı bir Singleton olarak tasarlandı
class Logger {
public:
    // Singleton örneğini döndürür
    static Logger& get_instance();

    // Logger'ı başlatır. Sadece bir kez çağrılmalı.
    void init(LogLevel level, const std::string& log_file_path = "", const std::string& log_source = "SYSTEM");

    // Mesajı loglar
    void log(LogLevel level, const std::string& message, const char* file, int line);
    void log_error_to_cerr(LogLevel level, const std::string& message, const char* file, int line); // YAZIM HATASI DÜZELTİLDİ

    // GUI'deki QTextEdit widget'ına logları yönlendirmek için
    void set_log_panel_text_edit(QTextEdit* text_edit);
    void flush_buffered_logs(); // Buffer'daki logları GUI'ye aktarır

    // Geçerli log seviyesini döndürür
    LogLevel get_level() const;

    // LogLevel'ı string'e dönüştürür (public yapıldı)
    std::string level_to_string(LogLevel level) const;

private:
    Logger(); // Kurucu private
    ~Logger(); // Yıkıcı private
    Logger(const Logger&) = delete; // Kopyalama engellenir
    Logger& operator=(const Logger&) = delete; // Atama engellenir

    LogLevel level_;
    std::ofstream log_file_;
    std::mutex mutex_;
    QTextEdit* m_guiLogTextEdit; // GUI'deki QTextEdit pointer'ı
    std::vector<std::string> log_buffer_; // GUI'ye bağlanmadan önceki loglar için
    unsigned long long log_counter_;
    std::string log_source_;

    // Sadece dahili kullanım için
    std::string format_log_message(LogLevel level, const std::string& message, const char* file, int line);
};

// Kolay loglama için makrolar
#define LOG(level, message) CerebrumLux::Logger::get_instance().log(level, (std::stringstream() << message).str(), __FILE__, __LINE__)
#define LOG_DEFAULT(level, message) CerebrumLux::Logger::get_instance().log(level, (std::stringstream() << message).str(), __FILE__, __LINE__)
#define LOG_ERROR_CERR(level, message) CerebrumLux::Logger::get_instance().log_error_to_cerr(level, (std::stringstream() << message).str(), __FILE__, __LINE__)

} // namespace CerebrumLux

#endif // LOGGER_H