#ifndef CEREBRUM_LUX_LOGGER_H
#define CEREBRUM_LUX_LOGGER_H

#include <string>
#include <fstream>
#include <sstream>
#include <mutex>
#include "../core/enums.h"

class Logger {
public:
    static Logger& get_instance();

    void init(LogLevel level, const std::wstring& log_file = L"");
    void log(LogLevel level, const std::wstringstream& message, const char* file, int line);

    Logger(const Logger&) = delete;
    LogLevel get_level() const;

private:
    Logger() : level_(LogLevel::INFO) {}
    ~Logger();

    LogLevel level_;
    std::wofstream file_stream_;
    std::mutex mutex_;
};

#define LOG(level, message) \
    do { \
        std::wstringstream log_ss_internal; \
        log_ss_internal << message; \
        Logger::get_instance().log(level, log_ss_internal, __FILE__, __LINE__); \
    } while(0)


#endif // CEREBRUM_LUX_LOGGER_H
