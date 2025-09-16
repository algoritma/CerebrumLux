#include <iostream>
#include <iomanip>
#include <locale>
#include <string>
#include <cmath>
#include <limits> // std::numeric_limits için
#include <cerrno> // errno için
#include <fstream> // std::ofstream için
#include <vector>  // std::vector için
#include <sstream> // std::stringstream için
#include <numeric> // std::accumulate için
#include <algorithm> // std::tolower için
#include <cctype>    // std::tolower için (bazı derleyicilerde <algorithm> yeterli olmayabilir)

#ifdef _WIN32
#include <Windows.h> // SetConsoleOutputCP, SetConsoleCP için
#include <io.h>      // _setmode için
#include <fcntl.h>   // _O_U8TEXT için
#endif

// Cerebrum Lux modüllerinin başlık dosyaları
#include "sensors/atomic_signal.h"
#include "core/enums.h"
#include "core/utils.h" // intent_to_string, abstract_state_to_string, convert_wstring_to_string, log_level_to_string (eğer utils.h'de ise)
#include "core/logger.h" // LOG_DEFAULT için
#include "sensors/simulated_processor.h"
#include "data_models/sequence_manager.h"
#include "brain/intent_analyzer.h"
#include "brain/intent_learner.h"
#include "brain/prediction_engine.h"
#include "brain/autoencoder.h"
#include "brain/cryptofig_processor.h"
#include "communication/ai_insights_engine.h"
#include "planning_execution/goal_manager.h"
#include "planning_execution/planner.h"
#include "communication/response_engine.h"
#include "communication/suggestion_engine.h"


// AI_LOG_FILE ve diğer sabitlerin tanımları (global alanda)
const std::string AI_MEMORY_FILE = "cerebrum_lux_memory.bin";
const std::string AI_STATE_GRAPH_FILE = "cerebrum_lux_state_graph.bin";
const std::string AI_LOG_FILE = "cerebrum_lux_log.txt";
const std::string AI_AUTOENCODER_WEIGHTS_FILE = "cerebrum_lux_autoencoder_weights.bin";


int main() {
    // Konsol ve Yerel Ayar Başlatma (Kesin Çözüm - Türkçe Karakter ve Stabilite için)
#ifdef _WIN32
    // Konsolun kod sayfasını UTF-8'e ayarla
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    // C Runtime Library streamlerini UTF-8 moduna ayarla
    // Bu, std::cout, std::cin, std::cerr için Türkçe karakter desteğini sağlamalıdır.
    _setmode(_fileno(stdout), _O_U8TEXT);
    _setmode(_fileno(stdin), _O_U8TEXT);
    _setmode(_fileno(stderr), _O_U8TEXT);
    
    // C++ streamlerini hızlı ve bağımsız yap (Performans için, Türkçe karakterlerle doğrudan ilgili değil)
    std::ios_base::sync_with_stdio(false);
    std::cin.tie(nullptr);

#endif // _WIN32

    Logger::get_instance().init(LogLevel::INFO, AI_LOG_FILE);
    LOG_DEFAULT(LogLevel::INFO, "Log dosyası açıldı: " << AI_LOG_FILE << "\n");

    try { // Start of try block for general stability
        // AI bileşenlerini başlat
        SimulatedAtomicSignalProcessor simulatedSignalProcessor;
        SequenceManager sequenceManager;

        IntentAnalyzer analyzer;
        IntentLearner learner(analyzer);

        PredictionEngine predictor(analyzer, sequenceManager);
        SuggestionEngine suggester(analyzer);

        CryptofigAutoencoder autoencoder;
        autoencoder.load_weights(AI_AUTOENCODER_WEIGHTS_FILE);
        CryptofigProcessor cryptofig_processor(analyzer, autoencoder); // CryptofigProcessor'ın autoencoder referansı aldığını varsayarak

        AIInsightsEngine insights_engine(analyzer, learner, predictor, autoencoder, cryptofig_processor);

        GoalManager goal_manager(insights_engine); // GoalManager'ın AIInsightsEngine referansı aldığını varsayarak

        Planner planner(analyzer, suggester, goal_manager, predictor, insights_engine); // Planner'ın AIInsightsEngine referansı aldığını varsayarak
        ResponseEngine responder(analyzer, goal_manager, insights_engine); // ResponseEngine'ın AIInsightsEngine referansı aldığını varsayarak

        // AI hafızalarını ve durum grafiklerini yükle
        analyzer.load_memory(AI_MEMORY_FILE);
        predictor.load_state_graph(AI_STATE_GRAPH_FILE);
        // autoencoder.load_weights(AI_AUTOENCODER_WEIGHTS_FILE); // Zaten yukarıda yapıldı.

        // Sinyal yakalamayı başlat
        simulatedSignalProcessor.start_capture();

        // Kullanıcı arayüzü mesajları
        std::cout << "\n--- Cerebrum Lux Başlatıldı ---\n";
        std::cout << "    - Mevcut Log Seviyesi: " << Logger::get_instance().level_to_string(Logger::get_instance().get_level()) << "\n";
        std::cout << "  - Raporlama Seviyesini Değiştirmek İçin 'L' tuşuna basın. \n";
        std::cout << "    (0=SILENT, 1=ERROR, 2=WARNING, 3=INFO, 4=DEBUG, 5=TRACE)\n";
        std::cout << "  - Durum Raporu İçin 'S' veya 'R' tuşuna basın. (Yeni satırda girilmelidir)\n";
        std::cout << "  - Çıkış İçin 'Q' tuşuna basın. (Yeni satırda girilmelidir)\n";
        std::cout << "------------------------------\n";

        UserIntent last_predicted_intent = UserIntent::Unknown;
        AIAction last_suggested_action = AIAction::None;


        while (true) {

            if (last_suggested_action != AIAction::None) {
                std::cout << "Önerilen eylem '" << suggester.action_to_string(last_suggested_action) << "' başarılı oldu mu? (E/H): ";
                char feedback_char_raw;
                std::cin >> feedback_char_raw;
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                char feedback_char = std::tolower(feedback_char_raw);
                bool approved = (feedback_char == 'e' || feedback_char == 'E');
                learner.process_explicit_feedback(last_predicted_intent, last_suggested_action, approved, *sequenceManager.current_sequence);
                std::cout << "Geri bildirim kaydedildi. AI öğreniyor...\n";
                last_suggested_action = AIAction::None;
            }

            std::cout << "Klavye Girişi (Q ile çıkış): ";
            std::string user_input_line;
            std::getline(std::cin, user_input_line);

            if (user_input_line.empty() && std::cin.eof()) {
                break;
            } else if (user_input_line.empty()) {
                continue;
            }

            if (user_input_line.length() == 1) {
                char ch_raw = user_input_line[0]; // Tek karakter için user_input_line[0] kullanın
                char ch = std::tolower(ch_raw);
                if (ch == 'q') {
                    break;
                } else if (ch == 's' || ch == 'r') {
                    std::cout << "\n--- CEREBRUM LUX: Durum Raporu ---\n";
                    std::cout << std::fixed << std::setprecision(4);

                    std::cout << "    - Niyet Belirleme Eşiği: " << analyzer.confidence_threshold_for_known_intent << "\n";
                    std::cout << "    - Öğrenme Oranı: " << learner.get_learning_rate() << "\n";

                    float total_implicit_performance = 0.0f;
                    int implicit_count = 0;
                    const auto& history_map = learner.get_implicit_feedback_history();
                    for (int i = static_cast<int>(UserIntent::FastTyping); i < static_cast<int>(UserIntent::Count); ++i) { // UserIntent::Count'a kadar döngü
                        UserIntent intent_id = static_cast<UserIntent>(i);
                        auto it = history_map.find(intent_id);

                        if (it != history_map.end() && !it->second.empty()) {
                            float avg = std::accumulate(it->second.begin(), it->second.end(), 0.0f) / it->second.size();
                            std::cout << "      - Niyet '" << intent_to_string(intent_id) << "' Ort. Örtük Performans: " << avg << "\n";
                            total_implicit_performance += avg;
                            implicit_count++;
                        }
                    }
                    if (implicit_count > 0) {
                        std::cout << "    - Toplam Ort. Örtük Performans (Bilinen Niyetler): " << total_implicit_performance / implicit_count << "\n";
                    } else {
                        std::cout << "    - Henüz bilinen niyetler için yeterli örtük performans verisi yok.\n";
                    }
                    std::cout << "    - Mevcut Log Seviyesi: " << Logger::get_instance().level_to_string(Logger::get_instance().get_level()) << "\n";
                    std::cout << "    - AI, niyetleri başarıyla tahmin etmeye başlamıştır. Öğrenme devam ediyor.\n";
                    std::cout << "Rapor tamamlandı. Devam etmek için herhangi bir tuşa basın (yeni girdi): ";
                    continue;
                } else if (ch == 'l') {
                    std::cout << "Yeni Log Seviyesi Seçin (0=SILENT, 1=ERROR, 2=WARNING, 3=INFO, 4=DEBUG, 5=TRACE, Mevcut: " << Logger::get_instance().level_to_string(Logger::get_instance().get_level()) << "): ";
                    int level_int;
                    std::cin >> level_int;
                    if (std::cin.fail() || level_int < static_cast<int>(LogLevel::SILENT) || level_int > static_cast<int>(LogLevel::TRACE)) {
                        std::cin.clear();
                        std::cout << "Geçersiz log seviyesi. Mevcut seviye korunuyor.\n";
                    } else {
                        Logger::get_instance().init(static_cast<LogLevel>(level_int), AI_LOG_FILE); // Re-init with new level
                        std::cout << "Log Seviyesi '" << Logger::get_instance().level_to_string(static_cast<LogLevel>(level_int)) << "' olarak ayarlandı.\n";
                    }
                    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                    continue;
                }
            }

            static std::random_device rd_main;
            static std::mt19937 gen_main(rd_main());

            int num_other_signals_to_simulate = static_cast<int>(std::round(learner.get_desired_other_signals_multiplier() * std::uniform_int_distribution<>(1, 3)(gen_main)));
            num_other_signals_to_simulate = std::max(1, num_other_signals_to_simulate);

            for (int i = 0; i < num_other_signals_to_simulate; ++i) {
                AtomicSignal other_signal = simulatedSignalProcessor.capture_next_signal();
                LOG_DEFAULT(LogLevel::DEBUG, "Diğer sensör sinyali oluşturuldu (tip: " << static_cast<int>(other_signal.sensor_type) << "). Manager'a ekleniyor.\n");
                sequenceManager.add_signal(other_signal, cryptofig_processor); // Add cryptofig_processor to add_signal
                LOG_DEFAULT(LogLevel::DEBUG, "Diğer sensör sinyali manager'a eklendi.\n");
            }

            bool sequence_updated_in_this_line = false;

            for (char ch_raw : user_input_line) {
                AtomicSignal keyboard_signal = simulatedSignalProcessor.create_keyboard_signal(ch_raw);
                LOG_DEFAULT(LogLevel::DEBUG, "Klavye sinyali manager'a ekleniyor (karakter: '" << ch_raw << "').\n");
                if (sequenceManager.add_signal(keyboard_signal, cryptofig_processor)) { // Add cryptofig_processor to add_signal
                    sequence_updated_in_this_line = true;
                }
                LOG_DEFAULT(LogLevel::DEBUG, "Klavye sinyali manager'a eklendi. Sequence güncellendi mi? " << (sequence_updated_in_this_line ? "Evet" : "Hayır") << ".\n");
            }

            if (sequence_updated_in_this_line) {
                std::cout << "\n--- CEREBRUM LUX: Anlık Dizi Özeti ---\n";
                std::cout << std::fixed << std::setprecision(4);

                std::cout << "Son Güncelleme Zamanı: " << sequenceManager.current_sequence->last_updated_us << " us\n";
                std::cout << "Ort. Tuş Aralığı: " << sequenceManager.current_sequence->avg_keystroke_interval / 1000.0f << " ms\n";
                std::cout << "Tuş Değişkenliği: " << sequenceManager.current_sequence->keystroke_variability / 1000.0f << " ms\n";
                std::cout << "Alfanumerik Oran: " << sequenceManager.current_sequence->alphanumeric_ratio << "\n";
                std::cout << "Kontrol Tuşu Sıklığı: " << sequenceManager.current_sequence->control_key_frequency << "\n";

                std::cout << "Fare Hareketi Yoğunluğu: " << sequenceManager.current_sequence->mouse_movement_intensity << " (Piksel/Örnek)\n";
                std::cout << "Fare Tıklama Sıklığı: " << sequenceManager.current_sequence->mouse_click_frequency << " (Tıklama/Örnek)\n";
                std::cout << "Ort. Ekran Parlaklığı: " << sequenceManager.current_sequence->avg_brightness << " (0-255)\n";
                std::cout << "Batarya Değişim Oranı: " << sequenceManager.current_sequence->battery_status_change << " (%)\n";
                std::cout << "Ağ Aktivite Seviyesi: " << sequenceManager.current_sequence->network_activity_level << " (Kbps)\n";

                std::cout << "Aktif Uygulama Hash: " << sequenceManager.current_sequence->current_app_hash << "\n";

                std::cout << "İstatistiksel Özellik Vektörü (Boyut " << sequenceManager.current_sequence->statistical_features_vector.size() << "): [";
                for (size_t i = 0; i < sequenceManager.current_sequence->statistical_features_vector.size(); ++i) {
                    std::cout << sequenceManager.current_sequence->statistical_features_vector[i];
                    if (i < sequenceManager.current_sequence->statistical_features_vector.size() - 1) {
                        std::cout << ", ";
                    }
                }
                std::cout << "]\n";

                std::cout << "Latent Kriptofig Vektörü (Boyut " << sequenceManager.current_sequence->latent_cryptofig_vector.size() << "): [";
                for (size_t i = 0; i < sequenceManager.current_sequence->latent_cryptofig_vector.size(); ++i) {
                    std::cout << std::fixed << std::setprecision(2) << sequenceManager.current_sequence->latent_cryptofig_vector[i];
                    if (i < sequenceManager.current_sequence->latent_cryptofig_vector.size() - 1) {
                        std::cout << ", ";
                    }
                }
                std::cout << "]\n";

                goal_manager.evaluate_and_set_goal(*sequenceManager.current_sequence);
                std::cout << "AI'ın Mevcut Hedefi: " << goal_to_string(goal_manager.get_current_goal()) << "\n";

                UserIntent current_predicted_intent = analyzer.analyze_intent(*sequenceManager.current_sequence);
                std::cout << "Tahmini Kullanıcı Niyeti: " << intent_to_string(current_predicted_intent) << "\n";

                AbstractState current_abstract_state = learner.infer_abstract_state(sequenceManager.get_signal_buffer_copy());
                std::cout << "Tahmini Soyut Durum: " << abstract_state_to_string(current_abstract_state) << "\n";

                learner.process_feedback(*sequenceManager.current_sequence, current_predicted_intent, sequenceManager.get_signal_buffer_copy());

                if (last_predicted_intent != UserIntent::Unknown && last_predicted_intent != current_predicted_intent &&
                    last_predicted_intent != UserIntent::None && current_predicted_intent != UserIntent::None) {
                    predictor.update_state_graph(last_predicted_intent, current_predicted_intent, *sequenceManager.current_sequence);
                }
                last_predicted_intent = current_predicted_intent;

                UserIntent next_predicted_intent = predictor.predict_next_intent(last_predicted_intent, *sequenceManager.current_sequence);
                if (next_predicted_intent != UserIntent::Unknown && next_predicted_intent != current_predicted_intent && next_predicted_intent != UserIntent::None) {
                    std::cout << "Olası Sonraki Niyet: " << intent_to_string(next_predicted_intent) << "\n";
                }

                AIAction suggested_action = suggester.suggest_action(current_predicted_intent, *sequenceManager.current_sequence);
                if (suggested_action != AIAction::None) {
                    std::cout << "AI Önerisi: " << suggester.action_to_string(suggested_action) << "\n";
                    last_suggested_action = suggested_action;
                }

                std::vector<ActionPlanStep> current_plan = planner.create_action_plan(current_predicted_intent, current_abstract_state, goal_manager.get_current_goal(), *sequenceManager.current_sequence);
                planner.execute_plan(current_plan);

                std::string ai_response = responder.generate_response(current_predicted_intent, current_abstract_state, goal_manager.get_current_goal(), *sequenceManager.current_sequence);
                if (!ai_response.empty()) {
                    std::cout << "AI: " << ai_response << "\n";
                }

                if (current_predicted_intent == UserIntent::Unknown) {
                    LOG_DEFAULT(LogLevel::DEBUG, "Unknown niyet tespit edildi, Autoencoder'a istatistiksel özellikler ile öğrenme sinyali gönderiliyor.\n");
                    std::cout << "AI: (Algı belirsizliği. Autoencoder yeni desenleri öğreniyor!)\n";
                }

                std::cout << "AI: (Anlık İstatistiksel Özellikler: ";
                for (size_t i = 0; i < sequenceManager.current_sequence->statistical_features_vector.size(); ++i) {
                    std::cout << std::fixed << std::setprecision(2) << sequenceManager.current_sequence->statistical_features_vector[i];
                    if (i < sequenceManager.current_sequence->statistical_features_vector.size() - 1) {
                        std::cout << ", ";
                    }
                }
                std::cout << ")\n";

                std::cout << "AI: (Anlık Latent Kriptofig: ";
                for (size_t i = 0; i < sequenceManager.current_sequence->latent_cryptofig_vector.size(); ++i) {
                    std::cout << std::fixed << std::setprecision(2) << sequenceManager.current_sequence->latent_cryptofig_vector[i];
                    if (i < sequenceManager.current_sequence->latent_cryptofig_vector.size() - 1) {
                        std::cout << ", ";
                    }
                }
                std::cout << ")\n";

                std::cout << "---------------------------------------------\n\n";
            }
        }

        simulatedSignalProcessor.stop_capture();
        analyzer.save_memory(AI_MEMORY_FILE);
        predictor.save_state_graph(AI_STATE_GRAPH_FILE);
        autoencoder.save_weights(AI_AUTOENCODER_WEIGHTS_FILE);

    } catch (const std::exception& e) { // Catch standard exceptions
        LOG_DEFAULT(LogLevel::ERR_CRITICAL, "Uygulama hatası: " << e.what() << "\n");
        std::cerr << "Uygulama hatası: " << e.what() << "\n";
    } catch (...) { // Catch all other exceptions
        LOG_DEFAULT(LogLevel::ERR_CRITICAL, "Bilinmeyen bir uygulama hatası oluştu.\n");
        std::cerr << "Bilinmeyen bir uygulama hatası oluştu.\n";
    }

    std::cout << "Çıkmak için Enter'a basın...";
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // Clear any leftover input
    std::cin.get(); // Wait for a key press

    return 0;
}