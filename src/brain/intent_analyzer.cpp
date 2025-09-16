#include "intent_analyzer.h" // Kendi başlık dosyasını dahil et
#include "../core/logger.h"      // LOG makrosu için
#include "../core/utils.h"       // intent_to_string, abstract_state_to_string için
#include "../data_models/dynamic_sequence.h" // DynamicSequence için
#include "autoencoder.h"         // CryptofigAutoencoder::LATENT_DIM için
#include "intent_template.h"     // IntentTemplate için
#include <algorithm>             // std::min/max için
#include <cmath>                 // std::log10 için
#include <fstream>               // Dosya G/Ç için
#include <iostream>              // std::cerr, std::cout için
#include <sstream>               // std::stringstream için


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
        LOG_DEFAULT(LogLevel::WARNING, "IntentAnalyzer::analyze_intent: Latent cryptofig vektörü boş veya boyut uyuşmazlığı. Unknown döndürülüyor.\n");
        return UserIntent::Unknown;
    }

    UserIntent best_intent = UserIntent::Unknown;
    float max_score = -std::numeric_limits<float>::max(); // Başlangıçta çok düşük bir değer

    for (auto& tmpl : intent_templates) {
        float score = 0.0f;
        // Ağırlık boyutu latent kriptofig boyutu ile eşleşmeli
        if (tmpl.weights.size() != sequence.latent_cryptofig_vector.size()) { 
            LOG_DEFAULT(LogLevel::ERR_CRITICAL, "IntentAnalyzer::analyze_intent: Niyet şablonu ağırlık boyutu latent kriptofig boyutuyla uyuşmuyor! Niyet: " << intent_to_string(tmpl.id) << ".\n");
            continue; // Bu şablonu atla
        }
        for (size_t i = 0; i < tmpl.weights.size(); ++i) {
            score += tmpl.weights[i] * sequence.latent_cryptofig_vector[i]; // latent_cryptofig_vector kullanılıyor
        }
        
        // --- YENİ: Bağlamsal Skor Ayarlama Mantığı ---
        if (tmpl.id == UserIntent::Editing) {
            if (sequence.avg_keystroke_interval > 1500.0f && sequence.control_key_frequency > 0.2f) {
                score *= 1.5f; 
                LOG_DEFAULT(LogLevel::DEBUG, "IntentAnalyzer: Editing skoru yavaş yazım ve kontrol tuşları nedeniyle artırıldı.\n");
            }
        }

        if (tmpl.id == UserIntent::IdleThinking) {
            if (sequence.avg_keystroke_interval > 5000.0f) { 
                score *= 2.0f; 
                LOG_DEFAULT(LogLevel::DEBUG, "IntentAnalyzer: IdleThinking skoru çok yavaş yazım nedeniyle artırıldı.\n");
            }
        }

        if (tmpl.id == UserIntent::Gaming) {
            if (sequence.avg_keystroke_interval > 2000.0f) {
                score *= 0.2f; 
                LOG_DEFAULT(LogLevel::DEBUG, "IntentAnalyzer: Gaming skoru yavaş yazım nedeniyle düşürüldü.\n");
            }
        }
        // --- YENİ MANTIK SONU ---

        if (score > max_score) {
            max_score = score;
            best_intent = tmpl.id;
        }
    }
    
    if (max_score < this->confidence_threshold_for_known_intent) { 
        LOG_DEFAULT(LogLevel::INFO, "IntentAnalyzer: En yüksek skor (" << max_score << ") eşik değerinin (" << this->confidence_threshold_for_known_intent << ") altında. Niyet 'Unknown' olarak ayarlandı.\n");
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
    LOG_DEFAULT(LogLevel::INFO, "[Meta-AI] Niyet: " << intent_to_string(intent_id) << 
               ", Ortuk Performans (Ort): " << implicit_feedback_avg << 
               ", Acik Performans (Ort): " << explicit_feedback_avg << "\n"); 

}

void IntentAnalyzer::save_memory(const std::string& filename) const {
    FILE* fp = fopen(filename.c_str(), "w"); 
    if (!fp) {
        LOG_DEFAULT(LogLevel::ERR_CRITICAL, "Hata: AI hafiza dosyasi yazilamadi: " << filename << " (errno: " << errno << ")\n");
        return;
    }

    fprintf(fp, "%zu\n", intent_templates.size());
    for (const auto& tmpl : intent_templates) {
        fprintf(fp, "%d %zu ", static_cast<int>(tmpl.id), tmpl.weights.size());
        for (float w : tmpl.weights) {
            fprintf(fp, "%.8f ", w); 
        }
        fprintf(fp, "%zu ", tmpl.action_success_scores.size());
        for (const auto& pair : tmpl.action_success_scores) {
            fprintf(fp, "%d %.8f ", static_cast<int>(pair.first), pair.second); 
        }
        fprintf(fp, "\n");
    }
    fclose(fp);
    LOG_DEFAULT(LogLevel::INFO, "AI hafizasi kaydedildi: " << filename << "\n");
}

void IntentAnalyzer::load_memory(const std::string& filename) {
    FILE* fp = fopen(filename.c_str(), "r"); 
    if (!fp) {
        LOG_DEFAULT(LogLevel::WARNING, "Uyari: AI hafiza dosyasi bulunamadi, varsayilan sablonlar kullaniliyor: " << filename << " (errno: " << errno << ")\n");
        return;
    }

    intent_templates.clear(); 
    size_t num_templates;
    if (fscanf(fp, "%zu\n", &num_templates) != 1) { 
        LOG_DEFAULT(LogLevel::ERR_CRITICAL, "Hata: AI hafiza dosyasi formati bozuk veya bos (templates): " << filename << "\n");
        fclose(fp);
        return;
    }

    for (size_t i = 0; i < num_templates; ++i) {
        int intent_id_int;
        if (fscanf(fp, "%d", &intent_id_int) != 1) {
             LOG_DEFAULT(LogLevel::ERR_CRITICAL, "Hata: AI hafiza dosyasi yuklenirken intent_id okunamadi. Satir: " << i << "\n");
             break;
        }
        UserIntent intent_id = static_cast<UserIntent>(intent_id_int);

        size_t num_weights;
        if (fscanf(fp, "%zu", &num_weights) != 1) {
            LOG_DEFAULT(LogLevel::ERR_CRITICAL, "Hata: AI hafiza dosyasi yuklenirken num_weights okunamadi. Satir: " << i << "\n");
            break;
        }
        std::vector<float> weights(num_weights);
        for (size_t j = 0; j < num_weights; ++j) {
            if (fscanf(fp, "%f", &weights[j]) != 1) {
                LOG_DEFAULT(LogLevel::ERR_CRITICAL, "Hata: AI hafiza dosyasi yuklenirken weight okunamadi. Satir: " << i << ", Eleman: " << j << "\n");
                break;
            }
        }

        size_t num_action_scores;
        if (fscanf(fp, "%zu", &num_action_scores) != 1) {
            LOG_DEFAULT(LogLevel::ERR_CRITICAL, "Hata: AI hafiza dosyasi yuklenirken num_action_scores okunamadi. Satir: " << i << "\n");
            break;
        }
        std::map<AIAction, float> action_scores;
        for (size_t j = 0; j < num_action_scores; ++j) {
            int action_id_int;
            float score;
            if (fscanf(fp, "%d %f", &action_id_int, &score) != 2) {
                LOG_DEFAULT(LogLevel::ERR_CRITICAL, "Hata: AI hafiza dosyasi yuklenirken action_score okunamadi. Satir: " << i << ", Eleman: " << j << "\n");
                break;
            }
            action_scores[static_cast<AIAction>(action_id_int)] = score;
        }
        
        char newline_char;
        fscanf(fp, "%c", &newline_char); 

        intent_templates.emplace_back(intent_id, weights);
        intent_templates.back().action_success_scores = action_scores;
    }
    fclose(fp);
    LOG_DEFAULT(LogLevel::INFO, "AI hafizasi yuklendi: " << filename << "\n");
}
