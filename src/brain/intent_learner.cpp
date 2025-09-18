#include "intent_learner.h" // Kendi başlık dosyasını dahil et
#include "../core/utils.h"       // intent_to_string
#include "../core/logger.h"      // LOG makrosu için
#include "../data_models/dynamic_sequence.h" // DynamicSequence için
#include "intent_analyzer.h"     // IntentAnalyzer için
#include "../sensors/atomic_signal.h"       // AtomicSignal için
#include "../communication/suggestion_engine.h" // SuggestionEngine için eklendi
#include "../brain/autoencoder.h" // CryptofigAutoencoder::LATENT_DIM için eklendi
#include "../user/user_profile_manager.h" // UserProfileManager için eklendi
// #include "../communication/natural_language_processor.h" // KALDIRILDI: NaturalLanguageProcessor için eklendi
#include <numeric> // std::accumulate
#include <algorithm> // std::min, std::max
#include <cmath>     // std::abs
#include <iomanip>   // std::fixed, std::setprecision
#include <iostream>  // std::cout, std::cerr
#include <sstream>   // std::stringstream için

// === IntentLearner Implementasyonlari ===

// Kurucu güncellendi - NaturalLanguageProcessor referansı kaldırıldı
IntentLearner::IntentLearner(IntentAnalyzer& analyzer_ref, SuggestionEngine& suggester_ref, UserProfileManager& user_profile_manager_ref)
    : analyzer(analyzer_ref), suggester(suggester_ref), user_profile_manager(user_profile_manager_ref), learning_rate(0.01f) {}

void IntentLearner::process_feedback(const DynamicSequence& sequence, UserIntent predicted_intent, const std::deque<AtomicSignal>& recent_signals) {
    // Kullanıcı profiline niyet geçmişi ekle
    user_profile_manager.add_intent_history_entry(predicted_intent, sequence.last_updated_us);

    // Geri bildirim gücünü hesapla (örneğin, tahmin edilen niyetin güvenine göre)
    float feedback_strength = 0.0f; // Varsayılan olarak sıfır

    // Eğer tahmin edilen niyet Unknown ise ve güçlü bir potansiyel niyet varsa
    if (predicted_intent == UserIntent::Unknown) {
        UserIntent best_potential_known_intent = UserIntent::Unknown;
        float max_potential_score = -std::numeric_limits<float>::max();

        for (const auto& tmpl : analyzer.intent_templates) {
            if (tmpl.id != UserIntent::Unknown) {
                float score = 0.0f;
                // CryptofigAutoencoder::LATENT_DIM kullanılarak weights boyutu kontrolü
                if (tmpl.weights.size() != CryptofigAutoencoder::LATENT_DIM) {
                    LOG_DEFAULT(LogLevel::ERR_CRITICAL, "IntentLearner::process_feedback: Niyet şablonu ağırlık boyutu latent kriptofig boyutuyla uyuşmuyor! Niyet: " << intent_to_string(tmpl.id) << ".\n");
                    continue; // Bu şablonu atla
                }
                for (size_t i = 0; i < tmpl.weights.size(); ++i) {
                    score += tmpl.weights[i] * sequence.latent_cryptofig_vector[i];
                }
                if (score > max_potential_score) {
                    max_potential_score = score;
                    best_potential_known_intent = tmpl.id;
                }
            }
        }

        if (best_potential_known_intent != UserIntent::Unknown && max_potential_score > analyzer.confidence_threshold_for_known_intent) {
            LOG_DEFAULT(LogLevel::INFO, "[AI-Ogrenen] 'Bilinmiyor' olarak tahmin edildi, ancak güçlü potansiyel niyet '" << intent_to_string(best_potential_known_intent) << "' (geri bildirim: " << feedback_strength << ") bulundu. Bu niyet için ayar yapılıyor.\n");
            // Bu durumda, best_potential_known_intent için ağırlıkları ayarla
            std::vector<float> current_weights = analyzer.get_intent_weights(best_potential_known_intent);
            if (current_weights.size() != sequence.latent_cryptofig_vector.size()) { // Boyut kontrolü
                 LOG_DEFAULT(LogLevel::ERR_CRITICAL, "IntentLearner::process_feedback: Current weights boyutu latent cryptofig boyutuyla uyuşmuyor! Ayarlama atlanıyor.\n");
            } else {
                for (size_t i = 0; i < current_weights.size(); ++i) {
                    current_weights[i] += learning_rate * (sequence.latent_cryptofig_vector[i] - current_weights[i]);
                    current_weights[i] = std::min(5.0f, std::max(-5.0f, current_weights[i]));
                }
                analyzer.update_template_weights(best_potential_known_intent, current_weights);
            }
        } else {
            // Gerçekten Unknown ise, öğrenme oranını düşür veya hiçbir şey yapma
            feedback_strength = 0.0f;
            LOG_DEFAULT(LogLevel::INFO, "[AI-Ogrenen] Niyet 'Bilinmiyor' için hesaplanan geri bildirim gucu: " << feedback_strength << ". (Net potansiyel niyet bulunamadı.)\n");
        }
    }
    else {
        // Bilinen bir niyet tahmin edildiyse, bu niyet için ağırlıkları ayarla
        feedback_strength = 1.0f; // Pozitif geri bildirim
        LOG_DEFAULT(LogLevel::INFO, "[AI-Ogrenen] Niyet '" << intent_to_string(predicted_intent) << "' için hesaplanan geri bildirim gucu: " << feedback_strength << "\n");
        std::vector<float> current_weights = analyzer.get_intent_weights(predicted_intent);
        if (current_weights.size() != sequence.latent_cryptofig_vector.size()) { // Boyut kontrolü
             LOG_DEFAULT(LogLevel::ERR_CRITICAL, "IntentLearner::process_feedback: Current weights boyutu latent cryptofig boyutuyla uyuşmuyor! Ayarlama atlanıyor.\n");
        } else {
            for (size_t i = 0; i < current_weights.size(); ++i) {
                current_weights[i] += learning_rate * (sequence.latent_cryptofig_vector[i] - current_weights[i]);
                current_weights[i] = std::min(5.0f, std::max(-5.0f, current_weights[i]));
            }
            analyzer.update_template_weights(predicted_intent, current_weights);
        }
    }

    // Implicit geri bildirim metriklerini topla ve işle
    AbstractState current_abstract_state = infer_abstract_state(recent_signals);
    user_profile_manager.add_state_history_entry(current_abstract_state, sequence.last_updated_us); // Kullanıcı profiline durum geçmişi ekle
    evaluate_implicit_feedback(predicted_intent, current_abstract_state);
}

void IntentLearner::evaluate_implicit_feedback(UserIntent current_intent, AbstractState current_abstract_state) {
    double implicit_feedback_score = 0.0; // Varsayılan nötr geri bildirim

    // AbstractState'e göre genel bir modifikasyon yapısı
    switch (current_abstract_state) {
        case AbstractState::PowerSaving:
            // Enerji tasarrufu modundayken farklı niyetler nasıl etkilenmeli?
            if (current_intent == UserIntent::Gaming ||
                current_intent == UserIntent::VideoEditing ||
                current_intent == UserIntent::CreativeWork) { // Yüksek performans gerektiren niyetler
                implicit_feedback_score = -0.5; // Güç tasarrufunda kötü performans -> negatif geri bildirim
                LOG_DEFAULT(LogLevel::INFO, "[AI-Ogrenen] Güç Tasarrufu modunda Yüksek Performans Niyeti algılandı. Negatif dolaylı geri bildirim uygulandı.");
            } else if (current_intent == UserIntent::MediaConsumption) { // Video izleme gibi, duruma göre değişebilir
                // Daha detaylı kontrol: çözünürlük düşükse nötr, yüksekse hafif negatif olabilir.
                // Şimdilik hafif negatif varsayalım, kullanıcı deneyimi tam olmayabilir.
                implicit_feedback_score = -0.1;
                LOG_DEFAULT(LogLevel::INFO, "[AI-Ogrenen] Güç Tasarrufu modunda Medya Tüketimi Niyeti algılandı. Hafif negatif dolaylı geri bildirim uygulandı.");
            } else if (current_intent == UserIntent::Browsing ||
                       current_intent == UserIntent::Reading ||
                       current_intent == UserIntent::GeneralInquiry) { // Düşük güç gerektiren niyetler
                implicit_feedback_score = 0.0; // Nötr veya hafif pozitif (eğer kullanıcı uzun pil ömrüne değer veriyorsa)
                LOG_DEFAULT(LogLevel::INFO, "[AI-Ogrenen] Güç Tasarrufu modunda Düşük Performans Niyeti algılandı. Nötr dolaylı geri bildirim uygulandı.");
            }
            break;

        case AbstractState::HighPerformance:
            // Yüksek performans modundayken niyetler için pozitif geri bildirim
            if (current_intent == UserIntent::Gaming ||
                current_intent == UserIntent::VideoEditing ||
                current_intent == UserIntent::CreativeWork) {
                implicit_feedback_score = 0.3; // Yüksek performansta iyi deneyim -> pozitif geri bildirim
            }
            break;

        case AbstractState::FaultyHardware: // Örneğin arızalı donanım durumu
             if (current_intent != UserIntent::None) { // Herhangi bir niyet için genel olarak negatif
                implicit_feedback_score = -0.7; // Donanım arızası tüm niyetleri olumsuz etkiler
             }
             break;
        // ... Diğer AbstractState'ler ve onların UserIntent'ler üzerindeki etkileri buraya eklenebilir

        default:
            // Varsayılan durum: Özel bir durum yoksa geri bildirim nötr kalır veya başka bir mantık uygulanır
            implicit_feedback_score = 0.0;
            break;
    }

    LOG_DEFAULT(LogLevel::DEBUG, "[AI-Ogrenen] Niyet: " << static_cast<int>(current_intent) <<
                         ", Durum: " << static_cast<int>(current_abstract_state) <<
                         ", Dolaylı Geri Bildirim Puanı: " << implicit_feedback_score);

    // implicit_feedback_history'yi güncelle
    implicit_feedback_history[current_intent].push_back(implicit_feedback_score);
    if (implicit_feedback_history[current_intent].size() > feedback_history_size) {
        implicit_feedback_history[current_intent].pop_front();
    }
}

AbstractState IntentLearner::infer_abstract_state(const std::deque<AtomicSignal>& recent_signals) {
    // Sinyallerden AbstractState çıkarmak için basit bir mantık
    // Daha karmaşık bir çıkarım mekanizması burada geliştirilebilir.
    // Şimdilik, en baskın AbstractState'i belirlemeye çalışalım.

    std::map<AbstractState, float> state_scores;
    state_scores[AbstractState::None] = 0.0f; // Varsayılan durum

    // Sinyal türlerine göre ağırlıklar (örnek değerler, orijinalden alınmıştır)
    const float BRIGHTNESS_WEIGHT = 0.1f;
    const float BATTERY_WEIGHT = 0.1f;
    // Diğer sensör tipleri için de benzer ağırlıklar eklenebilir

    for (const auto& sig : recent_signals) {
        switch (sig.sensor_type) {
            case SensorType::Display: {
                float brightness = sig.display_brightness;
                if (brightness < 0.3f) {
                    state_scores[AbstractState::PowerSaving] += BRIGHTNESS_WEIGHT * 0.5f;
                }
                // Yüksek parlaklık için HighPerformance durumu da eklenebilir
                else if (brightness > 0.8f) {
                    state_scores[AbstractState::HighPerformance] += BRIGHTNESS_WEIGHT * 0.3f;
                }
                break;
            }
            case SensorType::Battery: {
                float change = sig.battery_percentage;
                if (change < -0.05f) { // %5'ten fazla düşüş
                    state_scores[AbstractState::FaultyHardware] += BATTERY_WEIGHT * 0.7f;
                }
                break;
            }
            // Diğer sensör tipleri için AbstractState çıkarım mantığı buraya eklenebilir
            // Örneğin:
            // case SensorType::Temperature: {
            //     float temp = sig.value;
            //     if (temp > 80) { // Cihaz çok sıcak
            //         state_scores[AbstractState::FaultyHardware] += 0.1f;
            //     }
            //     break;
            // }
            // case SensorType::AmbientLight: {
            //     float light = sig.value;
            //     if (light < 50) { // Karanlık ortam
            //         state_scores[AbstractState::PowerSaving] += 0.05f;
            //     }
            //     break;
            // }
            // case SensorType::HeartRate: {
            //     float hr = sig.value;
            //     if (hr > 100) { // Yüksek kalp atış hızı
            //         state_scores[AbstractState::Distracted] += 0.05f; // Stres
            //     }
            //     break;
            // }
            default:
                break;
        }
    }

    // En yüksek puana sahip AbstractState'i bul
    AbstractState inferred_state = AbstractState::None;
    float max_score = 0.0f;

    for (const auto& pair : state_scores) {
        if (pair.second > max_score) {
            max_score = pair.second;
            inferred_state = pair.first;
        }
    }

    LOG_DEFAULT(LogLevel::DEBUG, "[AI-Ogrenen] Sinyallerden çıkarılan AbstractState: " << static_cast<int>(inferred_state) << " (Puan: " << max_score << ")");
    return inferred_state;
}

void IntentLearner::adjust_learning_rate(float rate) {
    learning_rate = std::min(0.1f, std::max(0.001f, rate)); // Öğrenme oranını belirli sınırlar içinde tut
}

void IntentLearner::process_explicit_feedback(UserIntent predicted_intent, AIAction action, bool approved, const DynamicSequence& sequence, AbstractState current_abstract_state) {
    float reward = approved ? 1.0f : -1.0f; // Onaylandıysa pozitif, reddedildiyse negatif ödül

    // current_state için StateKey oluştur
    StateKey state_key = {predicted_intent, current_abstract_state};

    // SuggestionEngine'ın Q-değerini güncelle
    suggester.update_q_value(state_key, action, reward);

    // Kullanıcı profiline açık geri bildirimi kaydet
    // UserProfileManager'da bir metod (örn. add_explicit_action_feedback) ekleyebiliriz.
    // Şimdilik, sadece IntentLearner'ın kendi explicit_feedback_history'sini güncelleyelim.
    explicit_feedback_history[predicted_intent].push_back(reward); 
    if (explicit_feedback_history[predicted_intent].size() > feedback_history_size) {
        explicit_feedback_history[predicted_intent].pop_front();
    }

    if (approved) {
        LOG_DEFAULT(LogLevel::INFO, "[AI-Ogrenen] Kullanıcıdan açık geri bildirim: Niyet '" << intent_to_string(predicted_intent) <<
                             "', Eylem '" << static_cast<int>(action) << "' ONAYLANDI. Öğrenme güçlendiriliyor.");
    } else {
        LOG_DEFAULT(LogLevel::INFO, "[AI-Ogrenen] Kullanıcıdan açık geri bildirim: Niyet '" << intent_to_string(predicted_intent) <<
                             "', Eylem '" << static_cast<int>(action) << "' REDDEDİLDİ. Öğrenme zayıflatılıyor.");
    }

}


// --- EK METOTLAR (Boş bırakıldı veya daha sonra geliştirilecek) ---

void IntentLearner::processMessages() {
    // Mesaj kuyruğunu işleme mantığı buraya gelecek
}

void IntentLearner::update_action_success_score(UserIntent intent, AIAction next_action, float success_score) {
    // Eylem başarı puanını güncelleme mantığı buraya gelecek
}

void IntentLearner::self_adjust_learning_rate(float adjustment_factor) {
    // Öğrenme oranını otomatik ayarlama mantığı buraya gelecek
}

void IntentLearner::evaluate_and_meta_adjust() {
    // Meta ayarlama mantığı buraya gelecek
}

void IntentLearner::adjust_template(UserIntent intent_id, const DynamicSequence& sequence, float feedback_strength) {
    // Niyet şablonlarını ayarlama mantığı buraya gelecek
}

void IntentLearner::adjust_action_score(UserIntent intent_id, AIAction action, float score_change) {
    // Eylem skorlarını ayarlama mantığı buraya gelecek
}