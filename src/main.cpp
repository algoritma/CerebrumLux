#include <iostream>
#include <iomanip> 
#include <locale>  
#include <string> 
#include <cmath> 
#include <limits> 
#include <cerrno> 
#include <fstream> 

// Cerebrum Lux modüllerinin başlık dosyaları
#include "sensors/atomic_signal.h" // AtomicSignal için eklendi
#include "core/enums.h"
#include "core/utils.h"
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
#include "communication/suggestion_engine.h" // SuggestionEngine için eklendi

#ifdef _WIN32
#include <windows.h> 
#include <fcntl.h>   
#include <io.h>      
#endif

// Dosya isimleri (constants)
const std::wstring AI_MEMORY_FILE = L"cerebrum_lux_memory.bin"; 
const std::wstring AI_STATE_GRAPH_FILE = L"cerebrum_lux_state_graph.bin"; 
const std::wstring AI_LOG_FILE = L"cerebrum_lux_log.txt"; 
const std::wstring AI_AUTOENCODER_WEIGHTS_FILE = L"cerebrum_lux_autoencoder_weights.bin"; 

int main() {
    #ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    _setmode(_fileno(stdout), _O_U8TEXT);
    _setmode(_fileno(stdin), _O_U8TEXT);
    #endif

    std::locale::global(std::locale(""));
    std::wcout.imbue(std::locale(""));
    std::wcin.imbue(std::locale(""));

    Logger::get_instance().init(LogLevel::INFO, AI_LOG_FILE);
    LOG(LogLevel::INFO, L"Log dosyası açıldı: " << AI_LOG_FILE << L"\n");

    // AI bileşenlerini başlat
    SimulatedAtomicSignalProcessor simulatedSignalProcessor;
    SequenceManager sequenceManager;

    IntentAnalyzer analyzer; // IntentAnalyzer nesnesi oluşturun

    IntentLearner learner(analyzer); // IntentLearner nesnesini IntentAnalyzer ile oluşturun

    PredictionEngine predictor(analyzer, sequenceManager);
    SuggestionEngine suggester(analyzer); 
    
    // Autoencoder ve CryptofigProcessor başlatılıyor
    CryptofigAutoencoder autoencoder;
    autoencoder.load_weights(AI_AUTOENCODER_WEIGHTS_FILE); 
    CryptofigProcessor cryptofig_processor(analyzer, autoencoder); 

    // AIInsightsEngine başlatılıyor (diğer tüm bileşenlere bağımlı)
    AIInsightsEngine insights_engine(analyzer, learner, predictor, autoencoder, cryptofig_processor);

    // GoalManager başlatılıyor (insights_engine'a bağımlı)
    GoalManager goal_manager(insights_engine);
    
    // Planner ve ResponseEngine doğru argümanlarla başlatılıyor
    Planner planner(analyzer, suggester, goal_manager, predictor, insights_engine); 
    ResponseEngine responder(analyzer, goal_manager, insights_engine); 

    // AI hafızalarını ve durum grafiklerini yükle
    analyzer.load_memory(AI_MEMORY_FILE);
    predictor.load_state_graph(AI_STATE_GRAPH_FILE);

    // Sinyal yakalamayı başlat
    simulatedSignalProcessor.start_capture();

    // Kullanıcı arayüzü mesajları
    std::wcout << L"\n--- Cerebrum Lux Baslatildi ---" << std::endl;
                     std::wcout << L"    - Mevcut Log Seviyesi: " << log_level_to_string(Logger::get_instance().get_level()) << std::endl;
    std::wcout << L"  - Raporlama Seviyesini Degistirmek Icin 'L' tusuna basin." << std::endl;
    std::wcout << L"    (0=SILENT, 1=ERROR, 2=WARNING, 3=INFO, 4=DEBUG, 5=TRACE)" << std::endl;
    std::wcout << L"  - Durum Raporu Icin 'S' veya 'R' tusuna basin." << L" (Yeni satırda girilmelidir)" << std::endl;
    std::wcout << L"  - Cikis Icin 'Q' tusuna basin." << L" (Yeni satırda girilmelidir)" << std::endl;
    std::wcout << L"------------------------------" << std::endl;

    UserIntent last_predicted_intent = UserIntent::Unknown; 
    AIAction last_suggested_action = AIAction::None; 

    

    
    while (true) {
        
        

        if (last_suggested_action != AIAction::None) {
            std::wcout << L"Onerilen eylem '" << suggester.action_to_string(last_suggested_action) << L"' basarili oldu mu? (E/H): ";
            wchar_t feedback_char;
            std::wcin >> feedback_char;
            std::wcin.ignore(std::numeric_limits<std::streamsize>::max(), L'\n'); 
            bool approved = (feedback_char == L'E' || feedback_char == L'e');
            learner.process_explicit_feedback(last_predicted_intent, last_suggested_action, approved, *sequenceManager.current_sequence);
            std::wcout << L"Geri bildirim kaydedildi. AI ogreniyor..." << std::endl;
            last_suggested_action = AIAction::None; 
        }
        
        std::wcout << L"Klavye Girisi (Q ile çikis): "; 
        std::wstring user_input_line;
        std::getline(std::wcin, user_input_line); 

        if (user_input_line.empty() && std::wcin.eof()) { 
            break;
        } else if (user_input_line.empty()) { 
            continue;
        }

        if (user_input_line.length() == 1) {
            wchar_t ch = std::towlower(user_input_line[0]);
            if (ch == L'q') {
                break; 
            } else if (ch == L's' || ch == L'r') { 
                 std::wcout << L"\n--- CEREBRUM LUX: Durum Raporu ---" << std::endl; 
                 std::wcout << L"    - Niyet Belirleme Esiği: " << std::fixed << std::setprecision(3) << analyzer.confidence_threshold_for_known_intent << std::endl;
                 std::wcout << L"    - Ogrenme Orani: " << std::fixed << std::setprecision(5) << learner.get_learning_rate() << std::endl;
                 std::wcout << L"    - Sensor Veri Carpani: " << std::fixed << std::setprecision(2) << learner.get_desired_other_signals_multiplier() << std::endl;
                 
                 float total_implicit_performance = 0.0f;
                 int implicit_count = 0;
                 const auto& history_map = learner.get_implicit_feedback_history(); 
                 for (int i = static_cast<int>(UserIntent::FastTyping); i < static_cast<int>(UserIntent::Count); ++i) { 
                     UserIntent intent_id = static_cast<UserIntent>(i);
                     auto it = history_map.find(intent_id); 

                     if (it != history_map.end() && !it->second.empty()) { 
                         float avg = std::accumulate(it->second.begin(), it->second.end(), 0.0f) / it->second.size();
                         std::wcout << L"      - Niyet '" << intent_to_string(intent_id) << L"' Ort. Ortuk Performans: " << std::fixed << std::setprecision(3) << avg << std::endl;
                         total_implicit_performance += avg;
                         implicit_count++;
                     }
                 }
                 if (implicit_count > 0) {
                     std::wcout << L"    - Toplam Ort. Ortuk Performans (Bilinen Niyetler): " << std::fixed << std::setprecision(3) << total_implicit_performance / implicit_count << std::endl;
                 } else {
                     std::wcout << L"    - Henuz bilinen niyetler icin yeterli ortuk performans verisi yok." << std::endl;
                 }

                 std::wcout << L"    - Mevcut Log Seviyesi: " << log_level_to_string(Logger::get_instance().get_level()) << std::endl;
                 std::wcout << L"    - AI, niyetleri basariyla tahmin etmeye baslamistir. Ogrenme devam ediyor." << std::endl;
                 std::wcout << L"Rapor tamamlandi. Devam etmek icin herhangi bir tusa basin (yeni girdi): ";
                 continue; 
            } else if (ch == L'l') { 
                                                                std::wcout << L"Yeni Log Seviyesi Seçin (0=SILENT, 1=ERROR, 2=WARNING, 3=INFO, 4=DEBUG, 5=TRACE, Mevcut: " << log_level_to_string(Logger::get_instance().get_level()) << L"): ";
                int level_int;
                std::wcin >> level_int;
                if (std::wcin.fail() || level_int < static_cast<int>(LogLevel::SILENT) || level_int > static_cast<int>(LogLevel::TRACE)) {
                    std::wcin.clear(); 
                    std::wcout << L"Geçersiz log seviyesi. Mevcut seviye korunuyor." << std::endl;
                } else {
                    Logger::get_instance().init(static_cast<LogLevel>(level_int));
                    std::wcout << L"Log Seviyesi '" << log_level_to_string(static_cast<LogLevel>(level_int)) << L"' olarak ayarlandi." << std::endl;
                }
                std::wcin.ignore(std::numeric_limits<std::streamsize>::max(), L'\n'); 
                continue; 
            }
        }

        static std::random_device rd_main;
        static std::mt19937 gen_main(rd_main());

        int num_other_signals_to_simulate = static_cast<int>(std::round(learner.get_desired_other_signals_multiplier() * std::uniform_int_distribution<>(1, 3)(gen_main))); 
        num_other_signals_to_simulate = std::max(1, num_other_signals_to_simulate); 

        for (int i = 0; i < num_other_signals_to_simulate; ++i) {
            AtomicSignal other_signal = simulatedSignalProcessor.capture_next_signal(); 
            LOG(LogLevel::DEBUG, L"Diger sensor sinyali olusturuldu (tip: " << static_cast<int>(other_signal.sensor_type) << L"). Manager'a ekleniyor.\n"); 
            sequenceManager.add_signal(other_signal, cryptofig_processor); 
            LOG(LogLevel::DEBUG, L"Diger sensor sinyali manager'a eklendi.\n"); 
        }

        bool sequence_updated_in_this_line = false; 

        for (wchar_t ch : user_input_line) {
            AtomicSignal keyboard_signal = simulatedSignalProcessor.create_keyboard_signal(ch); 
            LOG(LogLevel::DEBUG, L"Klavye sinyali manager'a ekleniyor (karakter: '" << ch << L").\n"); 
            if (sequenceManager.add_signal(keyboard_signal, cryptofig_processor)) { 
                sequence_updated_in_this_line = true; 
            }
            LOG(LogLevel::DEBUG, L"Klavye sinyali manager'a eklendi. Sequence güncellendi mi? " << (sequence_updated_in_this_line ? L"Evet" : L"Hayir") << L".\n"); 
        }

        if (sequence_updated_in_this_line) { 
            std::wcout << L"\n--- Cerebrum Lux: Anlik Dizi Ozeti ---" << std::endl;
            std::wcout << std::fixed << std::setprecision(4);

            std::wcout << L"Son Guncelleme Zamani: " << sequenceManager.current_sequence->last_updated_us << L" us" << std::endl;
            std::wcout << L"Ort. Tus Araligi: " << sequenceManager.current_sequence->avg_keystroke_interval / 1000.0f << L" ms" << std::endl;
            std::wcout << L"Tus Degiskenligi: " << sequenceManager.current_sequence->keystroke_variability / 1000.0f << L" ms" << std::endl;
            std::wcout << L"Alfanumerik Oran: " << sequenceManager.current_sequence->alphanumeric_ratio << std::endl;
            std::wcout << L"Kontrol Tusu Sikligi: " << sequenceManager.current_sequence->control_key_frequency << std::endl;
            
            std::wcout << L"Fare Hareketi Yogunlugu: " << sequenceManager.current_sequence->mouse_movement_intensity << L" (Piksel/Ornek)" << std::endl;
            std::wcout << L"Fare Tiklama Sikligi: " << sequenceManager.current_sequence->mouse_click_frequency << L" (Tiklama/Ornek)" << std::endl;
            std::wcout << L"Ort. Ekran Parlakligi: " << sequenceManager.current_sequence->avg_brightness << L" (0-255)" << std::endl;
            std::wcout << L"Batarya Degisim Orani: " << sequenceManager.current_sequence->battery_status_change << L" (%)" << std::endl;
            std::wcout << L"Ag Aktivite Seviyesi: " << sequenceManager.current_sequence->network_activity_level << L" (Kbps)" << std::endl;
            
            std::wcout << L"Aktif Uygulama Hash: " << sequenceManager.current_sequence->current_app_hash << std::endl;
            
            // İstatistiksel özellik vektörünü yazdır
            std::wcout << L"İstatistiksel Ozellik Vektoru (Boyut " << sequenceManager.current_sequence->statistical_features_vector.size() << L"): [";
            for (size_t i = 0; i < sequenceManager.current_sequence->statistical_features_vector.size(); ++i) {
                std::wcout << sequenceManager.current_sequence->statistical_features_vector[i];
                if (i < sequenceManager.current_sequence->statistical_features_vector.size() - 1) {
                    std::wcout << L", ";
                }
            }
            std::wcout << L"]" << std::endl;

            // Latent kriptofig vektörünü yazdır
            std::wcout << L"Latent Kriptofig Vektoru (Boyut " << sequenceManager.current_sequence->latent_cryptofig_vector.size() << L"): [";
            for (size_t i = 0; i < sequenceManager.current_sequence->latent_cryptofig_vector.size(); ++i) {
                std::wcout << sequenceManager.current_sequence->latent_cryptofig_vector[i];
                if (i < sequenceManager.current_sequence->latent_cryptofig_vector.size() - 1) {
                    std::wcout << L", ";
                }
            }
            std::wcout << L"]" << std::endl;

            // YENİ: Dinamik hedef belirleme
            goal_manager.evaluate_and_set_goal(*sequenceManager.current_sequence); // Her döngüde hedefi güncelle
            std::wcout << L"AI'ın Mevcut Hedefi: ";
            switch (goal_manager.get_current_goal()) {
                case AIGoal::OptimizeProductivity: std::wcout << L"Üretkenliği Optimize Etmek"; break;
                case AIGoal::MaximizeBatteryLife:  std::wcout << L"Batarya Ömrünü Maksimuma Çıkarmak"; break;
                case AIGoal::ReduceDistractions:   std::wcout << L"Dikkat Dağıtıcıları Azaltmak"; break;
                case AIGoal::EnhanceCreativity:    std::wcout << L"Yaratıcılığı Artırmak"; break;
                case AIGoal::ImproveGamingExperience: std::wcout << L"Oyun Deneyimini İyileştirmek"; break;
                case AIGoal::FacilitateResearch:   std::wcout << L"Araştırmayı Kolaylaştırmak"; break;
                case AIGoal::SelfImprovement:      std::wcout << L"Kendi Kendini Geliştirmek"; break;
                case AIGoal::None:                 std::wcout << L"Yok"; break;
                default:                           std::wcout << L"Bilinmiyor"; break;
            }
            std::wcout << std::endl;


            UserIntent current_predicted_intent = analyzer.analyze_intent(*sequenceManager.current_sequence);
            std::wcout << L"Tahmini Kullanici Niyeti: " << intent_to_string(current_predicted_intent) << std::endl; 

            AbstractState current_abstract_state = analyzer.analyze_abstract_state(*sequenceManager.current_sequence, current_predicted_intent);
            std::wcout << L"Tahmini Soyut Durum: " << abstract_state_to_string(current_abstract_state) << std::endl;

            learner.process_feedback(*sequenceManager.current_sequence, current_predicted_intent, sequenceManager.get_signal_buffer_copy());

            if (last_predicted_intent != UserIntent::Unknown && last_predicted_intent != current_predicted_intent &&
                last_predicted_intent != UserIntent::None && current_predicted_intent != UserIntent::None) { 
                predictor.update_state_graph(last_predicted_intent, current_predicted_intent, *sequenceManager.current_sequence);
            }
            last_predicted_intent = current_predicted_intent;

            UserIntent next_predicted_intent = predictor.predict_next_intent(last_predicted_intent, *sequenceManager.current_sequence); 
            if (next_predicted_intent != UserIntent::Unknown && next_predicted_intent != current_predicted_intent && next_predicted_intent != UserIntent::None) {
                std::wcout << L"Olasi Sonraki Niyet: " << intent_to_string(next_predicted_intent) << std::endl;
            }

            AIAction suggested_action = suggester.suggest_action(current_predicted_intent, *sequenceManager.current_sequence);
            if (suggested_action != AIAction::None) {
                std::wcout << L"AI Onerisi: " << suggester.action_to_string(suggested_action) << std::endl;
                last_suggested_action = suggested_action; 
            }
            
            std::vector<ActionPlanStep> current_plan = planner.create_action_plan(current_predicted_intent, current_abstract_state, goal_manager.get_current_goal(), *sequenceManager.current_sequence);
            planner.execute_plan(current_plan);

            std::wstring ai_response = responder.generate_response(current_predicted_intent, current_abstract_state, goal_manager.get_current_goal(), *sequenceManager.current_sequence);
            if (!ai_response.empty()) {
                std::wcout << L"AI: " << ai_response << std::endl;
            }

            // Unknown niyet tespit edildiğinde bilgi transferi yerine Autoencoder'ı doğrudan besleme
            if (current_predicted_intent == UserIntent::Unknown) {
                LOG(LogLevel::DEBUG, std::wcout, L"Unknown niyet tespit edildi, Autoencoder'a istatistiksel özellikler ile öğrenme sinyali gönderiliyor.\n");
                std::wcout << L"AI: (Algı belirsizliği. Autoencoder yeni desenleri öğreniyor!)" << std::endl;
            }

            // Anlık istatistiksel ve latent kriptofig çıktıları
            std::wcout << L"AI: (Anlık İstatistiksel Özellikler: ";
            for (size_t i = 0; i < sequenceManager.current_sequence->statistical_features_vector.size(); ++i) {
                std::wcout << std::fixed << std::setprecision(2) << sequenceManager.current_sequence->statistical_features_vector[i];
                if (i < sequenceManager.current_sequence->statistical_features_vector.size() - 1) {
                    std::wcout << L", ";
                }
            }
            std::wcout << L")" << std::endl;

            std::wcout << L"AI: (Anlık Latent Kriptofig: ";
            for (size_t i = 0; i < sequenceManager.current_sequence->latent_cryptofig_vector.size(); ++i) {
                std::wcout << std::fixed << std::setprecision(2) << sequenceManager.current_sequence->latent_cryptofig_vector[i];
                if (i < sequenceManager.current_sequence->latent_cryptofig_vector.size() - 1) {
                    std::wcout << L", ";
                }
            }
            std::wcout << L")" << std::endl;
            
            std::wcout << L"---------------------------------------------" << std::endl << std::endl;
        }
    }

    simulatedSignalProcessor.stop_capture();
    analyzer.save_memory(AI_MEMORY_FILE);
    predictor.save_state_graph(AI_STATE_GRAPH_FILE);
    autoencoder.save_weights(AI_AUTOENCODER_WEIGHTS_FILE); // Ağırlıkları kaydet

    

    return 0;
}