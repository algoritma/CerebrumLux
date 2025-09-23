#include "StegoDetector.h"
#include "../core/logger.h" 
#include <string> 
#include <numeric>   // std::accumulate için
#include <cmath>     // std::log2 için
#include <map>       // Frekans analizi için
#include <string_view> // YENİ: std::string_view için (Zero-width karakter kontrolü)

bool StegoDetector::detectSteganography(const std::string& data) const {
    // Çok kısa metinlerde yanlış pozitifleri önlemek için minimum uzunluk kontrolü
    if (data.length() < 20) { // Örneğin 20 karakterden kısa metinleri analiz etmeyelim
        return false;
    }

    // Heuristik 1: Belirli anahtar kelimelerin varlığı (korunuyor)
    if (data.find("hidden_message_tag") != std::string::npos ||
        data.find("stego_payload_marker") != std::string::npos ||
        data.find("encoded_data_start") != std::string::npos ||
        data.find("execute_command") != std::string::npos || // Heuristik 4'ten buraya taşındı
        data.find("system_call") != std::string::npos ||    // Heuristik 4'ten buraya taşındı
        data.find("inject_code") != std::string::npos) {    // Heuristik 4'ten buraya taşındı
        LOG_DEFAULT(LogLevel::WARNING, "StegoDetector: Detected suspicious keywords in data.");
        return true;
    }

    // Heuristik 2: Normalde beklenmeyen yüksek oranda görünmez karakterler veya boşluklar (eğitimli olarak ayarlandı)
    size_t zero_width_char_count = 0;
    size_t non_printable_except_whitespace_count = 0;
    size_t total_chars_approx = data.length();

    const std::string_view ZERO_WIDTH_SPACE = "\xE2\x80\x8B"; // U+200B
    const std::string_view ZERO_WIDTH_NON_JOINER = "\xE2\x80\x8C"; // U+200C
    const std::string_view ZERO_WIDTH_JOINER = "\xE2\x80\x8D"; // U+200D
    const std::string_view BYTE_ORDER_MARK_UTF8 = "\xEF\xBB\xBF"; // U+FEFF

    size_t current_pos = 0;
    while ((current_pos = data.find(ZERO_WIDTH_SPACE.data(), current_pos, ZERO_WIDTH_SPACE.length())) != std::string::npos) {
        zero_width_char_count++;
        current_pos += ZERO_WIDTH_SPACE.length();
    }
    current_pos = 0; 
    while ((current_pos = data.find(ZERO_WIDTH_NON_JOINER.data(), current_pos, ZERO_WIDTH_NON_JOINER.length())) != std::string::npos) {
        zero_width_char_count++;
        current_pos += ZERO_WIDTH_NON_JOINER.length();
    }
    current_pos = 0;
    while ((current_pos = data.find(ZERO_WIDTH_JOINER.data(), current_pos, ZERO_WIDTH_JOINER.length())) != std::string::npos) {
        zero_width_char_count++;
        current_pos += ZERO_WIDTH_JOINER.length();
    }
    current_pos = 0;
    while ((current_pos = data.find(BYTE_ORDER_MARK_UTF8.data(), current_pos, BYTE_ORDER_MARK_UTF8.length())) != std::string::npos) {
        zero_width_char_count++;
        current_pos += BYTE_ORDER_MARK_UTF8.length();
    }

    // Basılabilir olmayan ASCII karakterler (0-31, 127) ve C1 kontrol karakterleri (128-159)
    for (char c : data) {
        if ((static_cast<unsigned char>(c) >= 0x00 && static_cast<unsigned char>(c) <= 0x1F) ||
            (static_cast<unsigned char>(c) >= 0x7F && static_cast<unsigned char>(c) <= 0x9F)) {
            non_printable_except_whitespace_count++;
        }
    }

    // Zero-width karakterlerin oranı için eşik artırıldı (daha az yanlış pozitif için)
    if (total_chars_approx > 0 && (float)zero_width_char_count / total_chars_approx > 0.01f) { // %1'den fazla
        LOG_DEFAULT(LogLevel::WARNING, "StegoDetector: Detected high ratio of zero-width characters (UTF-8).");
        return true;
    }

    // Yazdırılamayan karakterlerin oranı için eşik artırıldı
    if (total_chars_approx > 0 && (float)non_printable_except_whitespace_count / total_chars_approx > 0.02f) { // %2'den fazla
        LOG_DEFAULT(LogLevel::WARNING, "StegoDetector: Detected high ratio of non-printable ASCII/C1 control characters.");
        return true;
    }

    // Heuristik 3: Entropi kontrolü (eğitimli olarak ayarlandı)
    if (total_chars_approx > 50) { 
        std::map<char, int> char_counts;
        for (char c : data) {
            char_counts[c]++;
        }

        double entropy = 0.0;
        for (auto const& [key, val] : char_counts) {
            double probability = (double)val / total_chars_approx;
            entropy -= probability * std::log2(probability);
        }

        // Entropi eşik değerleri ayarlandı (daha az yanlış pozitif için)
        // Temiz metinlerde genellikle 4-5 civarı. Daha düşük (<1.5) veya daha yüksek (>7.0) şüpheli.
        if (entropy < 1.5 || entropy > 7.0) { 
            LOG_DEFAULT(LogLevel::WARNING, "StegoDetector: Detected unusual entropy (low or high). Entropy: " << entropy);
            return true;
        }
    }

    // Heuristik 4: Anahtar kelimeler ve anlamsal anormallikler (Heuristik 1 ile birleştirildi)
    // Bu kısım artık Heuristik 1'e dahil edilmiştir.

    return false;
}