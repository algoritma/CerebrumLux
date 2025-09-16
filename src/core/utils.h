#ifndef CEREBRUM_LUX_UTILS_H
#define CEREBRUM_LUX_UTILS_H

#include <string>    // For std::string, std::wstring
#include <sstream> // For std::stringstream, std::wstringstream
#include <chrono>  // For std::chrono
#include <iomanip> // For std::put_time
#include <ctime>   // For std::localtime
#include <cstdio>  // For _CRT_WIDE
#include <cerrno>  // For errno
#include <fstream> // For std::ofstream (was std::wofstream, but now for general use)
#include <mutex>
#include <codecvt> // For std::codecvt_utf8
#include <locale>  // For std::locale, std::wstring_convert

#ifdef _WIN32
#include <stringapiset.h> // For _CRT_WIDE (Windows specific)
#endif

#include "enums.h" // Enum'lar için

//mesaj sistemi için
#include <queue>
#include <condition_variable>

// Helper function to convert wstring to string
std::string convert_wstring_to_string(const std::wstring& wstr);

// Yardımcı fonksiyon bildirimleri
std::string get_current_timestamp_str();
std::string intent_to_string(UserIntent intent); // UserIntent'ı string'e çevirme
std::string abstract_state_to_string(AbstractState state); // AbstractState'i string'e çevirme
std::string goal_to_string(AIGoal goal); // AIGoal'ı string'e çevirme
unsigned short hash_string(const std::string& s);

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