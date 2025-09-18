#ifndef CEREBRUM_LUX_SUGGESTION_ENGINE_H
#define CEREBRUM_LUX_SUGGESTION_ENGINE_H

#include <string> // std::string için
#include <vector> // std::vector için
#include <map>    // std::map için
#include <random> // For std::random_device, std::mt19937, std::uniform_real_distribution
#include "../core/enums.h"         // Enumlar için
#include "../core/utils.h"         // For convert_wstring_to_string (if needed elsewhere)
#include "../data_models/dynamic_sequence.h" // DynamicSequence için ileri bildirim
#include "../brain/intent_analyzer.h" // IntentAnalyzer için ileri bildirim

// İleri bildirimler
struct DynamicSequence;
class IntentAnalyzer;

// *** StateKey: Q-Tablosu için durum temsilcisi ***
// UserIntent ve AbstractState'i birleştirerek bir "durum" tanımlar.
struct StateKey {
    UserIntent intent;
    AbstractState state;

    // std::map'te anahtar olarak kullanılabilmesi için karşılaştırma operatörü
    bool operator<(const StateKey& other) const {
        if (intent != other.intent) {
            return intent < other.intent;
        }
        return state < other.state;
    }
};

// *** SuggestionEngine: AI'ın kullaniciya eylem önerileri sunar ***
class SuggestionEngine {
public:
    SuggestionEngine(IntentAnalyzer& analyzer_ref);

    // Belirli bir niyet ve soyut durum için en uygun eylemi önerir (imza güncellendi)
    virtual AIAction suggest_action(UserIntent current_intent, AbstractState current_abstract_state, const DynamicSequence& sequence); // YENİ: virtual eklendi

    // AIAction enum değerini string'e çevirir (ARTIK BURADA DEĞİL, src/core/utils.h içinde global bir fonksiyon)
    // std::string action_to_string(AIAction action) const; // KALDIRILDI

    // YENİ: Pekiştirmeli öğrenme için Q-değerini günceller
    virtual void update_q_value(const StateKey& state, AIAction action, float reward); // YENİ: virtual eklendi

private:
    IntentAnalyzer& analyzer;

    // Pekiştirmeli öğrenme için Q-Tablosu
    std::map<StateKey, std::map<AIAction, float>> q_table;
    float learning_rate_rl = 0.1f; // Alfa
    float discount_factor_rl = 0.9f; // Gama
    float exploration_rate_rl = 0.1f; // Epsilon (epsilon-greedy için)
    mutable std::mt19937 gen; // Rastgele sayı üreteci
    mutable std::random_device rd; // Rastgele sayı üreteci için seed
};

#endif // CEREBRUM_LUX_SUGGESTION_ENGINE_H