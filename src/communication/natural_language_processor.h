#ifndef NATURAL_LANGUAGE_PROCESSOR_H
#define NATURAL_LANGUAGE_PROCESSOR_H

#include <string>
#include <vector>
#include <map>
#include <algorithm> // std::transform için
#include "../core/enums.h" // UserIntent, AbstractState, AIGoal için
#include "../data_models/dynamic_sequence.h" // DynamicSequence için
#include "../learning/KnowledgeBase.h" // YENİ: KnowledgeBase için
#include "../planning_execution/goal_manager.h" // GoalManager için (TAM TANIM İÇİN)
#include "../brain/autoencoder.h" // CryptofigAutoencoder için

namespace CerebrumLux { // NaturalLanguageProcessor sınıfı bu namespace içine alınacak

class NaturalLanguageProcessor {
public:
    explicit NaturalLanguageProcessor(CerebrumLux::GoalManager& goal_manager_ref, CerebrumLux::KnowledgeBase& kbRef); // YENİ: KnowledgeBase referansı eklendi

    CerebrumLux::UserIntent infer_intent_from_text(const std::string& user_input) const;
    CerebrumLux::AbstractState infer_state_from_text(const std::string& user_input) const;

    // NLP'nin yanıt üretimi için merkezi metot
    virtual std::string generate_response_text( // YENİ: virtual olarak işaretlendi
        CerebrumLux::UserIntent current_intent,
        CerebrumLux::AbstractState current_abstract_state,
        CerebrumLux::AIGoal current_goal,
        const CerebrumLux::DynamicSequence& sequence,
        const std::vector<std::string>& relevant_keywords,
        const CerebrumLux::KnowledgeBase& kb
    ) const;

    // NLP modelini güncelleme ve eğitme
    void update_model(const std::string& observed_text, CerebrumLux::UserIntent true_intent, const std::vector<float>& latent_cryptofig);
    void trainIncremental(const std::string& input, const std::string& expected_intent);
    void trainFromKnowledgeBase(const CerebrumLux::KnowledgeBase& kb); // YENİ: KnowledgeBase'den eğitim

    // Modeli yükleme ve kaydetme (placeholder)
    void load_model(const std::string& path);
    void save_model(const std::string& path) const;

private:
    CerebrumLux::GoalManager& goal_manager; // GoalManager referansı
    CerebrumLux::KnowledgeBase& kbRef_; // YENİ: KnowledgeBase referansı üyesi

    // Anahtar kelime ve niyet/durum eşleşmeleri
    std::map<CerebrumLux::UserIntent, std::vector<std::string>> intent_keyword_map;
    std::map<CerebrumLux::AbstractState, std::vector<std::string>> state_keyword_map;

    mutable std::map<CerebrumLux::UserIntent, std::deque<float>> implicit_feedback_history; // Implicit feedback history
    mutable std::map<CerebrumLux::UserIntent, std::deque<float>> explicit_feedback_history; // Explicit feedback history

    mutable std::map<CerebrumLux::UserIntent, std::vector<float>> intent_cryptofig_weights; // NLP'nin kendi dahili model ağırlıkları

    // Yardımcı fonksiyonlar
    float cryptofig_score_for_intent(CerebrumLux::UserIntent intent, const std::vector<float>& latent_cryptofig) const; // YENİ: public veya protected'a taşındı
    CerebrumLux::UserIntent rule_based_intent_guess(const std::string& lower_text) const; // YENİ: public veya protected'a taşındı
    CerebrumLux::AbstractState rule_based_state_guess(const std::string& lower_text) const; // YENİ: public veya protected'a taşındı
public: // YENİ: fallback_response_for_intent'i public yapıyoruz
    std::string fallback_response_for_intent(CerebrumLux::UserIntent intent, CerebrumLux::AbstractState state, const CerebrumLux::DynamicSequence& sequence) const;
};

} // namespace CerebrumLux

#endif // NATURAL_LANGUAGE_PROCESSOR_H