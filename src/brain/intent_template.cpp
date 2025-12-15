#include "intent_template.h" // Kendi başlık dosyasını dahil et
#include "../core/enums.h"   // AIAction ve UserIntent enum'ları için


namespace CerebrumLux { 
// IntentTemplate sınıfının metot implementasyonları

float IntentTemplate::matchScore(const std::string& text) const {
    // Bu fonksiyonun implementasyonu, IntentTemplate sınıfının nasıl bir 'pattern' veya 'keywords' tuttuğuna bağlı olacaktır.
    // Şimdilik basit bir örnek: Eğer IntentTemplate'in bir 'pattern' üyesi varsa ve metin içinde bulunuyorsa 1.0 döndür.
    // Örneğin, eğer IntentTemplate sınıfında 'std::string pattern;' diye bir üye varsa:
    // if (text.find(pattern) != std::string::npos) return 1.0f;
    return 0.0f; // Varsayılan olarak 0.0 döndür
}

} // namespace CerebrumLux      
