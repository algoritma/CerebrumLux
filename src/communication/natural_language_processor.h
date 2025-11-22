#ifndef NATURAL_LANGUAGE_PROCESSOR_H
#define NATURAL_LANGUAGE_PROCESSOR_H

#include <string>
#include <vector>
#include <map>
#include <random> // SafeRNG için
#include <memory> // std::unique_ptr için
#include <optional> // std::optional için

#include "../core/enums.h"
#include "../planning_execution/goal_manager.h" // GoalManager için
#include "../learning/KnowledgeBase.h" // KnowledgeBase için

#include "../external/fasttext/include/fasttext.h" // FastText kütüphanesini dahil et

namespace CerebrumLux {

// YENİ EKLENEN STRUCT: Chat yanıtını ve ek meta-bilgileri taşımak için
struct ChatResponse {
    std::string text;
    std::string reasoning; // Yanıtın nasıl üretildiğine dair açıklama
    bool needs_clarification = false; // Yanıtın belirsiz olup olmadığı ve kullanıcının onayına ihtiyaç duyup duymadığı
};

// YENİ: Desteklenen diller için enum
enum class Language {
    EN,
    DE,
    TR,
    UNKNOWN
};

// YENİ: Dil string'ini enum'a çeviren yardımcı fonksiyon
Language string_to_lang(const std::string& lang_str);


class NaturalLanguageProcessor { // NaturalLanguageProcessor sınıfı

private:
    // YENİ: Tüm FastText modelleri için statik map
    static std::map<Language, std::unique_ptr<fasttext::FastText>> s_fastTextModels; 
    static std::atomic<bool> s_isModelReady; // Modelin kullanıma hazır olup olmadığını kontrol eder

public:
    NaturalLanguageProcessor(CerebrumLux::GoalManager& goal_manager_ref, CerebrumLux::KnowledgeBase& kbRef);

    // YENİ: Tüm FastText modellerini yüklemek için statik fonksiyon
    static void load_fasttext_models();

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

    static std::vector<float> generate_text_embedding(const std::string& text, Language lang = Language::EN);

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