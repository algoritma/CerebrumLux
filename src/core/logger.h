#ifndef LOGGER_H
#define LOGGER_H

#include <string>
#include <sstream>
#include <mutex>
#include <chrono>
#include <iomanip> // std::put_time için
#include <fstream> // logging to file
#include <QObject> // Q_OBJECT için
#include <QString> // QString için
#include <QThread> // QThread::currentThreadId için

#include "enums.h" // LogLevel enum'ı için

namespace CerebrumLux { // Logger sınıfı bu namespace içine alınacak

// Logger sınıfı bir Singleton olarak tasarlandı
class Logger : public QObject { // QObject'ten türemesi için eklendi
    Q_OBJECT // Sinyal/slot mekanizması için gerekli

public:
    // Singleton örneğini döndürür
    static Logger& getInstance(); // get_instance yerine getInstance

    // Logger'ı başlatır. Sadece bir kez çağrılmalı.
    void init(LogLevel level, const std::string& log_file_path = "", const std::string& log_source = "SYSTEM");

    // Logger'ı güvenli bir şekilde kapatır ve kaynakları serbest bırakır.
    void shutdown();

    // Mesajı loglar
    void log(LogLevel level, const std::string& message, const char* file, int line);
    void log_error_to_cerr(LogLevel level, const std::string& message, const char* file, int line);

    // Log seviyesi kontrolü
    bool shouldLog(LogLevel level) const { return level >= level_; } // Public ve const yapıldı

signals:
    // GUI'ye gönderilecek sinyal. Ham mesaj, dosya ve satır bilgisi ile birlikte.
    void messageLogged(CerebrumLux::LogLevel level, const QString& rawMessage, const QString& file, int line);

private:
    Logger(); // Kurucu private
    ~Logger() override; // Yıkıcı private ve override keyword eklendi
    Logger(const Logger&) = delete; // Kopyalama engellenir
    Logger& operator=(const Logger&) = delete; // Atama engellenir

    // m_guiLogTextEdit ve log_buffer_ artık burada tutulmuyor.
    // LogPanel kendi bağlantısını yönetecek.

    // Geçerli log seviyesini döndürür
    LogLevel get_level() const;

    // LogLevel'ı string'e dönüştürür (public yapıldı)
    std::string level_to_string(LogLevel level) const;

    // Sadece dahili kullanım için
    std::string format_log_message(LogLevel level, const std::string& message, const char* file, int line);

    LogLevel level_;
    std::ofstream log_file_;
    std::mutex mutex_;
    unsigned long long log_counter_;
    std::string log_source_;
};

} // namespace CerebrumLux
namespace CerebrumLux {

// Kolay loglama için makrolar
#define LOG(level, message) CerebrumLux::Logger::getInstance().log(level, (std::stringstream() << message).str(), __FILE__, __LINE__)
#define LOG_DEFAULT(level, message) CerebrumLux::Logger::getInstance().log(level, (std::stringstream() << message).str(), __FILE__, __LINE__)
#define LOG_ERROR_CERR(level, message) CerebrumLux::Logger::getInstance().log_error_to_cerr(level, (std::stringstream() << message).str(), __FILE__, __LINE__)

} // namespace CerebrumLux

#endif // LOGGER_H