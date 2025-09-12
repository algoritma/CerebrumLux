#ifndef CEREBRUM_LUX_UTILS_H
#define CEREBRUM_LUX_UTILS_H

#include <string>    // For std::wstring
#include <sstream> // For std::wstringstream
#include <chrono>  // For std::chrono
#include <iomanip> // For std::put_time
#include <ctime>   // For std::localtime
#include <cstdio>  // For _CRT_WIDE
#include <cerrno>  // For errno
#include <fstream> // For std::wofstream

#ifdef _WIN32
#include <stringapiset.h> // For _CRT_WIDE (Windows specific)
#endif

#include "enums.h" // Enum'lar için

// Bu dosyada tüm genel yardımcı fonksiyon bildirimleri yer alacak:
// log_level_to_string, get_current_timestamp_wstr, hash_string, LOG_MESSAGE makrosu
//İçerik Notu: cerebrum_lux_core.h ve cerebrum_lux_core.cpp dosyalarındaki g_current_log_level, g_log_file_stream tanımlarını, log_level_to_string, get_current_timestamp_wstr, hash_string fonksiyonlarının bildirimlerini ve LOG_MESSAGE makrosunu buraya taşıyın. Global değişkenlerin tanımları utils.cpp'ye gidecek.

extern LogLevel g_current_log_level; // Global raporlama seviyesi değişkeni (dışarıdan erişilebilir olmalı)
extern std::wofstream g_log_file_stream; // Global log dosyası akışı (main.cpp'de açılacak)

// Yardımcı fonksiyon bildirimleri
std::wstring log_level_to_string(LogLevel level);
std::wstring get_current_timestamp_wstr();
std::wstring intent_to_string(UserIntent intent); // UserIntent'ı string'e çevirme
std::wstring abstract_state_to_string(AbstractState state); // AbstractState'i string'e çevirme
unsigned short hash_string(const std::wstring& s);

// LOG_MESSAGE makrosu (cerebrum_lux_core.h'den buraya taşındı)
#define LOG_MESSAGE(level, stream, message) \
    do { \
        if (g_current_log_level >= level) { \
            std::wstringstream log_ss_internal; \
            log_ss_internal << L"[" << log_level_to_string(level) << L"]"; \
            log_ss_internal << L"[" << get_current_timestamp_wstr() << L"]"; \
            log_ss_internal << L"[" << static_cast<const wchar_t*>(_CRT_WIDE(__FILE__)) << L":" << __LINE__ << L"] "; \
            log_ss_internal << message; \
            stream << log_ss_internal.str(); \
            if (g_log_file_stream.is_open()) { \
                g_log_file_stream << log_ss_internal.str(); \
                if (level == LogLevel::ERR_CRITICAL) { \
                    g_log_file_stream.flush(); \
                } \
            } \
        } \
    } while(0)
    
#endif // CEREBRUM_LUX_UTILS_H