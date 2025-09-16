#ifndef CEREBRUM_LUX_LOGGER_H
#define CEREBRUM_LUX_LOGGER_H

#include <string>
#include <fstream>
#include <sstream>
#include <mutex>
#include <iostream> // std::cout, std::cerr için
#include "enums.h" // LogLevel için
// utils.h'yi burada include etmiyoruz, Logger kendi bağımsızlığını koruyor.

class Logger {
public:
    static Logger& get_instance(); // Tekil (singleton) erişim

    void init(LogLevel level, const std::string& log_file_path = ""); // Log dosya yolu artık std::string
    void log(LogLevel level, std::ostream& os, const std::stringstream& message_stream, const char* file, int line);
    void log(LogLevel level, const std::stringstream& message_stream, const char* file, int line); // Varsayılan olarak std::cout'a yazar

    Logger(const Logger&) = delete; // Kopyalamayı engelle
    void operator=(const Logger&) = delete; // Atamayı engelle

    LogLevel get_level() const;
    std::string level_to_string(LogLevel level) const; // LogLevel'ı std::string'e çevirir

private:
    Logger() : level_(LogLevel::INFO) {} // Kurucu
    ~Logger(); // Yıkıcı

    LogLevel level_;
    std::ofstream file_stream_; // std::ofstream kullanıyoruz
    std::mutex mutex_; // Thread-safe loglama için

    // Global log seviyesi ve dosya akışını Logger sınıfının içinde yönetiyoruz
    // extern LogLevel g_current_log_level; // Bu artık sınıf üyesi olacak veya init ile ayarlanacak
    // extern std::wofstream g_log_file_stream; // Bu da sınıf üyesi olacak
};

// LOG_INIT makrosu: Logger'ı başlangıçta yapılandırmak için
#define LOG_INIT(log_file_name) Logger::get_instance().init(LogLevel::INFO, log_file_name)

// LOG makrosu: stream'i de belirtir (std::string tabanlı)
#define LOG(level, os, message_stream) \
    do { \
        if (Logger::get_instance().get_level() >= level) { \
            std::stringstream ss_log_stream; \
            ss_log_stream << message_stream; \
            Logger::get_instance().log(level, os, ss_log_stream, __FILE__, __LINE__); \
        } \
    } while(0)

// LOG_DEFAULT makrosu (varsayılan olarak std::cout'a yazar, std::string tabanlı)
#define LOG_DEFAULT(level, message_stream) \
    do { \
        if (Logger::get_instance().get_level() >= level) { \
            std::stringstream log_ss_internal; \
            log_ss_internal << message_stream; \
            Logger::get_instance().log(level, log_ss_internal, __FILE__, __LINE__); \
        } \
    } while(0)


#endif // CEREBRUM_LUX_LOGGER_H