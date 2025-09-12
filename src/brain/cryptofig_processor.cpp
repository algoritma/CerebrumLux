#include "cryptofig_processor.h" // Kendi başlık dosyasını dahil et
#include "../core/utils.h"       // LOG_MESSAGE ve intent_to_string için
#include "../data_models/dynamic_sequence.h" // DynamicSequence için
#include "intent_learner.h"      // IntentLearner için
#include "intent_analyzer.h"     // IntentAnalyzer için
#include "autoencoder.h"         // CryptofigAutoencoder için
#include <numeric> // std::accumulate
#include <iostream>  // std::wcerr için

// === CryptofigProcessor Implementasyonlari ===
CryptofigProcessor::CryptofigProcessor(IntentAnalyzer& analyzer_ref, CryptofigAutoencoder& autoencoder_ref)
    : analyzer(analyzer_ref), autoencoder(autoencoder_ref) {}

// Bu metod DynamicSequence'i işleyerek hem statistical_features_vector'ı kullanacak hem de latent_cryptofig_vector'ı güncelleyecek.
void CryptofigProcessor::process_sequence(DynamicSequence& sequence, float autoencoder_learning_rate) {
    LOG_MESSAGE(LogLevel::DEBUG, std::wcerr, L"CryptofigProcessor::process_sequence: Latent kriptofig Autoencoder ile olusturuluyor ve ogrenme tetikleniyor.\n");
    if (sequence.statistical_features_vector.empty() || sequence.statistical_features_vector.size() != CryptofigAutoencoder::INPUT_DIM) {
        LOG_MESSAGE(LogLevel::ERR_CRITICAL, std::wcerr, L"CryptofigProcessor::process_sequence: DynamicSequence.statistical_features_vector boş veya boyut uyuşmuyor. Autoencoder işlemi atlanıyor.\n");
        sequence.latent_cryptofig_vector.assign(CryptofigAutoencoder::LATENT_DIM, 0.0f); // Latent vektörü sıfırla
        return;
    }
    sequence.latent_cryptofig_vector = autoencoder.encode(sequence.statistical_features_vector);
    autoencoder.adjust_weights_on_error(sequence.statistical_features_vector, autoencoder_learning_rate);
    LOG_MESSAGE(LogLevel::DEBUG, std::wcerr, L"CryptofigProcessor::process_sequence: Latent kriptofig olusturuldu ve Autoencoder ogrenme adimi tamamlandi.\n");
}


void CryptofigProcessor::apply_cryptofig_for_learning(IntentLearner& learner, const std::vector<float>& received_cryptofig, UserIntent target_intent) const {
    LOG_MESSAGE(LogLevel::DEBUG, std::wcerr, L"CryptofigProcessor::apply_cryptofig_for_learning: Niyet " << intent_to_string(target_intent) << L" için kriptofig ile öğrenme başlatıldı.\n");
    std::vector<float> current_weights = analyzer.get_intent_weights(target_intent);
    if (current_weights.empty() || current_weights.size() != received_cryptofig.size()) {
        LOG_MESSAGE(LogLevel::ERR_CRITICAL, std::wcerr, L"apply_cryptofig_for_learning: Boyut uyuşmazlığı veya boş ağırlıklar. İlerleme durduruldu.\n");
        return;
    }

    float assimilation_rate = learner.get_learning_rate() * 5.0f; 
    assimilation_rate = std::min(0.5f, assimilation_rate); 

    for (size_t i = 0; i < current_weights.size(); ++i) {
        current_weights[i] += assimilation_rate * (received_cryptofig[i] - current_weights[i]);
        current_weights[i] = std::min(5.0f, std::max(-5.0f, current_weights[i]));
    }
    analyzer.update_template_weights(target_intent, current_weights);
    
    LOG_MESSAGE(LogLevel::DEBUG, std::wcerr, L"CryptofigProcessor::apply_cryptofig_for_learning: Öğrenme tamamlandı.\n");
}

// Getter metodu tanımı
CryptofigAutoencoder& CryptofigProcessor::get_autoencoder() const { 
    return autoencoder; 
}