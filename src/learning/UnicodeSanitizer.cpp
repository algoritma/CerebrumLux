#include "UnicodeSanitizer.h"
#include <algorithm>
#include <cctype> // iscntrl için
#include "../core/logger.h" // LOG_DEFAULT için

namespace CerebrumLux { // Yeni eklendi

std::string UnicodeSanitizer::sanitize(const std::string& input) const {
    std::string sanitized_output;
    sanitized_output.reserve(input.length()); // Bellek ayırımı için optimize et

    for (char c : input) {
        // ASCII kontrol karakterlerini (tab, newline hariç) kaldır
        if (std::iscntrl(static_cast<unsigned char>(c)) && c != '\t' && c != '\n' && c != '\r') {
            // Kontrol karakterlerini atla veya yerine boşluk koyabiliriz.
            // Şimdilik atlıyoruz.
            // sanitized_output += ' ';
            continue;
        }
        // İkili (binary) verileri veya geçersiz UTF-8 karakterlerini daha gelişmiş bir mekanizma ile ele al.
        // Bu basit bir sanitizasyon, daha karmaşık durumlar için ek kütüphaneler (libicu gibi) gerekebilir.
        
        // Şimdilik sadece "güvenli" kabul edilen ASCII karakterlerini ve
        // geçerli UTF-8 başlangıç baytlarını geçirmeye çalışalım (basitçe).
        // Daha iyi bir UTF-8 validasyonu için tam bir parser gerekir.
        sanitized_output += c;
    }
    
    // Ardışık boşlukları tek boşluğa indir
    std::string final_output;
    bool last_was_space = false;
    for (char c : sanitized_output) {
        if (std::isspace(static_cast<unsigned char>(c))) {
            if (!last_was_space) {
                final_output += ' ';
                last_was_space = true;
            }
        } else {
            final_output += c;
            last_was_space = false;
        }
    }

    // Baştaki ve sondaki boşlukları kaldır
    size_t first = final_output.find_first_not_of(' ');
    size_t last = final_output.find_last_not_of(' ');
    if (std::string::npos == first) {
        return "";
    }
    std::string result = final_output.substr(first, (last - first + 1));

    if (result != input) {
        LOG_DEFAULT(LogLevel::DEBUG, "UnicodeSanitizer: İçerik sanitize edildi. Orijinalden farklı.");
    }
    return result;
}

} // namespace CerebrumLux // Yeni eklendi