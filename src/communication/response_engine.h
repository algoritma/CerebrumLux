#ifndef CEREBRUM_LUX_RESPONSE_ENGINE_H
#define CEREBRUM_LUX_RESPONSE_ENGINE_H

#include <string> // std::string
#include <vector> // std::vector
#include <map>    // std::map
#include <random> // std::random_device, std::mt19937

#include "../core/enums.h" // UserIntent, AbstractState, AIGoal, AIAction, LogLevel
#include "../core/utils.h" // intent_to_string, abstract_state_to_string, action_to_string
#include "../data_models/dynamic_sequence.h"
#include "../brain/intent_analyzer.h"
#include "../planning_execution/goal_manager.h"
#include "ai_insights_engine.h"
#include "natural_language_processor.h"

// İleri bildirimler
struct DynamicSequence;
class IntentAnalyzer;
class GoalManager;
class AIInsightsEngine;
class NaturalLanguageProcessor;

// Yanıt şablonlarını tutan yapı
struct ResponseTemplate {
    std::vector<std::string> responses;
    float trigger_threshold = 0.0f;
};

// ResponseEngine sınıfı
class ResponseEngine {
public:
    // Kurucu: tüm bağımlılıklar referans olarak alınır
        ResponseEngine(IntentAnalyzer& analyzer_ref, GoalManager& goal_manager_ref,
                   AIInsightsEngine& insights_engine_ref, NaturalLanguageProcessor* nlp_ptr);

    // Kullanıcının niyeti, durumu ve hedefi ile birlikte dinamik yanıt üretir
    std::string generate_response(UserIntent current_intent, AbstractState current_abstract_state,
                                  AIGoal current_goal, const DynamicSequence& sequence) const;

private:
    IntentAnalyzer& analyzer;
    GoalManager& goal_manager;
    AIInsightsEngine& insights_engine;
    NaturalLanguageProcessor* nlp;

    // Niyet ve durum kombinasyonlarına göre yanıt şablonları
    std::map<UserIntent, std::map<AbstractState, ResponseTemplate>> response_templates;

    mutable std::mt19937 gen; // Rastgele sayı üreteci
    mutable std::random_device rd; 
};

// === Yardımcı fonksiyonlar ===
// Kritik eylem önerisi içerip içermediğini kontrol eder
static bool is_critical_action_suggestion(const std::string& suggestion) {
    const std::vector<std::string> critical_keywords = {
        "optimize etmemi ister misiniz?", "Uygulamaları kapatmamı ister misiniz?",
        "Gerekli olmayan bağlantıları kesmemi ister misiniz?", "arka plan süreçlerini optimize edebilirim.",
        "gereksiz sekmeleri kapatarak", "Arka plan uygulamalarını kapatarak",
        "Sistem performansını optimize edelim mi?", "Kaydetmek ister misiniz?",
        "Otomatik düzeltmemi ister misiniz?"
    };
    for (const auto& kw : critical_keywords) {
        if (suggestion.find(kw) != std::string::npos) return true;
    }
    return false;
}

// Kritik eylem önerisinden sadece eylem açıklamasını çıkarır
static std::string extract_action_description(const std::string& full_suggestion) {
    std::string description = full_suggestion;

    // Önek temizleme
    const std::vector<std::string> prefixes = {"AI Önerisi: ", "[AI-ICGORU]: "};
    for (const auto& pre : prefixes) {
        if (description.rfind(pre, 0) == 0) {
            description.erase(0, pre.length());
        }
    }

    // Sık kullanılan son ekleri temizle
    const std::vector<std::string> suffixes = {
        " ister misiniz?", " edebilirim.", " sağlayabilirim.", " kapatabilirim.",
        " artırabilirim.", " optimize edebilirim?", " kesmemi ister misiniz?",
        " edelim mi?", " öneririm.", " olabilirim?", " Otomatik düzeltmemi ister misiniz?",
        " Kaydetmek ister misiniz?"
    };
    for (const auto& suf : suffixes) {
        size_t pos = description.find(suf);
        if (pos != std::string::npos) description.erase(pos);
    }

    // Noktalama ve boşluk temizleme
    while (!description.empty() && (description.back() == '.' || description.back() == '?' || 
                                   description.back() == ' ' || description.back() == '\n')) {
        description.pop_back();
    }
    return description;
}

#endif // CEREBRUM_LUX_RESPONSE_ENGINE_H
