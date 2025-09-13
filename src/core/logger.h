#ifndef CEREBRUM_LUX_LOGGER_H
#define CEREBRUM_LUX_LOGGER_H

#include <string>
#include <fstream>
#include <sstream>
#include <mutex>
#include <iostream> // std::wcout, std::wcerr için
#include "enums.h" // LogLevel için

class Logger {
public:
    static Logger& get_instance();

    void init(LogLevel level, const std::wstring& log_file = L"");
    // Yeni log fonksiyonu: bir ostream referansı alır
    void log(LogLevel level, std::wostream& os, const std::wstringstream& message_stream, const char* file, int line);
    // Eski log fonksiyonu (varsayılan olarak std::wcout'a yazar)
    void log(LogLevel level, const std::wstringstream& message_stream, const char* file, int line);


    Logger(const Logger&) = delete;
    LogLevel get_level() const;

private:
    Logger() : level_(LogLevel::INFO) {}
    ~Logger();

    LogLevel level_;
    std::wofstream file_stream_;
    std::mutex mutex_;
};

// Yeni LOG makrosu: stream'i de belirtir
#define LOG(level, os, message_stream) do { if (Logger::get_instance().get_level() >= level) { std::wstringstream ss; ss << message_stream; Logger::get_instance().log(level, os, ss, __FILE__, __LINE__); } } while(0)

// Eski LOG makrosu (varsayılan olarak std::wcout'a yazar)
#define LOG_DEFAULT(level, message_stream) \
    do { \
        std::wstringstream log_ss_internal; \
        log_ss_internal << message_stream; \
        Logger::get_instance().log(level, log_ss_internal, __FILE__, __LINE__); \
    } while(0)


#endif // CEREBRUM_LUX_LOGGER_H
