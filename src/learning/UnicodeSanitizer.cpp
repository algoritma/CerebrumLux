#include "UnicodeSanitizer.h"
#include <algorithm> 
#include <locale>    
#include <codecvt>   // std::wstring_convert için
#include "../core/logger.h" 
#include <string_view> // YENİ: std::string_view için (Zero-width karakter kontrolü)

// Unicode karakterlerle daha iyi çalışmak için basit bir utf8 -> u32string -> utf8 dönüşümü
// C++17'de std::wstring_convert deprecated olmasına rağmen, genel uyumluluk için şimdilik kullanılabilir.
// Daha sağlam bir çözüm için https://github.com/lemire/FastWithd gibi kütüphaneler veya manuel UTF-8 ayrıştırma tercih edilebilir.
std::u32string utf8_to_u32(const std::string& utf8_str) {
    try {
        std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> converter;
        return converter.from_bytes(utf8_str);
    } catch (const std::range_error& e) {
        LOG_DEFAULT(LogLevel::WARNING, "UTF-8'den u32'ye dönüşümde hata: " << e.what());
        return {}; // Hata durumunda boş döndür
    }
}

std::string u32_to_utf8(const std::u32string& u32_str) {
    try {
        std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> converter;
        return converter.to_bytes(u32_str);
    } catch (const std::range_error& e) {
        LOG_DEFAULT(LogLevel::WARNING, "u32'den UTF-8'e dönüşümde hata: " << e.what());
        return ""; // Hata durumunda boş döndür
    }
}


std::string UnicodeSanitizer::sanitize(const std::string& text) const {
    std::string original_text = text;
    std::u32string u32_text = utf8_to_u32(original_text);

    if (u32_text.empty() && !original_text.empty()) { // Dönüşüm başarısız olduysa
        LOG_DEFAULT(LogLevel::WARNING, "UnicodeSanitizer: UTF-8'den u32'ye dönüşüm başarısız oldu. Geriye düşüş yapılıyor (sadece ASCII karakter kontrolü).");
        // Geriye dönük uyumluluk veya hata durumunda sadece ASCII kontrolü yap
        original_text.erase(std::remove_if(original_text.begin(), original_text.end(), 
                                        [](unsigned char c){ return !std::isprint(c) && !std::isspace(c); }), 
                         original_text.end());
        if (original_text != text) {
            LOG_DEFAULT(LogLevel::INFO, "UnicodeSanitizer: Fallback sanitization applied. Original size: " << text.length() << ", New size: " << original_text.length());
        }
        return original_text;
    }

    std::u32string sanitized_u32_text;
    sanitized_u32_text.reserve(u32_text.length());

    for (char32_t c : u32_text) {
        // Heuristik 1: Sıfır genişlikli karakterleri (Zero Width Joiner, Zero Width Space, vb.) kaldır
        // Unicode standardında "Format" kategori C (Control) veya "Separator, Space" Zs (Space separator) içinde yer alabilir.
        // Özellikle dikkat edilmesi gerekenler: U+200B (Zero Width Space), U+200C (Zero Width Non-Joiner), U+200D (Zero Width Joiner)
        // Ayrıca kontrol karakterleri (U+0000-U+001F, U+007F-U+009F gibi)
        if (c == U'\u200B' || c == U'\u200C' || c == U'\u200D' || 
            (c >= U'\u0000' && c <= U'\u001F') || // ASCII kontrol karakterleri
            (c >= U'\u007F' && c <= U'\u009F') || // C1 kontrol karakterleri
            c == U'\uFEFF') // Byte Order Mark (BOM)
        {
            // Bu karakterleri atla
            continue;
        }

        // Heuristik 2: Normalde metin içinde beklenmeyen, görsel olarak kafa karıştırıcı (confusable) karakterleri kaldır
        // Bu çok geniş bir kategori olup, kapsamlı bir listeye ihtiyaç duyar.
        // Şimdilik sadece birkaç örneği ele alalım:
        // Örneğin: Tam genişlikli Latin karakterler (normal ASCII Latin karakterleri gibi görünen ancak farklı genişlikte olanlar)
        // U+FF01-U+FF5E (Fullwidth ASCII variants)
        if ((c >= U'\uFF01' && c <= U'\uFF5E')) { 
            // Bu karakterleri normal ASCII karşılıklarına dönüştürme veya kaldırma seçeneği.
            // Şimdilik kaldırma.
            continue;
        }

        // Geçerli ve korunması gereken karakterleri ekle
        sanitized_u32_text.push_back(c);
    }

    std::string sanitized_text = u32_to_utf8(sanitized_u32_text);

    if (sanitized_text != original_text) {
        LOG_DEFAULT(LogLevel::INFO, "UnicodeSanitizer: Sanitized content. Original size: " << original_text.length() << ", New size: " << sanitized_text.length());
    }

    return sanitized_text;
}