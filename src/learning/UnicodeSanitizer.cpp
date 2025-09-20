#include "UnicodeSanitizer.h"
#include <algorithm> // std::remove_if için
#include <locale>    // iswcntrl gibi geniş karakter fonksiyonları için (gelecekte kullanılabilir)
#include "../core/logger.h" // Loglama için

std::string UnicodeSanitizer::sanitize(const std::string& text) const {
    std::string sanitized_text = text;

    // Basit bir temizleme: ASCII olmayan kontrol karakterlerini veya belirli özel karakterleri kaldır.
    // Gerçek bir Unicode sanitizer, çok daha karmaşık kurallara sahip olacaktır.
    // Şimdilik, sadece ASCII'de basılabilir olmayanları (boşluk hariç) kaldıralım.
    sanitized_text.erase(std::remove_if(sanitized_text.begin(), sanitized_text.end(), 
                                        [](unsigned char c){ return !std::isprint(c) && !std::isspace(c); }), 
                         sanitized_text.end());
    
    // Ayrıca, bilinen sorunlu veya kafa karıştırıcı Unicode karakterlerini dönüştürebilir veya kaldırabiliriz.
    // Örneğin, Zero Width Joiner (ZWJ) veya Zero Width Non-Joiner (ZWNJ) gibi karakterler.
    // Bu, std::string yerine std::u32string gerektirir ve daha karmaşık bir işlemdir.
    
    if (sanitized_text != text) {
        LOG_DEFAULT(LogLevel::INFO, "UnicodeSanitizer: Sanitized content. Original size: " << text.length() << ", New size: " << sanitized_text.length());
    }

    return sanitized_text;
}