#include "fasttext_wrapper.h"
#include <iostream>
#include <algorithm> // std::transform için
#include <sstream>   // std::stringstream için
#include <cctype>    // std::tolower, std::ispunct için

namespace CerebrumLux {

// ----------------------------------------------------
// FastTextWrapper Implementasyonu
// ----------------------------------------------------

FastTextWrapper::FastTextWrapper(const std::string &modelPath) {
    ft_model_ = std::make_unique<fasttext::FastText>();
    try {
        if (std::filesystem::exists(modelPath) && std::filesystem::file_size(modelPath) > 0) {
            ft_model_->loadModel(modelPath);
            is_model_loaded_ = true;
            LOG_DEFAULT(LogLevel::INFO, "FastTextWrapper: Model başarıyla yüklendi: " << modelPath);
        } else {
            LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "FastTextWrapper: Model bulunamadi veya boş: " << modelPath);
        }
    } catch (const std::exception& e) {
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "FastTextWrapper: Model yüklenirken hata: " << e.what());
    }
}

FastTextWrapper::~FastTextWrapper() {
    LOG_DEFAULT(LogLevel::INFO, "FastTextWrapper: Model boşaltılıyor.");
}

FastTextResult FastTextWrapper::classify(const std::string &text) const {
    FastTextResult r;
    r.label = "unknown";
    r.confidence = 0.0;
    
    if (!is_model_loaded_ || text.empty()) {
        LOG_DEFAULT(LogLevel::WARNING, "FastTextWrapper: Model yüklü değil veya metin boş. Sınıflandırma yapılamadı.");
        return r;
    }

    std::string normalized_text = normalizeText(text);
    if (normalized_text.empty()) {
        return r;
    }

    // FastText'in predict metodunu kullan
    std::vector<std::pair<float, std::string>> predictions;
    const int32_t k = 1; // En iyi 1 tahmin
    const float threshold = 0.0f; // Tüm tahminleri al (tüm tahminleri alıp sonra filtrelemek yerine sadece k tanesini alıyoruz)
    
    std::stringstream ss(normalized_text);
    ft_model_->predictLine(ss, predictions, k, threshold);

    if (!predictions.empty()) {
        r.label = predictions[0].second; // '__label__<etiket>' formatında gelir
        // "__label__" kısmını temizle
        if (r.label.rfind("__label__", 0) == 0) {
            r.label = r.label.substr(9);
        }
        r.confidence = static_cast<double>(predictions[0].first);
    }
    LOG_DEFAULT(LogLevel::TRACE, "FastTextWrapper: '" << text.substr(0, std::min((size_t)20, text.length())) << "...' için FastText sınıflandırma sonucu: Label=" << r.label << ", Confidence=" << r.confidence);
    return r;
}

std::string FastTextWrapper::normalizeText(const std::string& text) const {
    std::string normalized = text;
    // Küçük harfe çevir
    std::transform(normalized.begin(), normalized.end(), normalized.begin(),
                   [](unsigned char c){ return static_cast<unsigned char>(std::tolower(c)); });
    // Basit noktalama işaretlerini kaldırma
    normalized.erase(std::remove_if(normalized.begin(), normalized.end(), [](char c){ return std::ispunct(c); }), normalized.end());
    return normalized;
}

} // namespace CerebrumLux