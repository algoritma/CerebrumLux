#include <iostream>
#include <iomanip> 
#include <locale>  
#include <string> 
#include <cmath> 
#include <limits> 
#include <cerrno> 
#include <fstream> 
#include <vector> // Added for std::vector
#include <sstream> // For std::stringstream
#include <numeric> // For std::accumulate
#include <algorithm> // For std::tolower
#include <atomic> // For std::atomic
#include <chrono> // For std::chrono
#include <thread> // For std::this_thread
#include <csignal> // For std::signal
#include <random>      // <-- eklendi: random_device, mt19937, uniform_int_distribution

// Windows-specific includes for console settings
#ifdef _WIN32
#include <windows.h>
#include <io.h>      // For _setmode
#include <fcntl.h>   // For _O_U8TEXT
#endif

// Cerebrum Lux modüllerinin başlık dosyaları
#include "sensors/atomic_signal.h"
#include "core/enums.h"
#include "core/utils.h" // intent_to_string etc.
#include "core/logger.h"
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

using namespace cerebrum;


// Dosya isimleri (constants)
const std::string AI_MEMORY_FILE = "cerebrum_lux_memory.bin";
const std::string AI_STATE_GRAPH_FILE = "cerebrum_lux_state_graph.bin";
const std::string AI_LOG_FILE = "cerebrum_lux_log.txt";
const std::string AI_AUTOENCODER_WEIGHTS_FILE = "cerebrum_lux_autoencoder_weights.bin";

// Global pointers for signal handler access
static SimulatedAtomicSignalProcessor* g_simulatedSignalProcessor = nullptr;
static IntentAnalyzer* g_analyzer = nullptr;
static PredictionEngine* g_predictor = nullptr;
static CryptofigAutoencoder* g_autoencoder = nullptr;

void signal_handler(int signum) {
    // Not: LOG_DEFAULT makrosu logger'ın hazır olmasını varsayar.
    // Signal anında güvenli olan işleri yap (çok ağır operasyonlardan kaçın).
    LOG_DEFAULT(LogLevel::INFO, "Sinyal alindi: " << signum << ". Uygulama kapatiliyor...");
    if (g_simulatedSignalProcessor) {
        try { g_simulatedSignalProcessor->stop_capture(); } catch(...) {}
    }
    if (g_analyzer) {
        try { g_analyzer->save_memory(AI_MEMORY_FILE); } catch(...) {}
    }
    if (g_predictor) {
        try { g_predictor->save_state_graph(AI_STATE_GRAPH_FILE); } catch(...) {}
    }
    if (g_autoencoder) {
        try { g_autoencoder->save_weights(AI_AUTOENCODER_WEIGHTS_FILE); } catch(...) {}
    }
    std::exit(signum);
}

int main(int argc, char **argv) {
    std::string logFile = AI_LOG_FILE;

    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "--log" && i + 1 < argc) {
            logFile = argv[++i];
        } else if (a == "--help" || a == "-h") {
            std::cout << "Kullanim: " << argv[0] << " [--log <dosya>]\n";
            return 0;
        }
    }

    try {
#ifdef _WIN32
        // Konsolu UTF-8 kod sayfasına geçir. _setmode(_O_U8TEXT) kullanmayın: std::cout bozulabilir.
        SetConsoleOutputCP(CP_UTF8);
        SetConsoleCP(CP_UTF8);
#endif
        std::ios_base::sync_with_stdio(false);
        std::cin.tie(nullptr);

        // Logger başlat
        Logger::get_instance().init(LogLevel::INFO, logFile);
        LOG_DEFAULT(LogLevel::INFO, "Log dosyası açıldı: " << logFile);

        // Sinyal handler kaydı (CTRL+C vb.)
        std::signal(SIGINT, signal_handler);
#ifdef SIGTERM
        std::signal(SIGTERM, signal_handler);
#endif

        // AI bileşenlerini başlat
        SimulatedAtomicSignalProcessor simulatedSignalProcessor;
        SequenceManager sequenceManager;

        IntentAnalyzer analyzer;
        IntentLearner learner(analyzer);

        PredictionEngine predictor(analyzer, sequenceManager);
        SuggestionEngine suggester(analyzer);

        CryptofigAutoencoder autoencoder;
        CryptofigProcessor cryptofig_processor(analyzer, autoencoder);

        AIInsightsEngine insights_engine(analyzer, learner, predictor, autoencoder, cryptofig_processor);

        GoalManager goal_manager(insights_engine);

        Planner planner(analyzer, suggester, goal_manager, predictor, insights_engine);
        ResponseEngine responder(analyzer, goal_manager, insights_engine);

        // Global pointer'ları ayarla
        g_simulatedSignalProcessor = &simulatedSignalProcessor;
        g_analyzer = &analyzer;
        g_predictor = &predictor;
        g_autoencoder = &autoencoder;

        // Yüklemeler (varsa)
        try { analyzer.load_memory(AI_MEMORY_FILE); } catch(...) { LOG_DEFAULT(LogLevel::WARNING, "Memory load hatasi (devam ediliyor)."); }
        try { predictor.load_state_graph(AI_STATE_GRAPH_FILE); } catch(...) { LOG_DEFAULT(LogLevel::WARNING, "State graph load hatasi (devam ediliyor)."); }

        simulatedSignalProcessor.start_capture();

        // UI Mesajları
        std::cout << "\n--- Cerebrum Lux Baslatildi ---\\n";
        std::cout << "    - Mevcut Log Seviyesi: " << Logger::get_instance().level_to_string(Logger::get_instance().get_level()) << "\n";
        std::cout << "  - Raporlama Seviyesini Degistirmek Icin 'L' tusuna basin.\n";
        std::cout << "    (0=SILENT, 1=ERROR, 2=WARNING, 3=INFO, 4=DEBUG, 5=TRACE)\n";
        std::cout << "  - Durum Raporu Icin 'S' veya 'R' tusuna basin. (Yeni satirda girin)\n";
        std::cout << "  - Cikis Icin 'Q' tusuna basin. (Yeni satirda girin)\n";
        std::cout << "------------------------------\n";


        UserIntent last_predicted_intent = UserIntent::Unknown;
        AIAction last_suggested_action = AIAction::None;

        // RNG hazırla
        static std::random_device rd_main;
        static std::mt19937 gen_main(rd_main());

        while (true) {
            std::cout << "Klavye Girisi (Q ile cikis): ";
            
            if (last_suggested_action != AIAction::None) {
                std::cout << "Onerilen eylem '" << suggester.action_to_string(last_suggested_action) << "' basarili oldu mu? (E/H): ";
                char feedback_char_raw;
                if (!(std::cin >> feedback_char_raw)) break;
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                char feedback_char = static_cast<char>(std::tolower(static_cast<unsigned char>(feedback_char_raw)));
                bool approved = (feedback_char == 'e');
                if (sequenceManager.current_sequence) {
                    learner.process_explicit_feedback(last_predicted_intent, last_suggested_action, approved, *sequenceManager.current_sequence);
                    std::cout << "Geri bildirim kaydedildi. AI ogreniyor...\n";
                } else {
                    LOG_DEFAULT(LogLevel::WARNING, "Geri bildirim islenemedi: current_sequence null.\n");
                }
                last_suggested_action = AIAction::None;
                continue; 
            }

            std::string user_input_line;
            if (!std::getline(std::cin, user_input_line)) break;
            if (user_input_line.empty()) continue;

            if (user_input_line.length() == 1) {
                char ch_raw = user_input_line[0];
                char ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch_raw)));
                if (ch == 'q') break;
                if (ch == 's' || ch == 'r') {
                    std::cout << "\n--- CEREBRUM LUX: Durum Raporu ---\\n";
                    std::cout << std::fixed << std::setprecision(4);
                    std::cout << "    - Niyet Belirleme Esiği: " << analyzer.confidence_threshold_for_known_intent << "\n";
                    std::cout << "    - Ogrenme Orani: " << learner.get_learning_rate() << "\n";

                    float total_implicit_performance = 0.0f;
                    int implicit_count = 0;
                    const auto& history_map = learner.get_implicit_feedback_history();
                    for (int i = static_cast<int>(UserIntent::FastTyping); i < static_cast<int>(UserIntent::Count); ++i) {
                        UserIntent intent_id = static_cast<UserIntent>(i);
                        auto it = history_map.find(intent_id);
                        if (it != history_map.end() && !it->second.empty()) {
                            float avg = std::accumulate(it->second.begin(), it->second.end(), 0.0f) / it->second.size();
                            std::cout << "      - Niyet '" << intent_to_string(intent_id) << "' Ort. Ortuk Performans: " << avg << "\n";
                            total_implicit_performance += avg;
                            implicit_count++;
                        }
                    }
                    if (implicit_count > 0) {
                        std::cout << "    - Toplam Ort. Ortuk Performans (Bilinen Niyetler): " << total_implicit_performance / implicit_count << "\n";
                    } else {
                        std::cout << "    - Henuz bilinen niyetler icin yeterli ortuk performans verisi yok.\n";
                    }
                    std::cout << "    - Mevcut Log Seviyesi: " << Logger::get_instance().level_to_string(Logger::get_instance().get_level()) << "\n";
                    std::cout << "Rapor tamamlandi. Devam etmek icin herhangi bir tusa basin (yeni girdi): ";
                    continue;
                } else if (ch == 'l') {
                    std::cout << "Yeni Log Seviyesi Secin (0-5, Mevcut: " << Logger::get_instance().level_to_string(Logger::get_instance().get_level()) << "): ";
                    int level_int = 0;
                    if (!(std::cin >> level_int)) {
                        std::cin.clear();
                        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                        std::cout << "Gecersiz input. Mevcut seviye korunuyor.\n";
                        continue;
                    }
                    if (level_int < static_cast<int>(LogLevel::SILENT) || level_int > static_cast<int>(LogLevel::TRACE)) {
                        std::cout << "Gecersiz log seviyesi. Mevcut seviye korunuyor.\n";
                    } else {
                        Logger::get_instance().init(static_cast<LogLevel>(level_int), logFile);
                        std::cout << "Log Seviyesi ayarlandi.\n";
                    }
                    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                    continue;
                }
            }

            int num_other_signals_to_simulate = static_cast<int>(std::round(learner.get_desired_other_signals_multiplier() * std::uniform_int_distribution<>(1, 3)(gen_main)));
            num_other_signals_to_simulate = std::max(1, num_other_signals_to_simulate);

            for (int i = 0; i < num_other_signals_to_simulate; ++i) {
                AtomicSignal other_signal = simulatedSignalProcessor.capture_next_signal();
                LOG_DEFAULT(LogLevel::DEBUG, "Diger sensor sinyali olusturuldu (tip: " << static_cast<int>(other_signal.sensor_type) << "). Manager'a ekleniyor.");
                sequenceManager.add_signal(other_signal, cryptofig_processor);
                LOG_DEFAULT(LogLevel::DEBUG, "Diger sensor sinyali manager'a eklendi.");
            }

            bool sequence_updated_in_this_line = false;
            for (char ch_raw : user_input_line) {
                AtomicSignal keyboard_signal = simulatedSignalProcessor.create_keyboard_signal(ch_raw);
                LOG_DEFAULT(LogLevel::DEBUG, "Klavye sinyali manager'a ekleniyor (karakter: '" << ch_raw << "').");
                if (sequenceManager.add_signal(keyboard_signal, cryptofig_processor)) {
                    sequence_updated_in_this_line = true;
                }
                LOG_DEFAULT(LogLevel::DEBUG, "Klavye sinyali manager'a eklendi. Sequence guncellendi mi? " << (sequence_updated_in_this_line ? "Evet" : "Hayir"));
            }

            if (sequence_updated_in_this_line && sequenceManager.current_sequence) {
                std::cout << "\n--- Cerebrum LUX: Anlik Dizi Ozeti ---\\n";
                std::cout << std::fixed << std::setprecision(4);

                std::cout << "Son Guncelleme Zamani: " << sequenceManager.current_sequence->last_updated_us << " us\n";
                std::cout << "Ort. Tus Araligi: " << sequenceManager.current_sequence->avg_keystroke_interval / 1000.0f << " ms\n";
                std::cout << "Tus Degiskenligi: " << sequenceManager.current_sequence->keystroke_variability / 1000.0f << " ms\n";
                std::cout << "Alfanumerik Oran: " << sequenceManager.current_sequence->alphanumeric_ratio << "\n";
                std::cout << "Kontrol Tusu Sikligi: " << sequenceManager.current_sequence->control_key_frequency << "\n";

                std::cout << "Fare Hareketi Yogunlugu: " << sequenceManager.current_sequence->mouse_movement_intensity << " (Piksel/Ornek)\n";
                std::cout << "Fare Tiklama Sikligi: " << sequenceManager.current_sequence->mouse_click_frequency << " (Tiklama/Ornek)\n";
                std::cout << "Ort. Ekran Parlakligi: " << sequenceManager.current_sequence->avg_brightness << " (0-255)\n";
                std::cout << "Batarya Degisim Orani: " << sequenceManager.current_sequence->battery_status_change << " (%)\n";
                std::cout << "Ag Aktivite Seviyesi: " << sequenceManager.current_sequence->network_activity_level << " (Kbps)\n";

                std::cout << "Aktif Uygulama Hash: " << sequenceManager.current_sequence->current_app_hash << "\n";

                std::cout << "İstatistiksel Ozellik Vektoru (Boyut " << sequenceManager.current_sequence->statistical_features_vector.size() << "): [";
                for (size_t i = 0; i < sequenceManager.current_sequence->statistical_features_vector.size(); ++i) {
                    std::cout << sequenceManager.current_sequence->statistical_features_vector[i];
                    if (i + 1 < sequenceManager.current_sequence->statistical_features_vector.size()) std::cout << ", ";
                }
                std::cout << "]\n";

                std::cout << "Latent Kriptofig Vektoru (Boyut " << sequenceManager.current_sequence->latent_cryptofig_vector.size() << "): [";
                for (size_t i = 0; i < sequenceManager.current_sequence->latent_cryptofig_vector.size(); ++i) {
                    std::cout << std::fixed << std::setprecision(2) << sequenceManager.current_sequence->latent_cryptofig_vector[i];
                    if (i + 1 < sequenceManager.current_sequence->latent_cryptofig_vector.size()) std::cout << ", ";
                }
                std::cout << "]\n";

                goal_manager.evaluate_and_set_goal(*sequenceManager.current_sequence);
                std::cout << "AI'ın Mevcut Hedefi: " << goal_to_string(goal_manager.get_current_goal()) << "\n";

                UserIntent current_predicted_intent = analyzer.analyze_intent(*sequenceManager.current_sequence);
                std::cout << "Tahmini Kullanici Niyeti: " << intent_to_string(current_predicted_intent) << "\n";

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
                    std::cout << "Olasi Sonraki Niyet: " << intent_to_string(next_predicted_intent) << "\n";
                }

                AIAction suggested_action = suggester.suggest_action(current_predicted_intent, *sequenceManager.current_sequence);
                if (suggested_action != AIAction::None) {
                    std::cout << "AI Onerisi: " << suggester.action_to_string(suggested_action) << "\n";
                    last_suggested_action = suggested_action;
                }

                std::vector<ActionPlanStep> current_plan = planner.create_action_plan(current_predicted_intent, current_abstract_state, goal_manager.get_current_goal(), *sequenceManager.current_sequence);
                planner.execute_plan(current_plan);

                std::string ai_response = responder.generate_response(current_predicted_intent, current_abstract_state, goal_manager.get_current_goal(), *sequenceManager.current_sequence);
                if (!ai_response.empty()) std::cout << "AI: " << ai_response << "\n";

                if (current_predicted_intent == UserIntent::Unknown) {
                    LOG_DEFAULT(LogLevel::DEBUG, "Unknown niyet tespit edildi, Autoencoder'a istatistiksel özellikler ile öğrenme sinyali gönderiliyor.");
                    std::cout << "AI: (Algı belirsizliği. Autoencoder yeni desenleri öğreniyor!)\n";
                }

                std::cout << "AI: (Anlık İstatistiksel Özellikler: ";
                for (size_t i = 0; i < sequenceManager.current_sequence->statistical_features_vector.size(); ++i) {
                    std::cout << std::fixed << std::setprecision(2) << sequenceManager.current_sequence->statistical_features_vector[i];
                    if (i + 1 < sequenceManager.current_sequence->statistical_features_vector.size()) std::cout << ", ";
                }
                std::cout << ")\n";

                std::cout << "AI: (Anlık Latent Kriptofig: ";
                for (size_t i = 0; i < sequenceManager.current_sequence->latent_cryptofig_vector.size(); ++i) {
                    std::cout << std::fixed << std::setprecision(2) << sequenceManager.current_sequence->latent_cryptofig_vector[i];
                    if (i + 1 < sequenceManager.current_sequence->latent_cryptofig_vector.size()) std::cout << ", ";
                }
                std::cout << ")\n";

                std::cout << "---------------------------------------------\n\n";
            }
        }

        // Normal kapanış
        simulatedSignalProcessor.stop_capture();
        analyzer.save_memory(AI_MEMORY_FILE);
        predictor.save_state_graph(AI_STATE_GRAPH_FILE);
        autoencoder.save_weights(AI_AUTOENCODER_WEIGHTS_FILE);

    } catch (const std::exception& e) {
        LOG_DEFAULT(LogLevel::ERR_CRITICAL, "Uygulama hatası: " << e.what());
        std::cerr << "Uygulama hatası: " << e.what() << "\n";
    } catch (...) {
        LOG_DEFAULT(LogLevel::ERR_CRITICAL, "Bilinmeyen bir uygulama hatasi olustu.");
        std::cerr << "Bilinmeyen bir uygulama hatasi olustu.\n";
    }

    std::cout << "Cikis icin Enter'a basin...";
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    std::cin.get();

    return 0;
}