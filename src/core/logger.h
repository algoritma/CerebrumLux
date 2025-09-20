#ifndef CEREBRUM_LUX_LOGGER_H
#define CEREBRUM_LUX_LOGGER_H

#include <string>
#include <fstream>
#include <sstream>
#include <mutex>
#include <vector> // initial_log_buffer için
#include <iostream> // std::cout, std::cerr için (artık Logger doğrudan kullanmasa da bazı makrolar veya başka yerler için)
#include "enums.h" // LogLevel için
#include <QTextEdit> // YENİ: QTextEdit için

// İleri bildirim kaldırıldı, doğrudan QTextEdit* kullanıyoruz.

class Logger {
public:
    static Logger& get_instance(); 

    void init(LogLevel level, const std::string& log_file_path = "", const std::string& log_source = "SYSTEM"); 
    
    void log(LogLevel level, const std::string& message, const char* file, int line);
    void log_error_to_cerr(LogLevel level, const std::string& message, const char* file, int line);


    Logger(const Logger&) = delete; 
    void operator=(const Logger&) = delete; 

    LogLevel get_level() const;
    std::string level_to_string(LogLevel level) const; 

    // YENİ: LogPanel'in QTextEdit'ini kaydetme metodu (QTextEdit* parametresi ile)
    void set_log_panel_text_edit(QTextEdit* textEdit);

private:
    Logger() : level_(LogLevel::INFO), m_guiLogTextEdit(nullptr), log_counter_(0), log_source_("SYSTEM") {} 
    ~Logger(); 

    LogLevel level_;
    std::ofstream file_stream_; 
    std::mutex mutex_; // Thread-safe dosya yazma ve sayıcı için

    // YENİ: LogPanel'deki QTextEdit'in pointer'ı
    QTextEdit* m_guiLogTextEdit;
    // YENİ: LogPanel ayarlanmadan önce logları tutmak için buffer
    std::vector<std::string> initial_log_buffer;
    // YENİ: Buffer ve m_guiLogTextEdit erişimini korumak için mutex
    std::mutex buffer_mutex; 

    unsigned long long log_counter_; 
    std::string log_source_; 
};

// LOG_INIT makrosu: Logger'ı başlangıçta yapılandırmak için
#define LOG_INIT(log_file_name, log_source_name) Logger::get_instance().init(LogLevel::INFO, log_file_name, log_source_name)

// LOG makroları (iç mantık Logger::log metodunda yönetilecek)
#define LOG(level, message) \
    do { \
        if (Logger::get_instance().get_level() >= level) { \
            std::stringstream ss_log_stream; \
            ss_log_stream << message; \
            Logger::get_instance().log(level, ss_log_stream.str(), __FILE__, __LINE__); \
        } \
    } while(0)

#define LOG_DEFAULT(level, message) \
    do { \
        if (Logger::get_instance().get_level() >= level) { \
            std::stringstream log_ss_internal; \
            log_ss_internal << message; \
            Logger::get_instance().log(level, log_ss_internal.str(), __FILE__, __LINE__); \
        } \
    } while(0)

#define LOG_ERROR_CERR(level, message) \
    do { \
        if (Logger::get_instance().get_level() >= level) { \
            std::stringstream log_ss_internal_err; \
            log_ss_internal_err << message; \
            Logger::get_instance().log_error_to_cerr(level, log_ss_internal_err.str(), __FILE__, __LINE__); \
        } \
    } while(0)

#endif // CEREBRUM_LUX_LOGGER_H