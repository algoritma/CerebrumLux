#include "cryptofig_processor.h"
#include "../core/logger.h"
#include "../core/utils.h"
#include "../data_models/dynamic_sequence.h"
#include "intent_learner.h" // IntentLearner için
#include "intent_analyzer.h"
#include "autoencoder.h" // CryptofigAutoencoder tanımı için
#include <numeric>
#include <iostream>
#include <sstream>

namespace CerebrumLux {

// === CryptofigProcessor Implementasyonlari ===
// Kurucu
CerebrumLux::CryptofigProcessor::CryptofigProcessor(IntentAnalyzer& analyzer_ref, CryptofigAutoencoder& cryptofig_autoencoder_ref)
    : intent_analyzer(analyzer_ref),
      cryptofig_autoencoder(cryptofig_autoencoder_ref)
{
    LOG_DEFAULT(CerebrumLux::LogLevel::INFO, "CryptofigProcessor: Initialized.");
}

// process_sequence metodu
void CerebrumLux::CryptofigProcessor::process_sequence(DynamicSequence& sequence, float autoencoder_learning_rate) {
    LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "CryptofigProcessor::process_sequence: Latent kriptofig Autoencoder ile olusturuluyor ve ogrenme tetikleniyor.\n");
    if (sequence.statistical_features_vector.empty() || sequence.statistical_features_vector.size() != CerebrumLux::CryptofigAutoencoder::INPUT_DIM) {
        LOG_DEFAULT(CerebrumLux::LogLevel::ERR_CRITICAL, "CryptofigProcessor::process_sequence: DynamicSequence.statistical_features_vector boş veya boyut uyuşmuyor. Autoencoder işlemi atlanıyor.\n");
        sequence.latent_cryptofig_vector.assign(CerebrumLux::CryptofigAutoencoder::LATENT_DIM, 0.0f);
        return;
    }
    this->cryptofig_autoencoder.encode(sequence.statistical_features_vector);
    this->cryptofig_autoencoder.adjust_weights_on_error(sequence.statistical_features_vector, autoencoder_learning_rate);
    LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "CryptofigProcessor::process_sequence: Latent kriptofig olusturuldu ve Autoencoder ogrenme adimi tamamlandi.\n");
}

// apply_cryptofig_for_learning metodu
void CerebrumLux::CryptofigProcessor::apply_cryptofig_for_learning(IntentLearner& learner, const std::vector<float>& received_cryptofig, CerebrumLux::UserIntent target_intent) const {
    LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "CryptofigProcessor::apply_cryptofig_for_learning: Niyet " << CerebrumLux::intent_to_string(target_intent) << " için kriptofig ile öğrenme başlatıldı.\n");
    std::vector<float> current_weights = this->intent_analyzer.get_intent_weights(target_intent);
    if (current_weights.empty() || current_weights.size() != received_cryptofig.size()) {
        LOG_DEFAULT(CerebrumLux::LogLevel::ERR_CRITICAL, "apply_cryptofig_for_learning: Boyut uyuşmazlığı veya boş ağırlıklar. İlerleme durduruldu.\n");
        return;
    }

    float assimilation_rate = learner.get_learning_rate() * 5.0f;
    assimilation_rate = std::min(0.5f, assimilation_rate);

    for (size_t i = 0; i < current_weights.size(); ++i) {
        current_weights[i] += assimilation_rate * (received_cryptofig[i] - current_weights[i]);
        current_weights[i] = std::min(5.0f, std::max(-5.0f, current_weights[i]));
    }
    this->intent_analyzer.update_template_weights(target_intent, current_weights);

    LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "CryptofigProcessor::apply_cryptofig_for_learning: Öğrenme tamamlandı.\n");
}

// get_autoencoder() const metodu
const CerebrumLux::CryptofigAutoencoder& CerebrumLux::CryptofigProcessor::get_autoencoder() const {
    return this->cryptofig_autoencoder;
}

// get_autoencoder() non-const metodu
CerebrumLux::CryptofigAutoencoder& CerebrumLux::CryptofigProcessor::get_autoencoder() {
    return this->cryptofig_autoencoder;
}

// process_expert_cryptofig metodu
void CerebrumLux::CryptofigProcessor::process_expert_cryptofig(const std::vector<float>& expert_cryptofig, CerebrumLux::IntentLearner& learner) {
    LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "[CryptofigProcessor] Uzman kriptofigi işleniyor. Boyut: " << expert_cryptofig.size() << ". Bu, AI'nın meta-evrim sürecinin bir parçası olarak harici bilgi aktarımını temsil eder.\n");
}

} // namespace CerebrumLux