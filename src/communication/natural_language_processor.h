// src/communication/NaturalLanguageProcessor.h - Değiştirilen kısım

#ifndef NATURAL_LANGUAGE_PROCESSOR_H
#define NATURAL_LANGUAGE_PROCESSOR_H

#include <string>
#include <vector>
#include <map>
#include <random> // SafeRNG için
#include <optional> // std::optional için

#include "../core/enums.h"
#include "../data_models/dynamic_sequence.h"
#include "../planning_execution/goal_manager.h" // GoalManager için
#include "../learning/KnowledgeBase.h" // KnowledgeBase için

namespace CerebrumLux {

// YENİ EKLENEN STRUCT: Chat yanıtını ve ek meta-bilgileri taşımak için
struct ChatResponse {
    std::string text;
    std::string reasoning; // Yanıtın nasıl üretildiğine dair açıklama
    bool needs_clarification = false; // Yanıtın belirsiz olup olmadığı ve kullanıcının onayına ihtiyaç duyup duymadığı
};


class NaturalLanguageProcessor { // NaturalLanguageProcessor sınıfı

private:
    static std::mt19937 s_rng; // Statik RNG motoru
    static std::uniform_real_distribution<float> s_dist; // Statik dağıtım

public:
    NaturalLanguageProcessor(CerebrumLux::GoalManager& goal_manager_ref, CerebrumLux::KnowledgeBase& kbRef);

    CerebrumLux::UserIntent infer_intent_from_text(const std::string& user_input) const;
    CerebrumLux::AbstractState infer_state_from_text(const std::string& user_input) const;
    float cryptofig_score_for_intent(CerebrumLux::UserIntent intent, const std::vector<float>& latent_cryptofig) const;

    // DEĞİŞTİRİLEN KOD: Dönüş tipi ChatResponse olarak değişti
    virtual ChatResponse generate_response_text( // 'virtual' anahtar kelimesi eklendi (eğer yoksa) ve dönüş tipi ChatResponse
        CerebrumLux::UserIntent current_intent,
        CerebrumLux::AbstractState current_abstract_state,
        CerebrumLux::AIGoal current_goal,
        const CerebrumLux::DynamicSequence& sequence,
        const std::vector<std::string>& relevant_keywords,
        const CerebrumLux::KnowledgeBase& kb
    ) const;

    void update_model(const std::string& observed_text, CerebrumLux::UserIntent true_intent, const std::vector<float>& latent_cryptofig);
    void trainIncremental(const std::string& input, const std::string& expected_intent);
    void trainFromKnowledgeBase(const CerebrumLux::KnowledgeBase& kb); // YENİ: KB'den eğitim
    void load_model(const std::string& path);
    void save_model(const std::string& path) const;

    // YENİ EKLENDİ: Metin girdisinden embedding hesaplama (STATİK ve placeholder) - 'const' kaldırıldı.
    static std::vector<float> generate_text_embedding(const std::string& text);

    // public yapıldı (önceki oturumda yapılmıştı)
    std::string fallback_response_for_intent(CerebrumLux::UserIntent intent, CerebrumLux::AbstractState state, const CerebrumLux::DynamicSequence& sequence) const;


private:
    CerebrumLux::GoalManager& goal_manager;
    CerebrumLux::KnowledgeBase& kbRef_; // KnowledgeBase referansı

    std::map<CerebrumLux::UserIntent, std::vector<std::string>> intent_keyword_map;
    std::map<CerebrumLux::AbstractState, std::vector<std::string>> state_keyword_map;
    std::map<CerebrumLux::UserIntent, std::vector<float>> intent_cryptofig_weights; // NLP modelinin dahili ağırlıkları

    CerebrumLux::UserIntent rule_based_intent_guess(const std::string& lower_text) const;
    CerebrumLux::AbstractState rule_based_state_guess(const std::string& lower_text) const;

    // YENİ EKLENEN METOD: Dinamik prompt oluşturma
    std::string generate_dynamic_prompt(
        CerebrumLux::UserIntent intent,
        CerebrumLux::AbstractState state,
        CerebrumLux::AIGoal goal,
        const CerebrumLux::DynamicSequence& sequence,
        const std::string& user_input,
        const std::vector<CerebrumLux::Capsule>& relevant_capsules
    ) const;
};

} // namespace CerebrumLux

#endif // NATURAL_LANGUAGE_PROCESSOR_H