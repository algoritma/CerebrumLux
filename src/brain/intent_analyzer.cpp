#include "intent_analyzer.h" // Kendi başlık dosyasını dahil et
#include "../core/logger.h"      // LOG makrosu için
#include "../core/utils.h"       // intent_to_string, abstract_state_to_string için
#include "../data_models/dynamic_sequence.h" // DynamicSequence için
#include "autoencoder.h"         // CryptofigAutoencoder::LATENT_DIM için
#include "intent_template.h"     // IntentTemplate için
#include <algorithm>             // std::min/max için
#include <cmath>                 // std::log10 için
#include <fstream>               // Dosya G/Ç için (fwprintf, fwscanf)
#include <iostream>              // std::wcerr, std::wcout için


// === IntentTemplate Implementasyonu (burada kalmalı, IntentAnalyzer'ın bir parçası) ===
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
        action_success_scores[AIAction::SuggestSelfImprovement] = 0.0f; 
    }

// === IntentAnalyzer Implementasyonlari ===

IntentAnalyzer::IntentAnalyzer() : confidence_threshold_for_known_intent(0.1f) { 
    // Ağırlıklar artık LATENT_DIM boyutunda olmalı
    intent_templates.emplace_back(UserIntent::FastTyping,    std::vector<float>(CryptofigAutoencoder::LATENT_DIM, 0.0f)); 
    intent_templates.emplace_back(UserIntent::Editing,       std::vector<float>(CryptofigAutoencoder::LATENT_DIM, 0.0f)); 
    intent_templates.emplace_back(UserIntent::IdleThinking,  std::vector<float>(CryptofigAutoencoder::LATENT_DIM, 0.0f)); 
    // YENİ NİYETLER İÇİN BAŞLANGIÇ AĞIRLIKLARI (varsayımsal ve LATENT_DIM boyutunda)
    intent_templates.emplace_back(UserIntent::Programming,    std::vector<float>(CryptofigAutoencoder::LATENT_DIM, 0.0f)); 
    intent_templates.emplace_back(UserIntent::Gaming,         std::vector<float>(CryptofigAutoencoder::LATENT_DIM, 0.0f)); 
    intent_templates.emplace_back(UserIntent::MediaConsumption,std::vector<float>(CryptofigAutoencoder::LATENT_DIM, 0.0f)); 
    intent_templates.emplace_back(UserIntent::CreativeWork,   std::vector<float>(CryptofigAutoencoder::LATENT_DIM, 0.0f)); 
    intent_templates.emplace_back(UserIntent::Research,       std::vector<float>(CryptofigAutoencoder::LATENT_DIM, 0.0f)); 
    intent_templates.emplace_back(UserIntent::Communication,  std::vector<float>(CryptofigAutoencoder::LATENT_DIM, 0.0f)); 
    
    // Varsayılan ağırlıkları daha anlamlı başlatma (latent uzaydaki temsili anlamlara göre)
    // Örn: latent_activity, latent_complexity, latent_engagement gibi
    intent_templates[0].weights = {-0.2f, -0.5f,  0.8f}; // FastTyping (Düşük karmaşıklık, yüksek etkileşim)
    intent_templates[1].weights = { 0.5f,  0.8f,  0.6f}; // Editing (Orta aktiflik, yüksek karmaşıklık)
    intent_templates[2].weights = { 0.8f, -0.7f, -0.5f}; // IdleThinking (Yüksek pasiflik, düşük etkileşim)

    // Yeni niyetler için daha iyi başlangıç ağırlıkları (örnek)
    intent_templates[3].weights = { 0.6f,  0.9f,  0.7f}; // Programming (odaklanma, karmaşıklık, etkileşim)
    intent_templates[4].weights = { 0.9f,  0.7f,  0.9f}; // Gaming (yüksek aktiflik, orta karmaşıklık, yüksek etkileşim)
    intent_templates[5].weights = {-0.8f, -0.6f,  0.2f}; // MediaConsumption (düşük aktiflik, düşük karmaşıklık, az etkileşim)
    intent_templates[6].weights = { 0.7f,  0.8f,  0.7f}; // CreativeWork (orta aktiflik, yüksek karmaşıklık, etkileşim)
    intent_templates[7].weights = { 0.4f,  0.6f,  0.8f}; // Research (orta aktiflik, yüksek dış etkileşim)
    intent_templates[8].weights = { 0.7f,  0.3f,  0.9f}; // Communication (yüksek aktiflik, düşük karmaşıklık, yüksek etkileşim)
}

UserIntent IntentAnalyzer::analyze_intent(const DynamicSequence& sequence) {
    // latent_cryptofig_vector kontrol ediliyor
    if (sequence.latent_cryptofig_vector.empty() || sequence.latent_cryptofig_vector.size() != CryptofigAutoencoder::LATENT_DIM) { 
        LOG(LogLevel::WARNING, L"IntentAnalyzer::analyze_intent: Latent cryptofig vektörü boş veya boyut uyuşmazlığı. Unknown döndürülüyor.\n");
        return UserIntent::Unknown;
    }

    UserIntent best_intent = UserIntent::Unknown;
    float max_score = -std::numeric_limits<float>::max(); // Başlangıçta çok düşük bir değer

    for (const auto& tmpl : intent_templates) {
        float score = 0.0f;
        // Ağırlık boyutu latent kriptofig boyutu ile eşleşmeli
        if (tmpl.weights.size() != sequence.latent_cryptofig_vector.size()) { 
            LOG(LogLevel::ERR_CRITICAL, L"IntentAnalyzer::analyze_intent: Niyet şablonu ağırlık boyutu latent kriptofig boyutuyla uyuşmuyor! Niyet: " << intent_to_string(tmpl.id) << L".\n");
            continue; // Bu şablonu atla
        }
        for (size_t i = 0; i < tmpl.weights.size(); ++i) {
            score += tmpl.weights[i] * sequence.latent_cryptofig_vector[i]; // latent_cryptofig_vector kullanılıyor
        }
        
        if (score > max_score) {
            max_score = score;
            best_intent = tmpl.id;
        }
    }
    
    if (max_score < this->confidence_threshold_for_known_intent) { 
        best_intent = UserIntent::Unknown;
    }

    return best_intent;
}

void IntentAnalyzer::set_confidence_threshold(float threshold) {
    confidence_threshold_for_known_intent = std::min(0.8f, std::max(0.01f, threshold));
}

void IntentAnalyzer::update_template_weights(UserIntent intent_id, const std::vector<float>& new_weights) {
    for (auto& tmpl : intent_templates) {
        if (tmpl.id == intent_id) {
            tmpl.weights = new_weights;
            return;
        }
    }
}

void IntentAnalyzer::update_action_success_score(UserIntent intent_id, AIAction action, float score_change) {
    for (auto& tmpl : intent_templates) {
        if (tmpl.id == intent_id) {
            tmpl.action_success_scores[action] += score_change;
            tmpl.action_success_scores[action] = std::min(10.0f, std::max(-10.0f, tmpl.action_success_scores[action]));
            return;
        }
    }
}

std::vector<float> IntentAnalyzer::get_intent_weights(UserIntent intent_id) const {
    for (const auto& tmpl : intent_templates) {
        if (tmpl.id == intent_id) {
            return tmpl.weights;
        }
    }
    return {}; 
}

void IntentAnalyzer::report_learning_performance(UserIntent intent_id, float implicit_feedback_avg, float explicit_feedback_avg) {
    LOG(LogLevel::INFO, L"[Meta-AI] Niyet: " << intent_to_string(intent_id) 
               << L", Ortuk Performans (Ort): " << std::fixed << std::setprecision(4) << implicit_feedback_avg
               << L", Acik Performans (Ort): " << explicit_feedback_avg << L"\n"); 

}

AbstractState IntentAnalyzer::analyze_abstract_state(const DynamicSequence& sequence, UserIntent current_intent) const {
    // latent_cryptofig_vector kontrol ediliyor
    if (sequence.latent_cryptofig_vector.empty() || sequence.latent_cryptofig_vector.size() != CryptofigAutoencoder::LATENT_DIM) {
        LOG(LogLevel::WARNING, L"IntentAnalyzer::analyze_abstract_state: Latent cryptofig vektörü boş veya boyut uyuşmazlığı. None döndürülüyor.\n");
        return AbstractState::None;
    }

    // Metrikler DynamicSequence'in üyelerinden alınıyor.
    // Not: Bu metrikler DynamicSequence::update_from_signals içinde zaten normalize edilmişti.
    float normalized_avg_interval = sequence.avg_keystroke_interval / 1000.0f; // ms'den saniye
    float normalized_variability = sequence.keystroke_variability / 1000.0f;   // ms'den saniye
    float alphanumeric_ratio = sequence.alphanumeric_ratio;           
    float control_key_frequency = sequence.control_key_frequency;     
    float mouse_movement_intensity_norm = sequence.mouse_movement_intensity / 500.0f; // Normalize edilmiş hali
    float mouse_click_norm = sequence.mouse_click_frequency;          
    float avg_brightness_norm = sequence.avg_brightness / 255.0f;              
    float battery_status_change_norm = sequence.battery_status_change; 
    float network_activity_level_norm = sequence.network_activity_level / 15000.0f; 

    std::map<AbstractState, float> state_scores;
    state_scores[AbstractState::NormalOperation] = 0.5f; 

    // Mevcut durumlar için puanlama (önceki koddan)
    float score_high_productivity = 0.0f;
    if (current_intent == UserIntent::FastTyping || current_intent == UserIntent::Programming || current_intent == UserIntent::Communication) score_high_productivity += 0.8f; 
    score_high_productivity += (alphanumeric_ratio > 0.90f ? 0.7f : 0.0f); 
    score_high_productivity += (normalized_variability < 0.10f ? 0.6f : 0.0f); 
    score_high_productivity += (normalized_avg_interval < 0.25f ? 0.6f : 0.0f); 
    score_high_productivity += (mouse_movement_intensity_norm < 0.08f ? 0.5f : 0.0f); 
    score_high_productivity += (mouse_click_norm < 0.08f ? 0.4f : 0.0f); 
    score_high_productivity += (network_activity_level_norm < 0.15f ? 0.4f : 0.0f); 
    score_high_productivity += (avg_brightness_norm > 0.6f ? 0.3f : 0.0f); 
    state_scores[AbstractState::HighProductivity] = score_high_productivity;

    float score_focused = 0.0f;
    if (current_intent == UserIntent::Editing || current_intent == UserIntent::Programming || current_intent == UserIntent::CreativeWork || current_intent == UserIntent::Research) score_focused += 0.9f; 
    score_focused += (control_key_frequency > 0.5f ? 0.7f : 0.0f); 
    score_focused += (alphanumeric_ratio < 0.7f && alphanumeric_ratio > 0.3f ? 0.5f : 0.0f); 
    score_focused += (normalized_variability > 0.3f && normalized_variability < 0.7f ? 0.5f : 0.0f); 
    score_focused += (mouse_click_norm > 0.2f && mouse_movement_intensity_norm < 0.5f ? 0.6f : 0.0f); 
    score_focused += (avg_brightness_norm > 0.5f ? 0.3f : 0.0f); 
    state_scores[AbstractState::Focused] = score_focused;

    float score_power_saving = 0.0f;
    if (current_intent == UserIntent::IdleThinking || current_intent == UserIntent::MediaConsumption) score_power_saving += 0.8f; 
    score_power_saving += (normalized_avg_interval > 0.8f ? 0.7f : 0.0f); 
    score_power_saving += (alphanumeric_ratio < 0.2f ? 0.5f : 0.0f); 
    score_power_saving += (mouse_movement_intensity_norm < 0.05f ? 0.5f : 0.0f); 
    score_power_saving += (network_activity_level_norm < 0.05f ? 0.5f : 0.0f); 
    score_power_saving += (avg_brightness_norm < 0.2f ? 0.8f : 0.0f); 
    if (sequence.current_battery_percentage < 20 && !sequence.current_battery_charging) score_power_saving += 1.5f; 
    state_scores[AbstractState::PowerSaving] = score_power_saving;

    float score_distracted = 0.0f;
    if (current_intent == UserIntent::IdleThinking || current_intent == UserIntent::Gaming || current_intent == UserIntent::MediaConsumption) score_distracted += 0.5f; 
    score_distracted += (mouse_movement_intensity_norm > 0.5f ? 0.7f : 0.0f); 
    score_distracted += (mouse_click_norm > 0.3f ? 0.6f : 0.0f); 
    score_distracted += (network_activity_level_norm > 0.6f ? 0.6f : 0.0f); 
    score_distracted += (alphanumeric_ratio < 0.4f && control_key_frequency < 0.2f ? 0.5f : 0.0f); 
    state_scores[AbstractState::Distracted] = score_distracted;

    float score_low_productivity = 0.0f;
    if (current_intent == UserIntent::Unknown || current_intent == UserIntent::IdleThinking) score_low_productivity += 0.8f; 
    score_low_productivity += (normalized_avg_interval > 0.6f ? 0.6f : 0.0f); 
    score_low_productivity += (alphanumeric_ratio < 0.5f ? 0.5f : 0.0f); 
    score_low_productivity += (control_key_frequency < 0.3f ? 0.4f : 0.0f); 
    if (score_high_productivity < 0.5f && score_focused < 0.5f && score_distracted < 0.5f && score_power_saving < 0.5f) {
        score_low_productivity += 0.4f;
    }
    state_scores[AbstractState::LowProductivity] = score_low_productivity;

    // YENİ SOYUT DURUMLAR İÇİN PUANLAMA
    float score_creative_flow = 0.0f;
    if (current_intent == UserIntent::CreativeWork) score_creative_flow += 1.2f;
    score_creative_flow += (normalized_variability > 0.5f && normalized_variability < 0.9f ? 0.7f : 0.0f); // Düzensiz, ani etkileşimler
    score_creative_flow += (mouse_movement_intensity_norm > 0.3f && mouse_click_norm > 0.3f ? 0.6f : 0.0f); // Yoğun fare kullanımı
    score_creative_flow += (alphanumeric_ratio < 0.6f ? 0.4f : 0.0f); // Daha az metin girişi
    state_scores[AbstractState::CreativeFlow] = score_creative_flow;

    float score_debugging = 0.0f;
    if (current_intent == UserIntent::Programming) score_debugging += 1.5f;
    score_debugging += (control_key_frequency > 0.7f ? 0.8f : 0.0f); // Hata ayıklama, kopyala/yapıştır vb.
    score_debugging += (normalized_avg_interval > 0.4f && normalized_avg_interval < 0.8f ? 0.6f : 0.0f); // Duraklamalı klavye
    score_debugging += (mouse_click_norm > 0.4f ? 0.5f : 0.0f); // Tıklama ile kod satırları seçme
    state_scores[AbstractState::Debugging] = score_debugging;

    float score_passive_consumption = 0.0f;
    if (current_intent == UserIntent::MediaConsumption) score_passive_consumption += 1.5f;
    score_passive_consumption += (normalized_avg_interval > 0.9f ? 0.8f : 0.0f); // Çok uzun klavye aralıkları
    score_passive_consumption += (mouse_movement_intensity_norm < 0.1f && mouse_click_norm < 0.1f ? 0.7f : 0.0f); // Çok az fare etkileşimi
    score_passive_consumption += (network_activity_level_norm > 0.2f ? 0.4f : 0.0f); // Streaming için ağ aktivitesi
    state_scores[AbstractState::PassiveConsumption] = score_passive_consumption;

    float score_hardware_anomaly = 0.0f;
    // Donanım anormalliği doğrudan niyetle ilişkili olmayabilir, daha çok genel sistem durumu
    if (battery_status_change_norm > 0.05f) score_hardware_anomaly += 0.8f; // Ani pil düşüşü
    if (network_activity_level_norm == 0.0f && sequence.current_network_active) score_hardware_anomaly += 0.7f; // Ağ aktif görünürken bant genişliği sıfır
    if (sequence.avg_brightness == 0.0f && sequence.current_display_on) score_hardware_anomaly += 0.6f; // Ekran açıkken parlaklık sıfır (sensör hatası?)
    state_scores[AbstractState::HardwareAnomaly] = score_hardware_anomaly;

    float score_seeking_information = 0.0f;
    if (current_intent == UserIntent::Research) score_seeking_information += 1.2f;
    score_seeking_information += (network_activity_level_norm > 0.7f ? 0.8f : 0.0f); // Yoğun ağ kullanımı
    score_seeking_information += (mouse_click_norm > 0.4f ? 0.5f : 0.0f); // Linklere tıklama
    score_seeking_information += (alphanumeric_ratio < 0.5f && normalized_avg_interval > 0.5f ? 0.4f : 0.0f); // Kısa metinler (arama terimleri) ve duraklamalar
    state_scores[AbstractState::SeekingInformation] = score_seeking_information;

    float score_social_interaction = 0.0f;
    if (current_intent == UserIntent::Communication) score_social_interaction += 1.2f;
    score_social_interaction += (alphanumeric_ratio > 0.8f && normalized_avg_interval < 0.4f ? 0.7f : 0.0f); // Hızlı, kısa metin yazma
    score_social_interaction += (control_key_frequency < 0.1f ? 0.3f : 0.0f); // Daha az kontrol tuşu
    state_scores[AbstractState::SocialInteraction] = score_social_interaction;


    AbstractState best_abstract_state = AbstractState::NormalOperation;
    float max_state_score = state_scores[AbstractState::NormalOperation]; 

    // Yeni puanlamada tüm durumları kontrol et
    for (const auto& pair : state_scores) {
        if (pair.first != AbstractState::None && pair.second > max_state_score) {
            max_state_score = pair.second;
            best_abstract_state = pair.first;
        }
    }
    
    // Eğer 'Unknown' niyet ve 'NormalOperation' baskınsa, 'LowProductivity' yerine 'SeekingInformation' da düşünülebilir
    if (current_intent == UserIntent::Unknown && best_abstract_state == AbstractState::NormalOperation) {
        // Burada daha fazla mantık eklenebilir, şimdilik LowProductivity'i koruyalım veya doğrudan Unknown'a bırakalım
    }

    return best_abstract_state;  
}

void IntentAnalyzer::save_memory(const std::wstring& filename) const {
    FILE* fp = _wfopen(filename.c_str(), L"w"); 
    if (!fp) {
        LOG(LogLevel::ERR_CRITICAL, L"Hata: AI hafiza dosyasi yazilamadi: " << filename << L" (errno: " << errno << L")\n");
        return;
    }

    fwprintf(fp, L"%zu\n", intent_templates.size());
    for (const auto& tmpl : intent_templates) {
        fwprintf(fp, L"%d %zu ", static_cast<int>(tmpl.id), tmpl.weights.size());
        for (float w : tmpl.weights) {
            fwprintf(fp, L"%.8f ", w); 
        }
        fwprintf(fp, L"%zu ", tmpl.action_success_scores.size());
        for (const auto& pair : tmpl.action_success_scores) {
            fwprintf(fp, L"%d %.8f ", static_cast<int>(pair.first), pair.second); 
        }
        fwprintf(fp, L"\n");
    }
    fclose(fp);
    LOG(LogLevel::INFO, L"AI hafizasi kaydedildi: " << filename << L"\n");
}

void IntentAnalyzer::load_memory(const std::wstring& filename) {
    FILE* fp = _wfopen(filename.c_str(), L"r"); 
    if (!fp) {
        LOG(LogLevel::WARNING, L"Uyari: AI hafiza dosyasi bulunamadi, varsayilan sablonlar kullaniliyor: " << filename << L" (errno: " << errno << L")\n");
        return;
    }

    intent_templates.clear(); 
    size_t num_templates;
    if (fwscanf(fp, L"%zu\n", &num_templates) != 1) { 
        LOG(LogLevel::ERR_CRITICAL, L"Hata: AI hafiza dosyasi formati bozuk veya bos (templates): " << filename << L"\n");
        fclose(fp);
        return;
    }

    for (size_t i = 0; i < num_templates; ++i) {
        int intent_id_int;
        if (fwscanf(fp, L"%d", &intent_id_int) != 1) {
             LOG(LogLevel::ERR_CRITICAL, L"Hata: AI hafiza dosyasi yuklenirken intent_id okunamadi. Satir: " << i << L"\n");
             break;
        }
        UserIntent intent_id = static_cast<UserIntent>(intent_id_int);

        size_t num_weights;
        if (fwscanf(fp, L"%zu", &num_weights) != 1) {
            LOG(LogLevel::ERR_CRITICAL, L"Hata: AI hafiza dosyasi yuklenirken num_weights okunamadi. Satir: " << i << L"\n");
            break;
        }
        std::vector<float> weights(num_weights);
        for (size_t j = 0; j < num_weights; ++j) {
            if (fwscanf(fp, L"%f", &weights[j]) != 1) {
                LOG(LogLevel::ERR_CRITICAL, L"Hata: AI hafiza dosyasi yuklenirken weight okunamadi. Satir: " << i << L", Eleman: " << j << L"\n");
                break;
            }
        }

        size_t num_action_scores;
        if (fwscanf(fp, L"%zu", &num_action_scores) != 1) {
            LOG(LogLevel::ERR_CRITICAL, L"Hata: AI hafiza dosyasi yuklenirken num_action_scores okunamadi. Satir: " << i << L"\n");
            break;
        }
        std::map<AIAction, float> action_scores;
        for (size_t j = 0; j < num_action_scores; ++j) {
            int action_id_int;
            float score;
            if (fwscanf(fp, L"%d %f", &action_id_int, &score) != 2) {
                LOG(LogLevel::ERR_CRITICAL, L"Hata: AI hafiza dosyasi yuklenirken action_score okunamadi. Satir: " << i << L", Eleman: " << j << L"\n");
                break;
            }
            action_scores[static_cast<AIAction>(action_id_int)] = score;
        }
        
        wchar_t newline_char;
        fwscanf(fp, L"%c", &newline_char); 

        intent_templates.emplace_back(intent_id, weights);
        intent_templates.back().action_success_scores = action_scores;
    }
    fclose(fp);
    LOG(LogLevel::INFO, L"AI hafizasi yuklendi: " << filename << L"\n");
}