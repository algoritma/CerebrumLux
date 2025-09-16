#ifndef CEREBRUM_LUX_LOGGER_H
#define CEREBRUM_LUX_LOGGER_H

#include <string>
#include <fstream>
#include <sstream>
#include <mutex>
#include <iostream> // std::cout, std::cerr için
#include "enums.h" // LogLevel için
#include "utils.h" // convert_wstring_to_string için

class Logger {
public:
    static Logger& get_instance();

    void init(LogLevel level, const std::string& log_file = ""); // log_file artık string
    // Yeni log fonksiyonu: bir ostream referansı alır
    void log(LogLevel level, std::ostream& os, const std::stringstream& message_stream, const char* file, int line);
    // Eski log fonksiyonu (varsayılan olarak std::cout'a yazar)
    void log(LogLevel level, const std::stringstream& message_stream, const char* file, int line);

    Logger(const Logger&) = delete;
    LogLevel get_level() const;

    // LogLevel'ı string'e çevirmek için public bir metot
    std::string level_to_string(LogLevel level) const; // Artık string döndürüyor

private:
    Logger() : level_(LogLevel::INFO) {}
    ~Logger();

    LogLevel level_;
    std::ofstream file_stream_;
    std::mutex mutex_;
};

// LOG_INIT makrosu: Logger'ı başlangıçta yapılandırmak için
#define LOG_INIT(log_file_name) Logger::get_instance().init(LogLevel::INFO, log_file_name)

// Yeni LOG makrosu: stream'i de belirtir
#define LOG(level, os, message_stream) \
    do { \
        if (Logger::get_instance().get_level() >= level) { \
            std::stringstream ss_log_stream; \
            ss_log_stream << message_stream; \
            Logger::get_instance().log(level, os, ss_log_stream, __FILE__, __LINE__); \
        } \
    } while(0)

// Eski LOG makrosu (varsayılan olarak std::cout'a yazar)
#define LOG_DEFAULT(level, message_stream) \
    do { \
        std::stringstream log_ss_internal; \
        log_ss_internal << message_stream; \
        Logger::get_instance().log(level, log_ss_internal, __FILE__, __LINE__); \
    } while(0)


#endif // CEREBRUM_LUX_LOGGER_H