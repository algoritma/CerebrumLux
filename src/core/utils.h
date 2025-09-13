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
#include <mutex>

#ifdef _WIN32
#include <stringapiset.h> // For _CRT_WIDE (Windows specific)
#endif

#include "enums.h" // Enum'lar için

//mesaj sistemi için
#include <queue>
#include <mutex>
#include <condition_variable>
#include "enums.h"

// Bu dosyada tüm genel yardımcı fonksiyon bildirimleri yer alacak:
// log_level_to_string, get_current_timestamp_wstr, hash_string, LOG_MESSAGE makrosu
//İçerik Notu: cerebrum_lux_core.h ve cerebrum_lux_core.cpp dosyalarındaki g_current_log_level, g_log_file_stream tanımlarını, log_level_to_string, get_current_timestamp_wstr, hash_string fonksiyonlarının bildirimlerini ve LOG_MESSAGE makrosunu buraya taşıyın. Global değişkenlerin tanımları utils.cpp'ye gidecek.


    
// Yardımcı fonksiyon bildirimleri
std::wstring log_level_to_string(LogLevel level);
std::wstring get_current_timestamp_wstr();
std::wstring intent_to_string(UserIntent intent); // UserIntent'ı string'e çevirme
std::wstring abstract_state_to_string(AbstractState state); // AbstractState'i string'e çevirme
unsigned short hash_string(const std::wstring& s);

class MessageQueue {
public:
    void enqueue(MessageData data);
    MessageData dequeue();
    bool isEmpty();

private:
    std::queue<MessageData> queue;
    std::mutex mutex;
    std::condition_variable cv;
};

#endif // CEREBRUM_LUX_UTILS_H