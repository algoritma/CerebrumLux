#ifndef CRYPTOFIG_PROCESSOR_H
#define CRYPTOFIG_PROCESSOR_H

#include <string>
#include <vector>

#include "intent_analyzer.h"
#include "autoencoder.h" // CryptofigAutoencoder için
#include "../sensors/atomic_signal.h" // DynamicSequence için
#include "../core/enums.h" // UserIntent için eklendi (TAM TANIM İÇİN)
#include "intent_learner.h" // IntentLearner için eklendi (ÇOK ÖNEMLİ)

// UserIntent için ileri bildirim yerine, tam tanım enums.h'den geliyor.
// Bu satırı tamamen kaldırıyoruz.
// namespace CerebrumLux { enum class UserIntent; } // BU SATIR KALDIRILDI

namespace CerebrumLux {

class CryptofigProcessor {
public:
    CryptofigProcessor(IntentAnalyzer& analyzer, CryptofigAutoencoder& autoencoder);

    std::vector<float> process_atomic_signal(const AtomicSignal& signal);
    void process_sequence(DynamicSequence& sequence, float autoencoder_learning_rate);
    void process_expert_cryptofig(const std::vector<float>& expert_cryptofig, IntentLearner& learner);

    // Get_autoencoder bildirimleri
    const CryptofigAutoencoder& get_autoencoder() const; // Const versiyon
    CryptofigAutoencoder& get_autoencoder(); // non-const versiyon

    void apply_cryptofig_for_learning(IntentLearner& learner, const std::vector<float>& received_cryptofig, CerebrumLux::UserIntent target_intent) const;

private:
    IntentAnalyzer& intent_analyzer;
    CryptofigAutoencoder& cryptofig_autoencoder;
};

} // namespace CerebrumLux

#endif // CRYPTOFIG_PROCESSOR_H