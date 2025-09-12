#include "goal_manager.h" // Kendi başlık dosyasını dahil et
#include "../core/utils.h"       // LOG_MESSAGE ve intent_to_string için
#include "../data_models/dynamic_sequence.h" // DynamicSequence için
#include "../communication/ai_insights_engine.h" // AIInsightsEngine için
#include "../brain/intent_analyzer.h" // IntentAnalyzer için (analyze_intent çağrısı için)
#include "../brain/autoencoder.h" // CryptofigAutoencoder::INPUT_DIM için (reconstruction_error için)
#include <vector>
#include <algorithm> // std::min/max
#include <iostream>  // std::wcerr için


// === GoalManager Implementasyonlari ===
GoalManager::GoalManager(AIInsightsEngine& insights_engine_ref) : current_goal(AIGoal::OptimizeProductivity), insights_engine(insights_engine_ref) {} 

AIGoal GoalManager::get_current_goal() const {
    return current_goal;
}

void GoalManager::set_current_goal(AIGoal goal) {
    current_goal = goal;
    std::wstringstream ss_goal;
    ss_goal << L"[AI-Hedef] Yeni hedef ayarlandi: ";
    switch (goal) {
        case AIGoal::OptimizeProductivity: ss_goal << L"Uretkenligi Optimize Etmek"; break;
        case AIGoal::MaximizeBatteryLife:  ss_goal << L"Batarya Omrunu Maksimuma Çikarmak"; break;
        case AIGoal::ReduceDistractions:   ss_goal << L"Dikkat Dagiticilari Azaltmak"; break;
        case AIGoal::SelfImprovement:      ss_goal << L"Kendi Kendini Geliştirmek"; break; 
        case AIGoal::None: ss_goal << L"Yok"; break; 
        case AIGoal::EnhanceCreativity: ss_goal << L"Yaraticiligi Artirmak"; break;
        case AIGoal::ImproveGamingExperience: ss_goal << L"Oyun Deneyimini Iyileştirmek"; break;
        case AIGoal::FacilitateResearch: ss_goal << L"Arastirmayi Kolaylastirmak"; break;
        case AIGoal::Count: ss_goal << L"Hedef Sayisi"; // Erişilmemeli, debug amaçlı
    }
    LOG_MESSAGE(LogLevel::INFO, std::wcout, ss_goal.str() << L"\n");
}

// Dinamik hedef belirleme fonksiyonu
void GoalManager::evaluate_and_set_goal(const DynamicSequence& current_sequence) {
    LOG_MESSAGE(LogLevel::DEBUG, std::wcerr, L"GoalManager::evaluate_and_set_goal: Dinamik hedef belirleme basladi.\n");
    std::vector<AIInsight> insights = insights_engine.generate_insights(current_sequence);

    // En yüksek aciliyetli içgörüye göre hedef belirle
    float max_urgency = 0.0f;
    AIAction critical_action = AIAction::None;

    for (const auto& insight : insights) {
        if (insight.urgency > max_urgency) {
            max_urgency = insight.urgency;
            critical_action = insight.suggested_action;
        }
    }

    UserIntent analyzed_current_intent = insights_engine.get_analyzer().analyze_intent(current_sequence); // AIInsightsEngine'ın getter'ı kullanıldı

    if (max_urgency > 0.7f && critical_action == AIAction::SuggestSelfImprovement) {
        if (current_goal != AIGoal::SelfImprovement) {
            set_current_goal(AIGoal::SelfImprovement);
            LOG_MESSAGE(LogLevel::INFO, std::wcout, L"[AI-Hedef] İçgörüye dayalı olarak 'Kendi Kendini Geliştirme' hedefi belirlendi.\n");
        }
    } else if (current_sequence.current_battery_percentage < 20 && !current_sequence.current_battery_charging) {
        if (current_goal != AIGoal::MaximizeBatteryLife) {
            set_current_goal(AIGoal::MaximizeBatteryLife);
            LOG_MESSAGE(LogLevel::INFO, std::wcout, L"[AI-Hedef] Kritik batarya seviyesi nedeniyle 'Batarya Ömrünü Maksimuma Çıkarma' hedefi belirlendi.\n");
        }
    } else if (current_sequence.current_network_active && current_sequence.network_activity_level == 0 && current_sequence.statistical_features_vector.size() == CryptofigAutoencoder::INPUT_DIM && insights_engine.calculate_autoencoder_reconstruction_error(current_sequence.statistical_features_vector) > 0.5f) {
        // Ağ bağlı ama aktivite yok ve Autoencoder hatası yüksek -> Belki bir donanım sorunu var?
        if (current_goal != AIGoal::ReduceDistractions) { 
            set_current_goal(AIGoal::ReduceDistractions); 
            LOG_MESSAGE(LogLevel::INFO, std::wcout, L"[AI-Hedef] Potansiyel ağ/donanım anormalliği nedeniyle 'Dikkat Dağıtıcıları Azaltma' hedefi belirlendi.\n");
        }
    } else if (analyzed_current_intent == UserIntent::IdleThinking && current_sequence.mouse_movement_intensity / 500.0f > 0.2f && current_sequence.network_activity_level / 15000.0f > 0.2f) { 
        if (current_goal != AIGoal::ReduceDistractions) {
            set_current_goal(AIGoal::ReduceDistractions);
            LOG_MESSAGE(LogLevel::INFO, std::wcout, L"[AI-Hedef] Boşta olmasına rağmen aktiflik nedeniyle 'Dikkat Dağıtıcıları Azaltma' hedefi belirlendi.\n");
        }
    }
    // Varsayılan olarak üretkenliğe dön (eğer kritik bir durum yoksa)
    else if (current_goal != AIGoal::OptimizeProductivity) {
        set_current_goal(AIGoal::OptimizeProductivity);
        LOG_MESSAGE(LogLevel::INFO, std::wcout, L"[AI-Hedef] Varsayılan olarak 'Üretkenliği Optimize Etme' hedefi belirlendi.\n");
    }
    LOG_MESSAGE(LogLevel::DEBUG, std::wcerr, L"GoalManager::evaluate_and_set_goal: Dinamik hedef belirleme bitti. Mevcut hedef: " << static_cast<int>(current_goal) << L"\n");
}