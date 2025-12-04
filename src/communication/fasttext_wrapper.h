#ifndef CEREBRUM_LUX_FASTTEXT_WRAPPER_H
#define CEREBRUM_LUX_FASTTEXT_WRAPPER_H

#include <string>
#include <memory>
#include <filesystem> // std::filesystem için
#include "../external/fasttext/include/fasttext.h" // FastText kütüphanesi için
#include "../core/logger.h" // LOG_DEFAULT, LOG_ERROR_CERR için
#include <algorithm> // std::transform için
#include <sstream>   // std::stringstream için

namespace CerebrumLux {

// FastText sınıflandırma sonucunu tutan yapı
struct FastTextResult {
    std::string label;
    double confidence;
    std::string shortAnswer; // İsteğe bağlı, önceden tanımlanmış kısa yanıt
};

// FastText modelini yükleyecek ve metin sınıflandırma yapacak sınıf
class FastTextWrapper {
public:
    // modelPath: FastText modelinin dosya yolu
    explicit FastTextWrapper(const std::string &modelPath);
    ~FastTextWrapper();

    // Metni sınıflandırır ve etiket + güven + isteğe bağlı kısa yanıt döndürür
    FastTextResult classify(const std::string &text) const;

    // FastText'in sınıflandırma için ihtiyaç duyduğu ön-işleme (küçük harf, noktalama kaldırma)
    std::string normalizeText(const std::string& text) const;

private:
    std::unique_ptr<fasttext::FastText> ft_model_; // FastText modelinin kendi instance'ı
    bool is_model_loaded_ = false; // Modelin yüklü olup olmadığını gösterir
};

} // namespace CerebrumLux

#endif // CEREBRUM_LUX_FASTTEXT_WRAPPER_H