#include "suggestion_engine.h" // Kendi başlık dosyasını dahil et
#include "../core/utils.h"       // LOG için (ve convert_wstring_to_string için)
#include "../core/logger.h"      // YENİ: LOG makrosu için logger.h dahil edildi
#include "../data_models/dynamic_sequence.h" // DynamicSequence için
#include "../brain/intent_analyzer.h" // IntentAnalyzer için
#include <algorithm> // std::max için
#include <random> // std::random_device, std::mt19937, std::uniform_real_distribution için
#include <iostream> // Debug için


// === SuggestionEngine Implementasyonlari ===

SuggestionEngine::SuggestionEngine(IntentAnalyzer& analyzer_ref) 
    : analyzer(analyzer_ref), gen(rd()) // rd() ile s_gen başlatılıyor
{
    // Q-tablosunu başlatmaya gerek yok, erişildikçe varsayılan 0.0f değeri alacak.
    // Ancak, exploration/exploitation için rastgele sayı üretecinin başlatılması önemlidir.
}

// YENİ: Q-değerini güncelleme metodu
void SuggestionEngine::update_q_value(const StateKey& state, AIAction action, float reward) {
    // Q(s,a) = Q(s,a) + alpha * [r + gamma * max(Q(s',a')) - Q(s,a)]
    // Basitlik için ve sonraki durumu henüz bilemediğimiz için, Sarsa benzeri bir güncelleme veya
    // sadece mevcut Q(s,a) değerini güncelleme yapalım.
    // Şimdilik, sadece mevcut eylemin Q-değerini güncelliyoruz (tek adımlı öğrenme).

    float current_q = q_table[state][action]; // Varsayılan olarak 0.0f eğer yoksa
    
    // Basit bir güncelleme: Q(s,a) += alpha * (reward - Q(s,a))
    // Bu, Q-değerini ödül hedefine doğru hareket ettirir.
    q_table[state][action] += learning_rate_rl * (reward - current_q);

    LOG_DEFAULT(LogLevel::DEBUG, "SuggestionEngine: Q-değeri güncellendi. Niyet: " << intent_to_string(state.intent) << 
                ", Durum: " << abstract_state_to_string(state.state) << 
                ", Eylem: " << action_to_string(action) << // action_to_string artık global
                ", Ödül: " << reward << ", Yeni Q: " << q_table[state][action] << "\n");
}


// suggest_action metodu güncellendi: Q-tablosu ve epsilon-greedy stratejisi kullanılıyor
AIAction SuggestionEngine::suggest_action(UserIntent current_intent, AbstractState current_abstract_state, const DynamicSequence& sequence) {
    StateKey current_state_key = {current_intent, current_abstract_state}; // Doğru StateKey oluşturuldu

    // Epsilon-greedy stratejisi
    std::uniform_real_distribution<float> distrib_rand(0.0f, 1.0f);
    if (distrib_rand(gen) < exploration_rate_rl) {
        // Keşfet (rastgele eylem seç)
        std::vector<AIAction> possible_actions;
        for (int i = 0; i < static_cast<int>(AIAction::Count); ++i) {
            AIAction action = static_cast<AIAction>(i);
            if (action != AIAction::None && action != AIAction::Count) { // Geçersiz eylemleri hariç tut
                possible_actions.push_back(action);
            }
        }
        if (!possible_actions.empty()) {
            std::uniform_int_distribution<> action_distrib(0, possible_actions.size() - 1);
            AIAction chosen_action = possible_actions[action_distrib(gen)];
            LOG_DEFAULT(LogLevel::DEBUG, "SuggestionEngine: Rastgele eylem seçildi (keşif). Niyet: " << intent_to_string(current_intent) << ", Durum: " << abstract_state_to_string(current_abstract_state) << ", Eylem: " << action_to_string(chosen_action) << "\n");
            return chosen_action;
        }
    } else {
        // Faydalan (en iyi Q-değerine sahip eylemi seç)
        float max_q_value = -std::numeric_limits<float>::max();
        AIAction best_action = AIAction::None;
        
        // Q-tablosunda bu duruma ait eylemleri kontrol et
        if (q_table.count(current_state_key)) {
            const auto& actions_for_state = q_table.at(current_state_key);
            for (const auto& pair : actions_for_state) {
                if (pair.second > max_q_value) {
                    max_q_value = pair.second;
                    best_action = pair.first;
                }
            }
        }

        if (best_action != AIAction::None) {
            LOG_DEFAULT(LogLevel::DEBUG, "SuggestionEngine: En iyi eylem seçildi (faydalanma). Niyet: " << intent_to_string(current_intent) << ", Durum: " << abstract_state_to_string(current_abstract_state) << ", Eylem: " << action_to_string(best_action) << " (Q: " << max_q_value << ")\n");
            return best_action;
        }
    }

    // Eğer hiçbir şey seçilemezse, varsayılan olarak None döndür
    LOG_DEFAULT(LogLevel::DEBUG, "SuggestionEngine: Varsayılan eylem seçildi (None).\n");
    return AIAction::None;
}