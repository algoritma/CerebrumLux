#include "response_engine.h"
#include "../core/logger.h"
#include "../data_models/dynamic_sequence.h"
#include <sstream>
#include <iomanip>
#include <algorithm>

// Kurucu: yanıt şablonları burada tanımlanır
ResponseEngine::ResponseEngine(IntentAnalyzer& analyzer_ref, GoalManager& goal_manager_ref,
                               AIInsightsEngine& insights_engine_ref, NaturalLanguageProcessor* nlp_ptr)
    : analyzer(analyzer_ref), goal_manager(goal_manager_ref), insights_engine(insights_engine_ref),
      nlp(nlp_ptr), gen(rd()) {

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
}

// Yanıt üretme fonksiyonu
std::string ResponseEngine::generate_response(UserIntent current_intent, AbstractState current_abstract_state,
                                              AIGoal current_goal, const DynamicSequence& sequence) const {
    LOG_DEFAULT(LogLevel::DEBUG, "ResponseEngine::generate_response: Niyet=" << intent_to_string(current_intent) << 
                ", Durum=" << abstract_state_to_string(current_abstract_state) << ", Hedef=" << goal_to_string(current_goal));

    // AIInsightsEngine'den içgörüler al
    std::vector<std::string> insights_as_keywords;
    std::vector<AIInsight> insights_from_engine = insights_engine.generate_insights(sequence);
    for (const auto& insight : insights_from_engine) {
        insights_as_keywords.push_back(insight.observation);
    }

    // NLP ile yanıt üretimi
    std::string final_response_text;
    if (nlp) { // Null check
        final_response_text = nlp->generate_response_text(
            current_intent, current_abstract_state, current_goal, sequence, insights_as_keywords
        );
    }

    // Kritik eylem önerisi varsa onay sorusu ekle
    if (is_critical_action_suggestion(final_response_text)) {
        std::string action_description = extract_action_description(final_response_text);
        final_response_text = "Şu eylemi [" + action_description + "] yapmak istiyorum. Onaylıyor musunuz? (Evet/Hayır)";
        LOG_DEFAULT(LogLevel::DEBUG, "Kritik eylem onayı istendi: " << final_response_text);
        return final_response_text;
    }

    // Dinamik yer tutucular ve ek mesajlar
    if (!final_response_text.empty()) {
        size_t pos_ms = final_response_text.find("X ms");
        if (pos_ms != std::string::npos) {
            std::stringstream ss_ms;
            ss_ms << std::fixed << std::setprecision(0) << sequence.avg_keystroke_interval / 1000.0f;
            final_response_text.replace(pos_ms, 4, ss_ms.str() + " ms");
        }

        size_t pos_battery = final_response_text.find("pil durumu");
        if (pos_battery != std::string::npos) {
            std::stringstream ss_battery;
            ss_battery << (int)sequence.current_battery_percentage << "%";
            final_response_text.replace(pos_battery, 10, ss_battery.str());
        }
    }

    LOG_DEFAULT(LogLevel::DEBUG, "Oluşturulan yanıt: " << final_response_text);
    return final_response_text;
}
