#include <QApplication>
#include <QTimer>
#include <iostream>   
#include <memory>     
#include <fstream>    
#include <QDebug>     

#include "gui/MainWindow.h"
#include "gui/engine_integration.h"
// #include "gui/qtextedit_stream_buf.h" // Kaldırıldı
#include "gui/panels/LogPanel.h" 

#ifdef _WIN32 
#include <Windows.h>
#include <io.h>
#include <fcntl.h>
#endif

// AI core bileşenleri
#include "sensors/atomic_signal.h"
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
#include "communication/suggestion_engine.h"
#include "meta/meta_evolution_engine.h"
#include "user/user_profile_manager.h"
#include "communication/natural_language_processor.h"
#include "learning/KnowledgeBase.h"
#include "learning/LearningModule.h" // YENİ: LearningModule.h dahil edildi (IngestResult, IngestReport ve Capsule tanımları için)
#include "learning/Capsule.h"       // YENİ: Capsule tanımı için (zaten LearningModule.h dahil ediyor olabilir ama emin olmak için)


// KALDIRILDI: std::cout/cerr yönlendirmesi için static unique_ptr'lar ve mutex
// static std::unique_ptr<QTextEditStreamBuf> g_coutRedirector;
// static std::unique_ptr<QTextEditStreamBuf> g_cerrRedirector;
// static QMutex g_streamMutex; 

// Qt'nin debug mesajlarını Logger'a yönlendirecek özel mesaj işleyici
void customQtMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    std::string source_file = context.file ? context.file : "unknown";
    int source_line = context.line;

    switch (type) {
    case QtDebugMsg:
        Logger::get_instance().log(LogLevel::DEBUG, msg.toStdString(), source_file.c_str(), source_line);
        break;
    case QtInfoMsg:
        Logger::get_instance().log(LogLevel::INFO, msg.toStdString(), source_file.c_str(), source_line);
        break;
    case QtWarningMsg:
        Logger::get_instance().log(LogLevel::WARNING, msg.toStdString(), source_file.c_str(), source_line);
        break;
    case QtCriticalMsg:
        Logger::get_instance().log_error_to_cerr(LogLevel::ERR_CRITICAL, msg.toStdString(), source_file.c_str(), source_line);
        break;
    case QtFatalMsg:
        Logger::get_instance().log_error_to_cerr(LogLevel::ERR_CRITICAL, msg.toStdString(), source_file.c_str(), source_line);
        abort(); 
    }
}


int main(int argc, char *argv[])
{
    #ifdef _WIN32 
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    _setmode(_fileno(stdout), _O_U8TEXT);
    _setmode(_fileno(stderr), _O_U8TEXT);
    #endif

    std::ofstream early_diagnostic_log("cerebrum_lux_early_diagnostic.log", std::ios_base::app);
    if (early_diagnostic_log.is_open()) {
        early_diagnostic_log << get_current_timestamp_str() << " [EARLY DIAGNOSTIC] main function entered." << std::endl;
        early_diagnostic_log.flush();
    } else {
        std::cerr << "ERROR: Could not open early_diagnostic_log. This is a critical failure." << std::endl;
    }

    QApplication app(argc, argv);

    Logger::get_instance().init(LogLevel::INFO, "cerebrum_lux_gui_log.txt", "MAIN_APP"); 
    LOG(LogLevel::INFO, "Application starting up. Standard streams (cout/cerr) will output to console. Qt logs and custom logs go to GUI.");
    early_diagnostic_log << get_current_timestamp_str() << " [EARLY DIAGNOSTIC] QApplication initialized. Logger ready (buffered)." << std::endl;
    early_diagnostic_log.flush();

    // --- AI motoru bileşenleri ---
    SequenceManager sequenceManager;
    IntentAnalyzer analyzer;
    SuggestionEngine suggester(analyzer);
    UserProfileManager userProfileManager;
    IntentLearner learner(analyzer, suggester, userProfileManager);
    PredictionEngine predictor(analyzer, sequenceManager);
    CryptofigAutoencoder autoencoder;
    CryptofigProcessor cryptofig_processor(analyzer, autoencoder);
    AIInsightsEngine insights_engine(analyzer, learner, predictor, autoencoder, cryptofig_processor);
    GoalManager goal_manager(insights_engine);
    NaturalLanguageProcessor nlp(goal_manager);
    Planner planner(analyzer, suggester, goal_manager, predictor, insights_engine);
    ResponseEngine responder(analyzer, goal_manager, insights_engine, &nlp);

    early_diagnostic_log << get_current_timestamp_str() << " [EARLY DIAGNOSTIC] Core AI components initialized." << std::endl;
    early_diagnostic_log.flush();

    // --- Learning Module ---
    KnowledgeBase kb;
    kb.load("knowledge.json");
    LearningModule learning_module(kb);

    early_diagnostic_log << get_current_timestamp_str() << " [EARLY DIAGNOSTIC] Learning Module and KnowledgeBase initialized." << std::endl;
    early_diagnostic_log.flush();

    // --- Meta Engine ---
    MetaEvolutionEngine meta_engine(
        analyzer,
        learner,
        predictor,
        goal_manager,
        cryptofig_processor,
        insights_engine,
        learning_module
    );

    early_diagnostic_log << get_current_timestamp_str() << " [EARLY DIAGNOSTIC] Meta Engine initialized." << std::endl;
    early_diagnostic_log.flush();

    // --- GUI entegrasyonu ---
    EngineIntegration integration(meta_engine, sequenceManager, learning_module, kb);
    MainWindow window(integration, learning_module);
    window.show(); 

    early_diagnostic_log << get_current_timestamp_str() << " [EARLY DIAGNOSTIC] MainWindow created and shown." << std::endl;
    early_diagnostic_log.flush();

    QTextEdit* guiLogTextEdit = nullptr;
    if (window.getLogPanel()) {
        guiLogTextEdit = window.getLogPanel()->getLogTextEdit(); 
    }

    if (guiLogTextEdit) {
        Logger::get_instance().set_log_panel_text_edit(guiLogTextEdit); 
        LOG(LogLevel::INFO, "Logger: Direct GUI QTextEdit link established. Buffered logs flushed to GUI.");
        early_diagnostic_log << get_current_timestamp_str() << " [EARLY DIAGNOSTIC] Logger linked directly to GUI QTextEdit." << std::endl;

        early_diagnostic_log << get_current_timestamp_str() << " [EARLY DIAGNOSTIC] Standard streams NOT redirected to GUI (intentionally). They will appear in console." << std::endl;

        qInstallMessageHandler(customQtMessageHandler);
        LOG(LogLevel::INFO, "Qt message handler installed for redirecting qDebug() etc. to Logger.");
        early_diagnostic_log << get_current_timestamp_str() << " [EARLY DIAGNOSTIC] Qt message handler installed." << std::endl;

    } else {
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "ERROR: GUI LogPanel's QTextEdit could not be found for direct linking. Logs will ONLY go to file and console.");
        early_diagnostic_log << get_current_timestamp_str() << " [EARLY DIAGNOSTIC] ERROR: GUI LogPanel's QTextEdit not found for direct linking." << std::endl;
    }
    early_diagnostic_log.flush();

    // --- YENİ: LearningModule::ingest_envelope için Test Senaryoları ---
    LOG(LogLevel::INFO, "--- Starting LearningModule::ingest_envelope Test Scenarios ---");

    // Test Senaryosu 1: Başarılı Kapsül Yutma (Valid Signature, Clean Content)
    Capsule test_capsule_1;
    test_capsule_1.id = 101;
    test_capsule_1.content = "Bu temiz bir test kapsuludur. Guzel bir gun geciriyoruz.";
    test_capsule_1.source = "Test_Peer_A";
    test_capsule_1.topic = "General Info";
    test_capsule_1.confidence = 0.8f;
    test_capsule_1.encrypted_content = learning_module.getKnowledgeBase().encrypt(test_capsule_1.content); // encrypt content

    IngestReport report_1 = learning_module.ingest_envelope(test_capsule_1, "valid_signature", "Test_Peer_A");
    LOG(LogLevel::INFO, "Test 1 Result: " << static_cast<int>(report_1.result) << " - " << report_1.message);

    // Test Senaryosu 2: Geçersiz İmza
    Capsule test_capsule_2;
    test_capsule_2.id = 102;
    test_capsule_2.content = "Bu kapsulun imzasi gecersiz olmali.";
    test_capsule_2.source = "Unauthorized_Peer";
    test_capsule_2.topic = "Security Alert";
    test_capsule_2.confidence = 0.5f;
    test_capsule_2.encrypted_content = learning_module.getKnowledgeBase().encrypt(test_capsule_2.content); // encrypt content

    IngestReport report_2 = learning_module.ingest_envelope(test_capsule_2, "invalid_signature", "Unauthorized_Peer");
    LOG(LogLevel::INFO, "Test 2 Result (Invalid Signature): " << static_cast<int>(report_2.result) << " - " << report_2.message);

    // Test Senaryosu 3: Steganografi İçeren Kapsül
    Capsule test_capsule_3;
    test_capsule_3.id = 103;
    test_capsule_3.content = "Normal gorunen bir metin ama icinde hidden_message_tag var."; // StegoDetector tetiklemeli
    test_capsule_3.source = "Suspicious_Source";
    test_capsule_3.topic = "Hidden Data";
    test_capsule_3.confidence = 0.7f;
    test_capsule_3.encrypted_content = learning_module.getKnowledgeBase().encrypt(test_capsule_3.content); // encrypt content

    IngestReport report_3 = learning_module.ingest_envelope(test_capsule_3, "valid_signature", "Suspicious_Source");
    LOG(LogLevel::INFO, "Test 3 Result (Steganography Detected): " << static_cast<int>(report_3.result) << " - " << report_3.message);

    // Test Senaryosu 4: Unicode Temizleme Gerektiren Kapsül
    Capsule test_capsule_4;
    test_capsule_4.id = 104;
    test_capsule_4.content = "Metin\x01\x02\x03i├ºinde kontrol karakterleri var. \t Yeni satir."; // UnicodeSanitizer tetiklemeli
    test_capsule_4.source = "Dirty_Source";
    test_capsule_4.topic = "Data Hygiene";
    test_capsule_4.confidence = 0.9f;
    test_capsule_4.encrypted_content = learning_module.getKnowledgeBase().encrypt(test_capsule_4.content); // encrypt content

    IngestReport report_4 = learning_module.ingest_envelope(test_capsule_4, "valid_signature", "Dirty_Source");
    LOG(LogLevel::INFO, "Test 4 Result (Sanitization Needed): " << static_cast<int>(report_4.result) << " - " << report_4.message);
    if (report_4.result == IngestResult::SanitizationNeeded) {
        LOG(LogLevel::INFO, "    Sanitized Content: " << report_4.processed_capsule.content);
    }

    LOG(LogLevel::INFO, "--- Finished LearningModule::ingest_envelope Test Scenarios ---");
    // --- Test Senaryoları Sonu ---


    // --- Engine döngüsü ---
    QTimer engineTimer;
    QObject::connect(&engineTimer, &QTimer::timeout, [&](){
        meta_engine.run_meta_evolution_cycle(sequenceManager.get_current_sequence_ref());
    });
    engineTimer.start(1000); 

    early_diagnostic_log << get_current_timestamp_str() << " [EARLY DIAGNOSTIC] Engine timer started. Entering QApplication::exec()." << std::endl;
    early_diagnostic_log.flush();

    int result = app.exec();

    kb.save("knowledge.json");
    early_diagnostic_log << get_current_timestamp_str() << " [EARLY DIAGNOSTIC] Application exited with code: " << result << std::endl;
    early_diagnostic_log.close(); 
    
    return result;
}