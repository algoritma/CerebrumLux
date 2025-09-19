#ifndef CEREBRUM_LUX_LOGGER_H
#define CEREBRUM_LUX_LOGGER_H

#include <string>
#include <fstream>
#include <sstream>
#include <mutex>
#include <iostream> // std::cout, std::cerr için
#include "enums.h" // LogLevel için

// YENİ: LogPanel için ileri bildirim
class LogPanel; 

class Logger {
public:
    static Logger& get_instance(); // Tekil (singleton) erişim

    // YENİ: init metodu log_source parametresi aldı
    void init(LogLevel level, const std::string& log_file_path = "", const std::string& log_source = "SYSTEM"); 
    
    // YENİ: log metotları stream yerine doğrudan string mesaj alıyor
    void log(LogLevel level, const std::string& message, const char* file, int line);
    // std::cerr için özel log metodu
    void log_error_to_cerr(LogLevel level, const std::string& message, const char* file, int line);


    Logger(const Logger&) = delete; // Kopyalamayı engelle
    void operator=(const Logger&) = delete; // Atamayı engelle

    LogLevel get_level() const;
    std::string level_to_string(LogLevel level) const; // LogLevel'ı std::string'e çevirir

    // YENİ: LogPanel'i kaydetme metodu
    void set_log_panel(LogPanel* panel);

private:
    Logger() : level_(LogLevel::INFO), log_panel_(nullptr), log_counter_(0), log_source_("SYSTEM") {} // Kurucu güncellendi
    ~Logger(); // Yıkıcı

    LogLevel level_;
    std::ofstream file_stream_; // std::ofstream kullanıyoruz
    std::mutex mutex_; // Thread-safe loglama için
    LogPanel* log_panel_; // LogPanel pointer'ı
    unsigned long long log_counter_; // YENİ: Log sıra numarası
    std::string log_source_; // YENİ: Log kaynağı (örn: SYSTEM, GUI, AI_CORE)
};

// LOG_INIT makrosu: Logger'ı başlangıçta yapılandırmak için (log_source parametresi eklendi)
#define LOG_INIT(log_file_name, log_source_name) Logger::get_instance().init(LogLevel::INFO, log_file_name, log_source_name)

// YENİ: LOG makrosu (doğrudan string mesaj alır)
#define LOG(level, message) \
    do { \
        if (Logger::get_instance().get_level() >= level) { \
            std::stringstream ss_log_stream; \
            ss_log_stream << message; \
            Logger::get_instance().log(level, ss_log_stream.str(), __FILE__, __LINE__); \
        } \
    } while(0)

// YENİ: LOG_DEFAULT makrosu (varsayılan olarak LogPanel'e veya dosyaya yazar)
#define LOG_DEFAULT(level, message) \
    do { \
        if (Logger::get_instance().get_level() >= level) { \
            std::stringstream log_ss_internal; \
            log_ss_internal << message; \
            Logger::get_instance().log(level, log_ss_internal.str(), __FILE__, __LINE__); \
        } \
    } while(0)

// YENİ: LOG_ERROR_CERR makrosu (std::cerr'e yazmak için özel)
#define LOG_ERROR_CERR(level, message) \
    do { \
        if (Logger::get_instance().get_level() >= level) { \
            std::stringstream log_ss_internal_err; \
            log_ss_internal_err << message; \
            Logger::get_instance().log_error_to_cerr(level, log_ss_internal_err.str(), __FILE__, __LINE__); \
        } \
    } while(0)


#endif // CEREBRUM_LUX_LOGGER_H