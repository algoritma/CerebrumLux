#include "StegoDetector.h"
#include <cmath> // log2 için
#include <map>   // frekans sayımı için
#include "../../core/logger.h" // LOG_DEFAULT için

namespace CerebrumLux { // Yeni eklendi

bool StegoDetector::detectSteganography(const std::string& data) const {
    // Birden fazla heuristik kontrolü
    if (checkEntropy(data)) {
        LOG_DEFAULT(LogLevel::WARNING, "StegoDetector: Yüksek entropi tespit edildi.");
        return true;
    }
    if (checkKnownSignatures(data)) {
        LOG_DEFAULT(LogLevel::WARNING, "StegoDetector: Bilinen steganografi imzası tespit edildi.");
        return true;
    }
    // checkMetadata(data); // Metin verilerinde metadata pek olmaz, bu yüzden şimdilik devre dışı.
    
    // Daha fazla gelişmiş heuristik eklenebilir.
    return false;
}

bool StegoDetector::checkEntropy(const std::string& data) const {
    if (data.empty()) return false;

    std::map<char, int> frequencies;
    for (char c : data) {
        frequencies[c]++;
    }

    double entropy = 0.0;
    size_t data_len = data.length();
    for (auto const& [key, val] : frequencies) {
        double p = (double)val / data_len;
        entropy -= p * std::log2(p);
    }

    // Eşik değeri belirle. Yüksek entropi, rastgele veri veya şifreli veri işareti olabilir.
    // Metin için tipik entropi değerleri daha düşüktür.
    // Bu değer, testlerle optimize edilmelidir.
    double threshold = 7.0; // Örnek bir eşik değeri (maksimum 8, ASCII karakterler için)
    if (entropy > threshold) {
        LOG_DEFAULT(LogLevel::DEBUG, "StegoDetector: Entropi değeri yüksek: " << entropy);
        return true;
    }
    return false;
}

bool StegoDetector::checkMetadata(const std::string& data) const {
    // Metin tabanlı verilerde bu çok geçerli olmayabilir.
    // Resim, ses veya video gibi dosya türleri için daha uygun bir kontroldür.
    // Ancak, metin içine gömülü (örneğin Base64 ile encode edilmiş) dosya verileri varsa,
    // o dosyaların metadata'ları kontrol edilebilir.
    
    // Şimdilik sadece basit bir "hidden_message_tag" arayalım.
    if (data.find("hidden_message_tag") != std::string::npos) {
        LOG_DEFAULT(LogLevel::DEBUG, "StegoDetector: Metin içinde 'hidden_message_tag' bulundu.");
        return true;
    }
    return false;
}

bool StegoDetector::checkKnownSignatures(const std::string& data) const {
    // Bilinen steganografi araçlarının veya tekniklerinin imzalarını arayın.
    // Bu, genellikle belirli byte dizilerini veya metin kalıplarını içerir.
    // Örneğin, LSB steganografi için belirli bir piksel deseni yerine,
    // metin tabanlı steganografi için belirli dilbilgisel anormallikler veya
    // sıkıştırılmış/şifrelenmiş veri blokları olabilir.

    // Basit bir örnek: belirli bir "stego marker" arayalım.
    if (data.find("STEGO_START_MARKER_XYZ") != std::string::npos ||
        data.find("ST3G0_END_MARKER_ABC") != std::string::npos) {
        LOG_DEFAULT(LogLevel::DEBUG, "StegoDetector: Bilinen stego marker tespit edildi.");
        return true;
    }

    // Daha gelişmiş imzalar için YARA kuralları veya regex kullanılabilir.
    return false;
}

} // namespace CerebrumLux // Yeni eklendi