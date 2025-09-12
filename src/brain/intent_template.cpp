#include "intent_template.h" // Kendi başlık dosyasını dahil et
#include "../core/enums.h"   // AIAction ve UserIntent enum'ları için

// === IntentTemplate Implementasyonu (Constructor) ===
IntentTemplate::IntentTemplate(UserIntent intent_id, const std::vector<float>& initial_weights)
    : id(intent_id), weights(initial_weights) {
        action_success_scores[AIAction::DisableSpellCheck] = 0.0f;
        action_success_scores[AIAction::EnableCustomDictionary] = 0.0f;
        action_success_scores[AIAction::ShowUndoHistory] = 0.0f;
        action_success_scores[AIAction::CompareVersions] = 0.0f;
        action_success_scores[AIAction::DimScreen] = 0.0f;
        action_success_scores[AIAction::MuteNotifications] = 0.0f;
        action_success_scores[AIAction::LaunchApplication] = 0.0f;
        action_success_scores[AIAction::OpenFile] = 0.0f;
        action_success_scores[AIAction::SetReminder] = 0.0f;
        action_success_scores[AIAction::SimulateOSAction] = 0.0f;
        action_success_scores[AIAction::SuggestBreak] = 0.0f;
        action_success_scores[AIAction::OptimizeForGaming] = 0.0f;
        action_success_scores[AIAction::EnableFocusMode] = 0.0f;
        action_success_scores[AIAction::AdjustAudioVolume] = 0.0f;
        action_success_scores[AIAction::OpenDocumentation] = 0.0f;
        action_success_scores[AIAction::SuggestSelfImprovement] = 0.0f; // YENİ eylem
    }