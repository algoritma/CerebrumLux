#ifndef CEREBRUM_LUX_CRYPTOFIG_PROCESSOR_H
#define CEREBRUM_LUX_CRYPTOFIG_PROCESSOR_H

#include <vector>
#include <string> // For wstring
#include "../core/enums.h"         // Enumlar için
#include "../core/utils.h"         // For convert_wstring_to_string (if needed elsewhere)
#include "../data_models/dynamic_sequence.h" // DynamicSequence için ileri bildirim
#include "intent_learner.h"        // IntentLearner için ileri bildirim
#include "intent_analyzer.h"       // IntentAnalyzer için ileri bildirim
#include "autoencoder.h"           // CryptofigAutoencoder için ileri bildirim


// İleri bildirimler
class IntentAnalyzer;
class IntentLearner;
class CryptofigAutoencoder;
struct DynamicSequence;

// *** CryptofigProcessor: Kriptofiglerin olusturulmasi ve islenmesi için ***
class CryptofigProcessor {
public:
    CryptofigProcessor(IntentAnalyzer& analyzer_ref, CryptofigAutoencoder& autoencoder_ref);

    // Bu metod DynamicSequence'i işleyerek hem statistical_features_vector'ı kullanacak hem de latent_cryptofig_vector'ı güncelleyecek.
    void process_sequence(DynamicSequence& sequence, float autoencoder_learning_rate); 

    void apply_cryptofig_for_learning(IntentLearner& learner, const std::vector<float>& received_cryptofig, UserIntent target_intent) const;
    //CryptofigAutoencoder& get_autoencoder() const { return autoencoder; } // Getter metodu
    virtual void process_expert_cryptofig(const std::vector<float>& expert_cryptofig, IntentLearner& learner); // YENİ EKLENDİ VE VIRTUAL
    virtual std::vector<float> generate_cryptofig_from_signals(const DynamicSequence& sequence); // YENİ EKLENDİ VE VIRTUAL
    virtual CryptofigAutoencoder& get_autoencoder(); //non-const versiyo
    virtual const CryptofigAutoencoder& get_autoencoder() const; // Getter metodu - Sadece prototip

private:
    IntentAnalyzer& analyzer;
    CryptofigAutoencoder& autoencoder; // Autoencoder referansı
};

#endif // CEREBRUM_LUX_CRYPTOFIG_PROCESSOR_H