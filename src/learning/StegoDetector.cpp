#include "StegoDetector.h"
#include "../core/logger.h" // Loglama için
#include <string> // std::string::npos için

bool StegoDetector::detectSteganography(const std::string& data) const {
    // Bu ilk versiyonda çok basit heuristikler kullanacağız.
    // Gerçek bir steganografi dedektörü, çok daha karmaşık algoritmalar ve istatistiksel analizler gerektirir.

    // Heuristik 1: Belirli anahtar kelimelerin varlığı
    if (data.find("hidden_message_tag") != std::string::npos ||
        data.find("stego_payload_marker") != std::string::npos ||
        data.find("encoded_data_start") != std::string::npos) {
        LOG_DEFAULT(LogLevel::WARNING, "StegoDetector: Detected suspicious keywords in data.");
        return true;
    }

    // Heuristik 2: Normalde beklenmeyen yüksek oranda görünmez karakterler veya boşluklar
    // Örneğin, ardışık çok sayıda boşluk veya non-printable karakter.
    size_t space_count = 0;
    size_t non_print_count = 0;
    for (char c : data) {
        if (c == ' ') {
            space_count++;
        } else if (!std::isprint(c) && c != '\n' && c != '\t') { // Yeni satır ve tab hariç
            non_print_count++;
        }
    }

    // Örneğin, %10'dan fazla boşluk veya %1'den fazla görünmez karakter
    if (data.length() > 0 && ( (float)space_count / data.length() > 0.10f || (float)non_print_count / data.length() > 0.01f ) ) {
        LOG_DEFAULT(LogLevel::WARNING, "StegoDetector: Detected unusual character distribution (high space/non-printable char count).");
        return true;
    }

    // Heuristik 3: Çok uzun ve anlamsız görünen veri blokları
    // Örneğin, belirli bir eşiğin üzerinde ardışık olmayan karakterler.
    // Bu, basit bir hash veya rastgelelik testi ile de yapılabilir.
    if (data.length() > 1000 && data.find("AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA") != std::string::npos) { // Rastgele bir desen
         LOG_DEFAULT(LogLevel::WARNING, "StegoDetector: Detected unusually long repeating pattern.");
        return true;
    }

    return false;
}