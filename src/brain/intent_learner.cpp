#include "intent_learner.h"
#include "../core/logger.h"
#include "../core/utils.h" // intent_to_string, abstract_state_to_string için
#include <numeric> // std::accumulate
#include <algorithm> // std::max, std::min
#include <map> // geçici olarak StateKey için

namespace CerebrumLux {

IntentLearner::IntentLearner(IntentAnalyzer& analyzer_ref, SuggestionEngine& suggester_ref, UserProfileManager& user_profile_manager_ref)
    : intent_analyzer(analyzer_ref),
      suggestion_engine(suggester_ref),
      user_profile_manager(user_profile_manager_ref),
      learning_rate(0.01f), // Başlangıç öğrenme hızı
      feedback_history_size(50) // Geri bildirim tarihçesi boyutu
{
    LOG_DEFAULT(LogLevel::INFO, "IntentLearner: Initialized.");
}

std::string IntentLearner::resolveIntent(const std::vector<IntentSignal>& intentSignals) {
    for (const auto& s : intentSignals) {
    // LLaMA sinyallerini kontrol et, güveni yüksekse onu seç
        if (s.source == "llama" && s.confidence > 0.80f) { // Eşik değeri ayarlanabilir
            LOG_DEFAULT(LogLevel::DEBUG, "IntentLearner: LLaMA tabanlı niyet çözüldü: " << s.intent << " (Güven: " << s.confidence << ")");
            return s.intent;
        }
    }

    // Aksi halde, ilk FastText sinyalini kullan (veya varsayılan olarak)
    for (const auto& s : intentSignals) {
        if (s.source == "fasttext") {
            LOG_DEFAULT(LogLevel::DEBUG, "IntentLearner: FastText tabanlı niyet çözüldü: " << s.intent << " (Güven: " << s.confidence << ")");
            return s.intent;
        }
    }
    
    // Hiçbir niyet çözülemezse Undefined döndür
    LOG_DEFAULT(LogLevel::WARNING, "IntentLearner: Hibrit analizden niyet çözülemedi, Undefined döndürüldü.");
    return "Undefined"; 
}

void IntentLearner::set_learning_rate(float rate) {
    learning_rate = std::max(0.0001f, std::min(1.0f, rate)); // Öğrenme hızını sınırla
    LOG_DEFAULT(LogLevel::INFO, "IntentLearner: Öğrenme hızı ayarlandı: " << learning_rate);
}

void IntentLearner::process_feedback(const DynamicSequence& sequence, UserIntent predicted_intent,
                                     const std::deque<AtomicSignal>& recent_signals) {
    // Burada örtük geri bildirim işlenir.
    // Örneğin, kullanıcı eylemlerinin gecikmesi, doğruluk oranı vb.
    float implicit_feedback_score = 0.5f; // Varsayılan nötr geri bildirim
    
    // Basit bir örnek: Hızlı yazma niyeti varsa ve kullanıcı uzun süre duraklamadıysa, olumlu geri bildirim.
    if (predicted_intent == UserIntent::FastTyping && sequence.event_count > 5) {
        // Simülatif bir gecikme analizi:
        if (!recent_signals.empty() && recent_signals.size() > 1) {
            long long last_signal_time = recent_signals.back().timestamp_us;
            long long first_signal_time = recent_signals.front().timestamp_us;
            long long duration = last_signal_time - first_signal_time;
            
            // Ortalama olay başına süre (mikrosaniye)
            float avg_time_per_event = static_cast<float>(duration) / (recent_signals.size() - 1);
            
            if (avg_time_per_event < 200000) { // 200ms'den az ise hızlı
                implicit_feedback_score = 0.8f; // Olumlu
            } else if (avg_time_per_event > 500000) { // 500ms'den fazla ise yavaş
                implicit_feedback_score = 0.2f; // Olumsuz
            }
        }
    }

    implicit_feedback_history[predicted_intent].push_back(implicit_feedback_score);
    if (implicit_feedback_history[predicted_intent].size() > feedback_history_size) {
        implicit_feedback_history[predicted_intent].pop_front();
    }

    LOG_DEFAULT(LogLevel::DEBUG, "IntentLearner: Niyet '" << CerebrumLux::to_string(predicted_intent) << "' için örtük geri bildirim işlendi: " << implicit_feedback_score);

    // Duruma göre aksiyon önerme mekanizması buradan tetiklenebilir
    // suggestion_engine.suggest_action(predicted_intent, inferred_state, sequence);
}

void IntentLearner::process_explicit_feedback(UserIntent predicted_intent, AIAction action, bool approved, const DynamicSequence& sequence, AbstractState current_abstract_state) {
    user_profile_manager.add_explicit_action_feedback(predicted_intent, action, approved);

    float reward = approved ? 1.0f : -1.0f; // Onaylandıysa olumlu, reddedildiyse olumsuz ödül
    StateKey state = {predicted_intent, current_abstract_state};
    suggestion_engine.update_q_value(state, action, reward);

    // AI'ın niyet şablonu ağırlıklarını doğrudan etkilemek de mümkün
    float feedback_strength = user_profile_manager.get_personalized_feedback_strength(predicted_intent, action);
    adjust_template(predicted_intent, sequence, feedback_strength);

    LOG_DEFAULT(LogLevel::INFO, "IntentLearner: Açık geri bildirim işlendi. Niyet '" << CerebrumLux::to_string(predicted_intent)
                                  << "', Eylem '" << CerebrumLux::to_string(action) << "', Onaylandı: " << (approved ? "Evet" : "Hayır"));
}

bool IntentLearner::get_implicit_feedback_for_intent(UserIntent intent_id, std::deque<float>& history_out) const {
    auto it = implicit_feedback_history.find(intent_id);
    if (it != implicit_feedback_history.end()) {
        history_out = it->second;
        return true;
    }
    return false;
}

AbstractState IntentLearner::infer_abstract_state(const std::deque<AtomicSignal>& recent_signals) {
    // Sensör verilerine dayanarak soyut durumu çıkarım
    // Basit bir örnek:
    if (recent_signals.empty()) {
        return AbstractState::Idle;
    }
    
    // Ağ aktivitesini kontrol et
    bool network_active = false;
    for (const auto& signal : recent_signals) {
        if (signal.type == SensorType::Network && signal.current_network_active) {
            network_active = true;
            break;
        }
    }

    if (network_active) {
        return AbstractState::SeekingInformation; // Basit örnek
    }
    
    // Klavye ve fare aktivitesini kontrol et
    int keyboard_events = 0;
    int mouse_events = 0;
    for (const auto& signal : recent_signals) {
        if (signal.type == SensorType::Keyboard) keyboard_events++;
        if (signal.type == SensorType::Mouse) mouse_events++;
    }

    if (keyboard_events > 10 || mouse_events > 20) { // Yüksek aktivite
        return AbstractState::ProcessingInput;
    }

    return AbstractState::NormalOperation; // Varsayılan
}

void IntentLearner::evaluate_implicit_feedback(UserIntent current_intent, AbstractState current_abstract_state) {
    // Detaylı örtük geri bildirim değerlendirme lojiği
    LOG_DEFAULT(LogLevel::DEBUG, "IntentLearner: Örtük geri bildirim değerlendiriliyor.");
}

void IntentLearner::adjust_template(UserIntent intent_id, const DynamicSequence& sequence, float feedback_strength) {
    // Geri bildirim gücüne göre niyet şablonu ağırlıklarını ayarla
    std::vector<float> current_weights = intent_analyzer.get_intent_weights(intent_id);
    if (current_weights.empty()) return;

    // Geri bildirim gücüne göre ağırlıkları yumuşakça ayarla
    float adjustment_factor = learning_rate * feedback_strength;

    for (size_t i = 0; i < current_weights.size(); ++i) {
        current_weights[i] += adjustment_factor * (sequence.statistical_features_vector[i] - current_weights[i]);
    }
    intent_analyzer.update_template_weights(intent_id, current_weights);
    LOG_DEFAULT(LogLevel::DEBUG, "IntentLearner: Niyet '" << CerebrumLux::to_string(intent_id) << "' şablonu ayarlandı.");
}

void IntentLearner::adjust_action_score(UserIntent intent_id, AIAction action, float score_change) {
    intent_analyzer.update_action_success_score(intent_id, action, score_change);
    LOG_DEFAULT(LogLevel::DEBUG, "IntentLearner: Niyet '" << CerebrumLux::to_string(intent_id) << "' için eylem '" << CerebrumLux::to_string(action) << "' puanı ayarlandı.");
}

float IntentLearner::calculate_sensor_contribution(SensorType type) const {
    // Sensör katkısını hesaplama lojiği
    auto it = sensor_performance_contribution.find(type);
    if (it != sensor_performance_contribution.end()) {
        return it->second;
    }
    return 0.0f; // Varsayılan
}

} // namespace CerebrumLux
