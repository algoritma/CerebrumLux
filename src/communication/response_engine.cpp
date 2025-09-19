#include "response_engine.h"
#include "../core/logger.h"
#include "../core/utils.h" // YENİ: SafeRNG için dahil edildi (action_to_string vb. için de gerekli)
#include "../data_models/dynamic_sequence.h"
#include "../brain/intent_analyzer.h"
#include "../planning_execution/goal_manager.h"
#include "ai_insights_engine.h"
#include "natural_language_processor.h"
#include <sstream>
#include <iomanip>
#include <algorithm>
// #include <random> // KALDIRILDI, artık utils.h'den SafeRNG kullanılıyor


// === Yardımcı fonksiyonlar (Header dosyasında tanımlı olduğu için burada tekrar tanımlanmayacak) ===
// static bool is_critical_action_suggestion(const std::string& suggestion);
// static std::string extract_action_description(const std::string& full_suggestion);


// Kurucu: yanıt şablonları burada tanımlanır
ResponseEngine::ResponseEngine(IntentAnalyzer& analyzer_ref, GoalManager& goal_manager_ref,
                               AIInsightsEngine& insights_engine_ref, NaturalLanguageProcessor* nlp_ptr)
    : analyzer(analyzer_ref), goal_manager(goal_manager_ref), insights_engine(insights_engine_ref),
      nlp(nlp_ptr) {

    // Örnek yanıt şablonları
    response_templates[UserIntent::FastTyping][AbstractState::HighProductivity].responses = {
        "Harika bir hizla yaziyorsunuz! Odaklanmaya devam edin.",
        "Üretkenliğiniz zirvede, devam edin!",
        "Yazım akıcılığınız etkileyici. Şu anki akışı bozmayalım."
    };

    response_templates[UserIntent::Editing][AbstractState::Focused].responses = {
        "Düzenleme modundasınız ve odaklanmışsınız. Yardıma ihtiyacınız var mı?",
        "Metninizi titizlikle düzenliyorsunuz, takdire şayan."
    };

    response_templates[UserIntent::None][AbstractState::None].responses = {
        "Size yardımcı olmak için buradayım.",
        "Nasıl bir destek istersiniz?"
    };

    // ... Diğer yanıt şablonları da benzer şekilde eklenecek ...
    // NOTE: Bu şablonlar şu anki kodda eksik, ancak derleme hatası vermez.
    // Projenin genel kodunda bu şablonların tam olarak tanımlı olduğu varsayılmaktadır.

    response_templates[UserIntent::FastTyping][AbstractState::NormalOperation].responses = {
        "Hizli yazim modunda gibisiniz. Daha verimli olmak için önerilerim olabilir mi? ",
        "Hizli yaziminizda dikkat dagiticilari azaltmak ister misiniz? "
    };
    response_templates[UserIntent::Unknown][AbstractState::None].responses = {
        "Mevcut niyetiniz belirsiz. Daha fazla veri toplanarak öğrenmeye devam ediyorum. ",
        "Davranışınızı anlamaya çalışıyorum. Lütfen etkileşiminize devam edin. "
    };
    response_templates[UserIntent::Unknown][AbstractState::LowProductivity].responses = {
        "Şu anki aktiviteniz düşük ve niyetiniz belirsiz. Size nasıl yardımcı olabilirim? ",
        "Odaklanmanızı artırmak için bildirimleri sessize alabilirim. "
    };
    response_templates[UserIntent::Unknown][AbstractState::Distracted].responses = {
        "Dikkatinizin dağıldığını algıladım. Odaklanmanıza yardımcı olacak bir şey yapabilirim? ",
        "Çok fazla dikkat dağıtıcı var gibi görünüyor. Bildirimleri sessize alalım mı? "
    };
    response_templates[UserIntent::None][AbstractState::FaultyHardware].responses = {
        "Sistem performansınız düşük görünüyor. Optimizasyon yapmamı ister misiniz?",
        "Uygulamalarınız yavaş mı çalışıyor? Arka plan işlemlerini kontrol edebilirim."
    };
    response_templates[UserIntent::Programming][AbstractState::Focused].responses = {
        "Programlama modundasınız ve odaklanmışsınız. Akışınızı bozmayalım. ",
        "Kod yazımınızda bir yardıma ihtiyacınız var mı? ",
        "Hata ayıklama yapıyor gibisiniz, bu konuda bir ipucu ister misiniz? "
    };
    response_templates[UserIntent::Programming][AbstractState::NormalOperation].responses = {
        "Programlama ile uğraşıyorsunuz. Hızlıca bir doküman açmamı ister misiniz? "
    };
    response_templates[UserIntent::Programming][AbstractState::Debugging].responses = {
        "Yoğun bir hata ayıklama sürecindesiniz. Kodunuzun geçmişini kontrol edelim mi? ",
        "Bu sorun üzerinde ne kadar zamandır çalışıyorsunuz? Belki kısa bir mola iyi gelir."
    };
    response_templates[UserIntent::Gaming][AbstractState::Focused].responses = {
        "Oyun deneyiminiz zirvede! İyi eğlenceler. ",
        "Oyun oynarken tüm bildirimleri sessize almamı ister misiniz? "
    };
    response_templates[UserIntent::Gaming][AbstractState::Distracted].responses = {
        "Oyun sırasında dikkatiniz dağılıyor gibi. Sistem performansını optimize edelim mi? ",
        "Arka plan uygulamalarını kapatarak daha akıcı bir deneyim sağlayabilirim. "
    };
    response_templates[UserIntent::MediaConsumption][AbstractState::PassiveConsumption].responses = {
        "Pasif medya tüketiyorsunuz. Keyfinizi bozmayalım. ",
        "Ekranı daha da karartarak pil tasarrufu sağlayabilirim. "
    };
    response_templates[UserIntent::MediaConsumption][AbstractState::NormalOperation].responses = {
        "Medya tüketiyorsunuz. Ses seviyesini ayarlayabilirim. "
    };
    response_templates[UserIntent::CreativeWork][AbstractState::CreativeFlow].responses = {
        "Yaratıcı akış modundasınız! Size ilham verecek başka bir şey yapabilirim? ",
        "Yaratıcılığınızı kesintiye uğratmamak için dikkat dağıtıcıları engelleyelim. "
    };
    response_templates[UserIntent::CreativeWork][AbstractState::NormalOperation].responses = {
        "Yaratıcı bir işle uğraşıyorsunuz. Sizin için bir hatırlatıcı kurabilirim. "
    };
    response_templates[UserIntent::Research][AbstractState::SeekingInformation].responses = {
        "Yoğun bir bilgi arayışındasınız. İlgili dokümanları açmamı ister misiniz? ",
        "Odak modunu etkinleştirerek araştırmanıza daha iyi odaklanabilirsiniz. "
    };
    response_templates[UserIntent::Communication][AbstractState::SocialInteraction].responses = {
        "Sosyal etkileşim halindesiniz. Hızlı ve kesintisiz iletişim için buradayım. ",
        "Yazım denetimini hızlandırmak ister misiniz? "
    };
    response_templates[UserIntent::Communication][AbstractState::NormalOperation].responses = {
        "İletişim kuruyorsunuz. E-posta veya mesajlaşma uygulamasını hızlıca açabilirim. "
    };
}

std::string ResponseEngine::generate_response(UserIntent current_intent, AbstractState current_abstract_state, AIGoal current_goal, const DynamicSequence& sequence) const {
    LOG_DEFAULT(LogLevel::DEBUG, "ResponseEngine::generate_response: Niyet=" << intent_to_string(current_intent) << 
                ", Durum=" << abstract_state_to_string(current_abstract_state) << ", Hedef=" << goal_to_string(current_goal) << "\n"); 

    std::vector<std::string> insights_as_keywords; 
    std::vector<AIInsight> insights_from_engine = insights_engine.generate_insights(sequence);
    for (const auto& insight : insights_from_engine) {
        insights_as_keywords.push_back(insight.observation); 
    }

    // NLP ile yanıt üretimi
    std::string final_response_text;
    if (nlp) { // Null check
        final_response_text = nlp->generate_response_text( 
            current_intent,
            current_abstract_state,
            current_goal,
            sequence,
            insights_as_keywords 
        );
    } else {
        LOG_DEFAULT(LogLevel::ERR_CRITICAL, "ResponseEngine::generate_response: NaturalLanguageProcessor pointer null! Genel fallback yan─▒ta d├╝┼iliyor.\n"); // DÜZELTİLDİ: LogLevel::ERROR yerine ERR_CRITICAL
        // NLP yoksa veya null ise, eski kural tabanlı yanıtlardan birini seç
        if (response_templates.count(current_intent) && response_templates.at(current_intent).count(current_abstract_state)) {
            // Hata düzeltildi: current_abstract_state kullanıldı
            std::uniform_int_distribution<> distrib(0, response_templates.at(current_intent).at(current_abstract_state).responses.size() - 1); 
            final_response_text = response_templates.at(current_intent).at(current_abstract_state).responses[distrib(SafeRNG::get_instance().get_generator())]; // SafeRNG kullanıldı
        } else if (response_templates.count(UserIntent::None) && response_templates.at(UserIntent::None).count(AbstractState::None)) {
            std::uniform_int_distribution<> distrib(0, response_templates.at(UserIntent::None).at(AbstractState::None).responses.size() - 1);
            final_response_text = response_templates.at(UserIntent::None).at(AbstractState::None).responses[distrib(SafeRNG::get_instance().get_generator())]; // SafeRNG kullanıldı
        } else {
            final_response_text = "NLP ve şablonlar kullanılamadığı için yardımcı olamıyorum.";
        }
    }


    // Kritik eylem önerisi varsa onay sorusu ekle
    if (is_critical_action_suggestion(final_response_text)) {
        std::string action_description = extract_action_description(final_response_text); 
        final_response_text = "Şu eylemi [" + action_description + "] yapmak istiyorum. Onaylıyor musunuz? (Evet/Hayır)";
        LOG_DEFAULT(LogLevel::DEBUG, "Kritik eylem onayı istendi: " << final_response_text << "\n");
        return final_response_text;
    }

    // Dinamik yer tutucuları ve ek mesajlar
    if (!final_response_text.empty()) {
        // Replace 'X ms'
        size_t pos_ms = final_response_text.find("X ms");
        if (pos_ms != std::string::npos) {
            std::stringstream ss_ms;
            ss_ms << std::fixed << std::setprecision(0) << sequence.avg_keystroke_interval / 1000.0f; 
            final_response_text.replace(pos_ms, 4, ss_ms.str() + " ms");
        }

        // Replace 'pil durumu'
        size_t pos_battery = final_response_text.find("pil durumu");
        if (pos_battery != std::string::npos) {
            std::stringstream ss_battery;
            ss_battery << (int)sequence.current_battery_percentage << "%";
            final_response_text.replace(pos_battery, 10, ss_battery.str());
        }

        // Add intent-specific concluding remarks (if not already handled by more specific responses)
        // Bu kısım artık NLP içinde daha iyi ele alınabilir, ancak basit bir fallback olarak burada durabilir.
        if (current_intent == UserIntent::FastTyping && !sequence.statistical_features_vector.empty() && sequence.statistical_features_vector[2] > 0.95f && final_response_text.find("Harika bir odaklanmayla") == std::string::npos) { 
            final_response_text += " Harika bir odaklanmayla çalışıyorsunuz.";
        } else if (current_intent == UserIntent::Editing && !sequence.statistical_features_vector.empty() && sequence.statistical_features_vector[3] > 0.40f && final_response_text.find("Gelişmiş düzenleme yetenekleriniz") == std::string::npos) { 
            final_response_text += " Gelişmiş düzenleme yetenekleriniz etkileyici.";
        }
    }

    LOG_DEFAULT(LogLevel::DEBUG, "ResponseEngine::generate_response: Oluşturulan yanıt (NLP destekli): " << final_response_text << "\n");
    return final_response_text; 
}