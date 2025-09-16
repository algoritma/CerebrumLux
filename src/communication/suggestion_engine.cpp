#include "suggestion_engine.h" // Kendi başlık dosyasını dahil et
#include "../core/utils.h"       // LOG için (ve convert_wstring_to_string için)
#include "../data_models/dynamic_sequence.h" // DynamicSequence için
#include "../brain/intent_analyzer.h" // IntentAnalyzer için
#include <algorithm> // std::max için
#include <random> // std::random_device, std::mt19937, std::uniform_real_distribution için

// === SuggestionEngine Implementasyonlari ===

SuggestionEngine::SuggestionEngine(IntentAnalyzer& analyzer_ref) : analyzer(analyzer_ref) {}

std::string SuggestionEngine::action_to_string(AIAction action) const {
    switch (action) {
        case AIAction::DisableSpellCheck: return "Yazim denetimini devre disi birak";
        case AIAction::EnableCustomDictionary: return "Ozel sozlügü etkinlestir";
        case AIAction::ShowUndoHistory: return "Geri alma geçmisini goster";
        case AIAction::CompareVersions: return "Versiyonlari karsilastir";
        case AIAction::DimScreen: return "Ekrani karart";
        case AIAction::MuteNotifications: return "Bildirimleri sessize al";
        case AIAction::LaunchApplication: return "Uygulama baslat";
        case AIAction::OpenFile: return "Dosya aç";
        case AIAction::SetReminder: return "Hatirlatici kur";
        case AIAction::SimulateOSAction: return "OS eylemi simule et"; 
        case AIAction::SuggestBreak: return "Ara vermeyi öner";
        case AIAction::OptimizeForGaming: return "Oyun performansı için optimize et";
        case AIAction::EnableFocusMode: return "Odaklanma modunu etkinleştir";
        case AIAction::AdjustAudioVolume: return "Ses seviyesini ayarla";
        case AIAction::OpenDocumentation: return "Dokümantasyon aç";
        case AIAction::SuggestSelfImprovement: return "Kendi kendini geliştirme önerisi"; 
        case AIAction::None: return "Hiçbir eylem önerisi yok"; 
        case AIAction::Count: return "Eylem Sayisi"; // Erişilmemeli, debug amaçlı
        default: return "Tanimlanmamis eylem"; 
    }
}

AIAction SuggestionEngine::suggest_action(UserIntent current_intent, const DynamicSequence& sequence) {
    return select_action_based_on_probability(current_intent, sequence); 
}

AIAction SuggestionEngine::select_action_based_on_probability(UserIntent intent, const DynamicSequence& sequence) {
    const IntentTemplate* tmpl = nullptr;
    for (const auto& t : analyzer.intent_templates) {
        if (t.id == intent) {
            tmpl = &t;
            break;
        }
    }

    if (!tmpl || tmpl->action_success_scores.empty()) {
        return AIAction::None;
    }

    std::vector<std::pair<AIAction, float>> weighted_actions;
    float total_weight = 0.0f;

    for (const auto& pair : tmpl->action_success_scores) {
        float weight = std::max(0.1f, pair.second); 
        weighted_actions.push_back({pair.first, weight});
        total_weight += weight;
    }

    if (total_weight == 0.0f) { 
        return AIAction::None;
    }

    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_real_distribution<> distrib(0.0f, total_weight);

    float random_point = distrib(gen);
    float current_sum = 0.0f;

    for (const auto& wa : weighted_actions) {
        current_sum += wa.second;
        if (random_point <= current_sum) {
            return wa.first;
        }
    }
    return AIAction::None; 
}
