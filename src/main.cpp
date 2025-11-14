#ifdef _WIN32
#include <Windows.h>
#endif

#include <QApplication>
#include <QTimer> 
#include <iostream>
#include <memory>
#include <fstream>
#include <QDebug>
#include <iomanip>
#include <sstream>

#include "gui/MainWindow.h"
#include "gui/engine_integration.h"
#include "gui/panels/LogPanel.h"

#include "crypto/CryptoManager.h"
#include "crypto/CryptoUtils.h"

#include "sensors/atomic_signal.h"
#include "core/enums.h"
#include "core/utils.h" // YENİ EKLENDİ: SafeRNG için (getInstance, shutdown metodları için)
#include "core/logger.h" // YENİ EKLENDİ: Logger için (getInstance, shutdown metodları için)
#include "sensors/simulated_processor.h" // SimulatedAtomicSignalProcessor için
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
#include "meta/meta_evolution_engine.h"
#include "user/user_profile_manager.h"
#include "learning/KnowledgeBase.h"
#include "learning/LearningModule.h"
#include "learning/Capsule.h"


int main(int argc, char *argv[])
{
    // Erken teşhis log dosyası (QApplication başlatılmadan önceki hataları yakalamak için)
    std::ofstream early_diagnostic_log("cerebrum_lux_early_diagnostic.log", std::ios_base::app);
    if (early_diagnostic_log.is_open()) {
        early_diagnostic_log << CerebrumLux::get_current_timestamp_str() << " [EARLY DIAGNOSTIC] main function entered." << std::endl;
        early_diagnostic_log.flush();
    } else {
        std::cerr << "ERROR: Could not open early_diagnostic_log. This is a critical failure." << std::endl;
    }

    QApplication app(argc, argv);

    qRegisterMetaType<CerebrumLux::IngestResult>("CerebrumLux::IngestResult");
    qRegisterMetaType<CerebrumLux::IngestReport>("CerebrumLux::IngestReport");
    
    // Logger başlat ve yapılandır
    CerebrumLux::Logger::getInstance().init(CerebrumLux::LogLevel::DEBUG, "cerebrum_lux_gui_log.txt", "MAIN_APP");
    LOG_DEFAULT(CerebrumLux::LogLevel::INFO, "Application starting up. Standard streams (cout/cerr) will output to console.");


    early_diagnostic_log << CerebrumLux::get_current_timestamp_str() << " [EARLY DIAGNOSTIC] QApplication initialized. Logger ready (buffered)." << std::endl;
    early_diagnostic_log.flush();

    // --- AI motoru bileşenleri ---
    CerebrumLux::SequenceManager sequenceManager;
    CerebrumLux::IntentAnalyzer analyzer;
    CerebrumLux::SuggestionEngine suggester(analyzer);
    CerebrumLux::UserProfileManager userProfileManager;
    CerebrumLux::IntentLearner learner(analyzer, suggester, userProfileManager);
    CerebrumLux::PredictionEngine predictor(analyzer, sequenceManager);

    early_diagnostic_log << CerebrumLux::get_current_timestamp_str() << " [EARLY DIAGNOSTIC] PredictionEngine init complete." << std::endl;
    early_diagnostic_log.flush();

    CerebrumLux::CryptofigAutoencoder autoencoder;
    early_diagnostic_log << CerebrumLux::get_current_timestamp_str() << " [EARLY DIAGNOSTIC] CryptofigAutoencoder init complete." << std::endl;
    early_diagnostic_log.flush();

    CerebrumLux::CryptofigProcessor cryptofig_processor(analyzer, autoencoder);
    early_diagnostic_log << CerebrumLux::get_current_timestamp_str() << " [EARLY DIAGNOSTIC] CryptofigProcessor init complete." << std::endl;
    early_diagnostic_log.flush();

    CerebrumLux::AIInsightsEngine insights_engine(analyzer, learner, predictor, autoencoder, cryptofig_processor);
    early_diagnostic_log << CerebrumLux::get_current_timestamp_str() << " [EARLY DIAGNOSTIC] AIInsightsEngine init complete." << std::endl;
    early_diagnostic_log.flush();

    CerebrumLux::GoalManager goal_manager(insights_engine);
    early_diagnostic_log << CerebrumLux::get_current_timestamp_str() << " [EARLY DIAGNOSTIC] GoalManager init complete." << std::endl;
    early_diagnostic_log.flush();

    CerebrumLux::KnowledgeBase kb;
    //CerebrumLux::KnowledgeBase kb("data/CerebrumLux_lmdb_db"); // DÜZELTME: Veritabanı dosyalarını kalıcı bir yola kaydetmek için bu satır aktive edildi.

    CerebrumLux::NaturalLanguageProcessor nlp(goal_manager, kb);
    early_diagnostic_log << CerebrumLux::get_current_timestamp_str() << " [EARLY DIAGNOSTIC] NaturalLanguageProcessor init complete." << std::endl;
    early_diagnostic_log.flush();

    CerebrumLux::Planner planner(analyzer, suggester, goal_manager, predictor, insights_engine);
    early_diagnostic_log << CerebrumLux::get_current_timestamp_str() << " [EARLY DIAGNOSTIC] Planner init complete." << std::endl;
    early_diagnostic_log.flush();
 

    // --- Kripto Yöneticisi ---
    CerebrumLux::Crypto::CryptoManager cryptoManager;
    early_diagnostic_log << CerebrumLux::get_current_timestamp_str() << " [EARLY DIAGNOSTIC] CryptoManager initialized." << std::endl;
    early_diagnostic_log.flush();

    // --- Learning Module ---
    early_diagnostic_log << CerebrumLux::get_current_timestamp_str() << " [EARLY DIAGNOSTIC] KnowledgeBase created. Will load after GUI shows." << std::endl;
    early_diagnostic_log.flush();
    //CerebrumLux::LearningModule learning_module(kb, cryptoManager, &app);
    CerebrumLux::LearningModule learning_module(kb, cryptoManager, nullptr); // YENİ DÜZELTME: Parent'ı nullptr yaparak manuel yıkımı sağlayın

    early_diagnostic_log << CerebrumLux::get_current_timestamp_str() << " [EARLY DIAGNOSTIC] Learning Module initialized." << std::endl;
    early_diagnostic_log.flush();

    // Response Engine oluşturulurken NaturalLanguageProcessor'ın bir unique_ptr'ı verilmeli
    CerebrumLux::ResponseEngine responder(std::make_unique<CerebrumLux::NaturalLanguageProcessor>(goal_manager, kb));
    early_diagnostic_log << CerebrumLux::get_current_timestamp_str() << " [EARLY DIAGNOSTIC] ResponseEngine init complete." << std::endl;
    early_diagnostic_log.flush();
    
    early_diagnostic_log << CerebrumLux::get_current_timestamp_str() << " [EARLY DIAGNOSTIC] All core AI components initialized." << std::endl;
    early_diagnostic_log.flush();

    // --- Meta Engine ---
    CerebrumLux::MetaEvolutionEngine meta_engine(
        analyzer,
        learner,
        predictor,
        goal_manager,
        cryptofig_processor,
        insights_engine,
        learning_module
    );
    early_diagnostic_log << CerebrumLux::get_current_timestamp_str() << " [EARLY DIAGNOSTIC] Meta Engine initialized." << std::endl;
    early_diagnostic_log.flush();

    // --- GUI entegrasyonu ---
    CerebrumLux::EngineIntegration integration(meta_engine, sequenceManager, learning_module, kb, nlp, goal_manager, responder);
    early_diagnostic_log << CerebrumLux::get_current_timestamp_str() << " [EARLY DIAGNOSTIC] EngineIntegration initialized." << std::endl;
    early_diagnostic_log.flush();
    CerebrumLux::MainWindow window(integration, learning_module);
    early_diagnostic_log << CerebrumLux::get_current_timestamp_str() << " [EARLY DIAGNOSTIC] MainWindow created." << std::endl;
    early_diagnostic_log.flush();

    window.show();
    early_diagnostic_log << CerebrumLux::get_current_timestamp_str() << " [EARLY DIAGNOSTIC] MainWindow shown." << std::endl;
    early_diagnostic_log.flush();
    
    // Asenkron FastText model ve KnowledgeBase yüklemesini tetikle
    QTimer::singleShot(100, [&]() { // GUI açıldıktan kısa bir süre sonra
        LOG_DEFAULT(CerebrumLux::LogLevel::INFO, "MAIN_APP: Asenkron KnowledgeBase ve FastText model yüklemesi başlatılıyor.");
        LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "MAIN_APP: KnowledgeBase JSON dosyasindan kapsuller yukleniyor...");
        kb.import_from_json("knowledge.json");
        LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "MAIN_APP: KnowledgeBase JSON dosyasindan kapsuller yuklendi.");
        CerebrumLux::NaturalLanguageProcessor::load_fasttext_models();
        LOG_DEFAULT(CerebrumLux::LogLevel::INFO, "MAIN_APP: KnowledgeBase ve FastText model yükleme işlemleri başlatıldı (asenkron).");
        if (window.getKnowledgeBasePanel()) {
            window.getKnowledgeBasePanel()->updateKnowledgeBaseContent();
        } else {
            LOG_ERROR_CERR(CerebrumLux::LogLevel::WARNING, "MAIN_APP: KnowledgeBasePanel null. KB içeriği güncellenemedi.");
        }
    });
    
    LOG_DEFAULT(CerebrumLux::LogLevel::INFO, "GUI LogPanel will connect to Logger signals for display.");
    early_diagnostic_log << CerebrumLux::get_current_timestamp_str() << " [EARLY DIAGNOSTIC] GUI LogPanel setup for Logger signals." << std::endl;
    early_diagnostic_log.flush();

    QTimer* engineTimer = new QTimer(&app);
    QObject::connect(engineTimer, &QTimer::timeout, [&](){
        try {
            LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "MAIN_APP: Meta-evolution cycle başlatılıyor.");
            meta_engine.run_meta_evolution_cycle(sequenceManager.get_current_sequence_ref());
            LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "MAIN_APP: Meta-evolution cycle tamamlandı.");
        } catch (const std::exception& e) {
            LOG_ERROR_CERR(CerebrumLux::LogLevel::ERR_CRITICAL, "MAIN_APP: Meta-evolution cycle sirasinda hata: " << e.what());
        } catch (...) {
            LOG_ERROR_CERR(CerebrumLux::LogLevel::ERR_CRITICAL, "MAIN_APP: Meta-evolution cycle sirasinda bilinmeyen bir hata olustu.");
        }
    });
    engineTimer->start(1000);

    CerebrumLux::SimulatedAtomicSignalProcessor simulated_sensor_processor;
    QTimer* signalCaptureTimer = new QTimer(&app);
    QObject::connect(signalCaptureTimer, &QTimer::timeout, [&]() {
        try {
            CerebrumLux::AtomicSignal signal = simulated_sensor_processor.capture_next_signal();
            LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "MAIN_APP: Simüle sinyal yakalandı. Type: " << CerebrumLux::sensor_type_to_string(signal.type) << ", Value: " << signal.value);
            sequenceManager.add_signal(signal, cryptofig_processor);
            
        } catch (const std::exception& e) {
            LOG_ERROR_CERR(CerebrumLux::LogLevel::ERR_CRITICAL, "MAIN_APP: Sinyal yakalama sirasinda hata: " << e.what());
        } catch (...) {
            LOG_ERROR_CERR(CerebrumLux::LogLevel::ERR_CRITICAL, "MAIN_APP: Sinyal yakalama sirasinda bilinmeyen bir hata olustu.");
        }
    });
    signalCaptureTimer->start(500);

    early_diagnostic_log << CerebrumLux::get_current_timestamp_str() << " [EARLY DIAGNOSTIC] Entering QApplication::exec()." << std::endl;
    early_diagnostic_log.flush();

    int result = app.exec();

    early_diagnostic_log << CerebrumLux::get_current_timestamp_str() << " [EARLY DIAGNOSTIC] Exiting QApplication::exec()." << std::endl;
    early_diagnostic_log.flush();
    
    // DÜZELTİLDİ: Singleton'ların shutdown metodları çağrıldı.
    // YENİ DÜZELTME: LearningModule'ün Q-Table'ını Logger kapatılmadan önce kaydet.
    // Eğer LearningModule'ün yıkıcısı çağrılmıyorsa, burada manuel olarak kaydedilir.
    learning_module.save_q_table(); 
    CerebrumLux::Logger::getInstance().shutdown();
    CerebrumLux::SafeRNG::getInstance().shutdown(); // get_instance yerine getInstance kullanıldı

    return result;
}