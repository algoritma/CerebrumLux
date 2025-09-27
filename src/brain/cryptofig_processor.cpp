#include "cryptofig_processor.h"
#include "../core/logger.h" // LOG_DEFAULT için
#include "../core/enums.h" // LogLevel için
#include "../brain/autoencoder.h" // CryptofigAutoencoder::INPUT_DIM, LATENT_DIM için
#include "../core/utils.h" // SafeRNG için
#include <algorithm> // std::min için
#include <stdexcept> // std::runtime_error için


namespace CerebrumLux {

CryptofigProcessor::CryptofigProcessor(IntentAnalyzer& analyzer_ref, CryptofigAutoencoder& autoencoder_ref)
    : intent_analyzer(analyzer_ref), cryptofig_autoencoder(autoencoder_ref)
{
    LOG_DEFAULT(LogLevel::INFO, "CryptofigProcessor: Initialized.");
}

std::vector<float> CryptofigProcessor::process_atomic_signal(const AtomicSignal& signal) {
    // Bu metod şu an için kullanılmıyor, ancak gelecekte tekil sinyalleri işlemek için eklenebilir.
    // Şimdilik dummy bir vektör döndürüyoruz.
    LOG_DEFAULT(LogLevel::DEBUG, "CryptofigProcessor::process_atomic_signal: Dummy implementasyon çağrıldı.");
    return {SafeRNG::get_instance().get_float(0.0f, 1.0f), SafeRNG::get_instance().get_float(0.0f, 1.0f), SafeRNG::get_instance().get_float(0.0f, 1.0f)};
}

void CryptofigProcessor::process_sequence(DynamicSequence& sequence, float autoencoder_learning_rate) {
    LOG_DEFAULT(LogLevel::DEBUG, "CryptofigProcessor::process_sequence: Latent kriptofig Autoencoder ile olusturuluyor ve ogrenme tetikleniyor.");

    if (sequence.statistical_features_vector.empty() || sequence.statistical_features_vector.size() != CryptofigAutoencoder::INPUT_DIM) {
        LOG_DEFAULT(LogLevel::WARNING, "CryptofigProcessor::process_sequence: DynamicSequence.statistical_features_vector boş veya boyut uyuşmuyor. Autoencoder işlemi atlanıyor.");
        sequence.latent_cryptofig_vector.clear(); // Vektörü temizle
        return;
    }

    try {
        // Statistical features'ı autoencoder ile latent kriptofige dönüştür
        std::vector<float> latent_features = cryptofig_autoencoder.encode(sequence.statistical_features_vector);
        
        // latent_features'ı DynamicSequence'e kaydet
        sequence.latent_cryptofig_vector = latent_features; // BU SATIR EKLENDİ/DÜZELTİLDİ

        // Autoencoder'ın ağırlıklarını hataya göre ayarla (öğrenme adımı)
        std::vector<float> reconstructed_features = cryptofig_autoencoder.decode(latent_features);
        float reconstruction_error = cryptofig_autoencoder.calculate_reconstruction_error(sequence.statistical_features_vector, reconstructed_features);
        cryptofig_autoencoder.adjust_weights_on_error(sequence.statistical_features_vector, autoencoder_learning_rate);
        
        LOG_DEFAULT(LogLevel::DEBUG, "CryptofigAutoencoder: Agirliklar hataya gore ayarlandi. Hata: " << reconstruction_error);
        LOG_DEFAULT(LogLevel::DEBUG, "CryptofigProcessor::process_sequence: Latent kriptofig olusturuldu ve Autoencoder ogrenme adimi tamamlandi.");

    } catch (const std::exception& e) {
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "CryptofigProcessor::process_sequence: Autoencoder işlemi sırasında hata: " << e.what());
        sequence.latent_cryptofig_vector.clear(); // Hata durumunda vektörü temizle
    } catch (...) {
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "CryptofigProcessor::process_sequence: Autoencoder işlemi sırasında bilinmeyen hata.");
        sequence.latent_cryptofig_vector.clear(); // Hata durumunda vektörü temizle
    }
}

void CryptofigProcessor::process_expert_cryptofig(const std::vector<float>& expert_cryptofig, IntentLearner& learner) {
    LOG_DEFAULT(LogLevel::INFO, "CryptofigProcessor::process_expert_cryptofig: Uzman kriptofig işleniyor (henüz implemente edilmedi).");
    // Gelecekte, uzman kriptofig'leri öğrenme modülüne yönlendirme mantığı eklenebilir.
}

const CryptofigAutoencoder& CryptofigProcessor::get_autoencoder() const {
    return cryptofig_autoencoder;
}

CryptofigAutoencoder& CryptofigProcessor::get_autoencoder() {
    return cryptofig_autoencoder;
}

void CryptofigProcessor::apply_cryptofig_for_learning(IntentLearner& learner, const std::vector<float>& received_cryptofig, CerebrumLux::UserIntent target_intent) const {
    LOG_DEFAULT(LogLevel::INFO, "CryptofigProcessor::apply_cryptofig_for_learning: Öğrenme için kriptofig uygulanıyor (henüz implemente edilmedi).");
    // Gelecekte, gelen kriptofiglerin öğrenme modülüne nasıl entegre edileceği mantığı eklenebilir.
}

} // namespace CerebrumLux