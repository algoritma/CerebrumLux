#ifndef NATURAL_LANGUAGE_PROCESSOR_H
#define NATURAL_LANGUAGE_PROCESSOR_H

#include <string>
#include <vector>
#include <map>
#include <memory> // std::unique_ptr için
#include <optional> // std::optional için
#include <atomic> // std::atomic için
#include <QObject> // EKLENDİ (NaturalLanguageProcessor bir QObject olacak)

#include "../core/enums.h" // Language enum'u ve diğer enum'lar için
#include "../gui/DataTypes.h" // ChatResponse için

// DÜZELTME: Incomplete type hatasını çözmek için tam tanımları dahil et
#include "../learning/KnowledgeBase.h"
#include "../learning/Capsule.h"

namespace CerebrumLux {
class GoalManager; // Forward declaration
struct DynamicSequence; // Forward declaration

class NaturalLanguageProcessor : public QObject { // QObject mirası eklendi
    Q_OBJECT 
 public:
    // Constructor güncellendi (QObject parent eklendi)
    explicit NaturalLanguageProcessor(CerebrumLux::GoalManager& goal_manager_ref, CerebrumLux::KnowledgeBase& kbRef, QObject* parent = nullptr);
    ~NaturalLanguageProcessor() override = default;

    CerebrumLux::UserIntent infer_intent_from_text(const std::string& user_input) const; // Placeholder, asenkron embedding ile çağrılacak
    CerebrumLux::AbstractState infer_state_from_text(const std::string& user_input) const; // Placeholder
    float cryptofig_score_for_intent(CerebrumLux::UserIntent intent, const std::vector<float>& latent_cryptofig) const; // Placeholder

    // Yeni generate_response_text imza (IntentRouter'a uyumlu)
    virtual ChatResponse generate_response_text(
        CerebrumLux::UserIntent current_intent,
        CerebrumLux::AbstractState current_abstract_state,
        CerebrumLux::AIGoal current_goal,
        const CerebrumLux::DynamicSequence& sequence,
        const std::vector<std::string>& relevant_keywords,
        const CerebrumLux::KnowledgeBase& kb,
        const std::vector<float>& user_embedding // YENİ: Embedding parametresi
    ) const = 0; // const anahtar kelimesi mevcut, saf sanal yapıldı

    // Model eğitimi ve yönetimi için metotlar
    void update_model(const std::string& observed_text, CerebrumLux::UserIntent true_intent, const std::vector<float>& latent_cryptofig);
    void trainIncremental(const std::string& input, const std::string& expected_intent);
    void trainFromKnowledgeBase(const CerebrumLux::KnowledgeBase& kb);
    void load_model(const std::string& path);
    void save_model(const std::string& path) const;

    // YENİ: Senkron embedding (SADECE ARKA PLAN THREAD'LERİNDEN ÇAĞRILMALI)
    virtual std::vector<float> generate_text_embedding_sync(
        const std::string& text,
        CerebrumLux::Language lang = CerebrumLux::Language::TR // Varsayılan dil eklendi
    ) const = 0; // const EKLENDİ ve saf sanal yapıldı

    // YENİ: Asenkron embedding (GUI THREAD'İNDEN ÇAĞRILMALI)
    void request_embedding_async(const std::string& text, const std::string& request_id);

    // YENİ: TeacherInvoker gibi iç sistemlerin basit istemler göndermesi için.
    virtual std::string generate_simple_response(const std::string& prompt) const;

signals: // Q_OBJECT olduğu için sinyaller burada
    void embeddingReady(const std::string& request_id, const std::vector<float>& embedding);

public:
    // YENİ: Metnin bilişsel zorluk derecesini hesaplar (0.0 - 1.0 arası)
    // 0.0: Çok basit (Refleks), 1.0: Çok karmaşık (Derin Düşünme)
    inline float calculate_cognitive_load(const std::string& text) const {
        // Bu fonksiyonun tanımı .cpp dosyasından buraya taşındı.
        float load = 0.0f;
        size_t length = text.length();
        if (length < 10) load += 0.1f;
        else if (length < 50) load += 0.3f;
        else load += 0.8f;
        std::string lower_text = text;
        std::transform(lower_text.begin(), lower_text.end(), lower_text.begin(), [](unsigned char c){ return static_cast<unsigned char>(std::tolower(c)); });
        if (lower_text.find("neden") != std::string::npos || lower_text.find("nasil") != std::string::npos || lower_text.find("açıkla") != std::string::npos || lower_text.find("analiz") != std::string::npos) {
            load += 0.5f;
        }
        if (lower_text == "selam" || lower_text == "merhaba" || lower_text == "naber") {
            load = 0.0f;
        }
        return std::min(1.0f, load);
    }

    // YENİ: Refleks yanıtı (Varsa döndürür, yoksa boş döner)
    inline std::string get_reflex_response(const std::string& text, const UserIntent& intent) const {
        // Bu fonksiyonun tanımı .cpp dosyasından buraya taşındı.
        std::string key = text;
        if (key.find("Selam") != std::string::npos || key.find("selam") != std::string::npos) return "Selam! Senin için ne yapabilirim?";
        if (key.find("Merhaba") != std::string::npos || key.find("merhaba") != std::string::npos) return "Merhaba! Cerebrum Lux sistemi hazır.";
        if (key.find("Nasılsın") != std::string::npos || key.find("nasılsın") != std::string::npos) return "Ben bir yapay zeka sistemiyim, dolayısıyla hislerim yok ama tüm sistemlerim %100 verimlilikle çalışıyor. Sen nasılsın?";
        if (key.find("Kimsin") != std::string::npos || key.find("kimsin") != std::string::npos) return "Ben Cerebrum Lux. Kişisel donanımlarda çalışmak üzere tasarlanmış, mahremiyet odaklı ve yüksek performanslı bir yapay zekayım.";
        if (key.find("Adın ne") != std::string::npos || key.find("adın ne") != std::string::npos || key.find("ismin ne") != std::string::npos) return "Adım Cerebrum Lux. 'Işık Beyin' anlamına gelir.";
        if (key.find("kapat") != std::string::npos || key.find("exit") != std::string::npos) return "Sistemi kapatma yetkim şu an simülasyon modunda. (Refleks Yanıtı)";
        return "";
    }

    // public yapıldı (önceki oturumda yapılmıştı)
    inline std::string fallback_response_for_intent(CerebrumLux::UserIntent intent, CerebrumLux::AbstractState state, const CerebrumLux::DynamicSequence& sequence) const;

    // YENİ: Embedding ile niyet çıkarımı için ayrı bir metot
    inline CerebrumLux::UserIntent infer_intent_from_text_with_embedding(const std::string& user_input, const std::vector<float>& embedding) const {
        std::string lower_text = user_input;
        std::transform(lower_text.begin(), lower_text.end(), lower_text.begin(),
                       [](unsigned char c){ return static_cast<unsigned char>(std::tolower(c)); });

        CerebrumLux::UserIntent guessed_intent = rule_based_intent_guess(lower_text);
        if (guessed_intent != CerebrumLux::UserIntent::Undefined) {
            return guessed_intent;
        }

        std::vector<CerebrumLux::Capsule> related_capsules = kbRef_.semantic_search(embedding, 2);
        if (!related_capsules.empty()) {
            for (const auto& capsule : related_capsules) {
                if (capsule.topic == "Programming") return CerebrumLux::UserIntent::Programming;
                if (capsule.topic == "Research" || capsule.topic == "WebSearch") return CerebrumLux::UserIntent::Research;
                if (capsule.topic == "AI Insight") return CerebrumLux::UserIntent::Question;
            }
        }
        return CerebrumLux::UserIntent::Undefined;
    }


protected:
    // YENİ EKLENEN METOD: Dinamik prompt oluşturma
    // Türetilmiş sınıfların (LLMProcessor gibi) erişebilmesi için protected yapıldı.
    std::string generate_dynamic_prompt(
        CerebrumLux::UserIntent intent,
        CerebrumLux::AbstractState state,
        CerebrumLux::AIGoal goal,
        const CerebrumLux::DynamicSequence& sequence,
        const std::string& user_input,
        const std::vector<CerebrumLux::Capsule>& relevant_capsules
    ) const;

private:
    CerebrumLux::GoalManager& goal_manager;
    CerebrumLux::KnowledgeBase& kbRef_; // KnowledgeBase referansı
    std::map<CerebrumLux::UserIntent, std::vector<std::string>> intent_keyword_map;
    std::map<CerebrumLux::AbstractState, std::vector<std::string>> state_keyword_map;
    std::map<CerebrumLux::UserIntent, std::vector<float>> intent_cryptofig_weights; // NLP modelinin dahili ağırlıkları

    CerebrumLux::UserIntent rule_based_intent_guess(const std::string& lower_text) const;
    CerebrumLux::AbstractState rule_based_state_guess(const std::string& lower_text) const;
};

// YENİ: Somut NLP Sınıfı
// Bu sınıf, NaturalLanguageProcessor arayüzünü Llama-2 motorunu kullanarak uygular.
class LLMProcessor : public NaturalLanguageProcessor {
    Q_OBJECT

public:
    explicit LLMProcessor(CerebrumLux::GoalManager& goal_manager_ref, CerebrumLux::KnowledgeBase& kbRef, QObject* parent = nullptr);
    ~LLMProcessor() override = default;
    ChatResponse generate_response_text(
        CerebrumLux::UserIntent current_intent,
        CerebrumLux::AbstractState current_abstract_state,
        CerebrumLux::AIGoal current_goal,
        const CerebrumLux::DynamicSequence& sequence,
        const std::vector<std::string>& relevant_keywords,
        const CerebrumLux::KnowledgeBase& kb,
        const std::vector<float>& user_embedding
    ) const override;

    std::vector<float> generate_text_embedding_sync(const std::string& text, CerebrumLux::Language lang) const override;

    // YENİ: TeacherInvoker gibi iç sistemlerin basit istemler göndermesi için.
    std::string generate_simple_response(const std::string& prompt) const override;
};

} // namespace CerebrumLux

#endif // NATURAL_LANGUAGE_PROCESSOR_H