// Düzeltme: Windows GUI uygulamaları için Qt'nin özel başlığı
#include <QApplication>
#include <QTimer>
#include <iostream>   
#include <memory>     
#include <fstream>    
#include <QDebug>     
#include <iomanip> 
#include <sstream> 

// OpenSSL için gerekli başlıklar (Sadece EVP_CIPHER_iv_length için gerekli olanı bırakıldı)
#include <openssl/evp.h>    

#include "gui/MainWindow.h"
#include "gui/engine_integration.h"
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
#include "learning/LearningModule.h" 
#include "learning/Capsule.h"       


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
    // konsol açılmasın
    /*
    #ifdef _WIN32 
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    _setmode(_fileno(stdout), _O_U8TEXT);
    _setmode(_fileno(stderr), _O_U8TEXT);
    #endif
    */

    std::ofstream early_diagnostic_log("cerebrum_lux_early_diagnostic.log", std::ios_base::app);
    if (early_diagnostic_log.is_open()) {
        early_diagnostic_log << get_current_timestamp_str() << " [EARLY DIAGNOSTIC] main function entered." << std::endl;
        early_diagnostic_log.flush();
    } else {
        std::cerr << "ERROR: Could not open early_diagnostic_log. This is a critical failure." << std::endl;
    }

    QApplication app(argc, argv);

    qRegisterMetaType<IngestResult>("IngestResult");
    qRegisterMetaType<IngestReport>("IngestReport");

     // Logger başlat
    Logger::get_instance().init(LogLevel::INFO, "cerebrum_lux_gui_log.txt", "MAIN_APP"); 
    LOG(LogLevel::INFO, "Application starting up. Standard streams (cout/cerr) will output to console. Qt logs and custom logs go to GUI.");
    
    early_diagnostic_log << get_current_timestamp_str() << " [EARLY DIAGNOSTIC] QApplication initialized. Logger ready (buffered)." << std::endl;
    early_diagnostic_log.flush();

    qInstallMessageHandler(customQtMessageHandler);

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

    } else {
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "ERROR: GUI LogPanel's QTextEdit could not be found for direct linking. Logs will ONLY go to file and console.");
        early_diagnostic_log << get_current_timestamp_str() << " [EARLY DIAGNOSTIC] ERROR: GUI LogPanel's QTextEdit not found for direct linking." << std::endl;
    }
    early_diagnostic_log.flush();

    // --- YENİ: LearningModule::ingest_envelope için Test Senaryoları ---
    LOG(LogLevel::INFO, "--- Starting LearningModule::ingest_envelope Test Scenarios ---");

    // Helper to sign and encrypt capsules for testing
    // Lambda LearningModule objesini yakalıyor
    auto create_signed_encrypted_capsule = [&](const std::string& id_prefix, const std::string& content, const std::string& source_peer, float confidence) {
        Capsule c;
        static unsigned int local_capsule_id_counter = 0; // main.cpp için yerel sayaç
        c.id = id_prefix + std::to_string(++local_capsule_id_counter); 
        c.content = content;
        c.source = source_peer;
        c.topic = "Test Topic";
        c.confidence = confidence;
        c.plain_text_summary = content.substr(0, std::min((size_t)100, content.length())) + "...";
        c.timestamp_utc = std::chrono::system_clock::now();
        
        c.embedding = learning_module.compute_embedding(c.content);
        c.cryptofig_blob_base64 = learning_module.cryptofig_encode(c.embedding);

        std::string aes_key = learning_module.get_aes_key_for_peer(source_peer);
        std::string iv = learning_module.generate_random_bytes(EVP_CIPHER_iv_length(EVP_aes_256_gcm()));
        c.encryption_iv_base64 = learning_module.base64_encode_string(iv); // LearningModule'ün public base64_encode_string metodu kullanıldı
        c.encrypted_content = learning_module.aes_gcm_encrypt(c.content, aes_key, iv);

        // Ed25519 fonksiyonları yorum satırı yapıldığı için burada da simüle ediyoruz
        // std::string private_key = learning_module.get_my_private_key(); 
        // c.signature_base64 = learning_module.ed25519_sign(c.encrypted_content, private_key);
        c.signature_base64 = "valid_signature"; // Geçici simülasyon
        return c;
    };


    // Test Senaryosu 1: Başarılı Kapsül Yutma (Valid Signature, Clean Content)
    Capsule test_capsule_1 = create_signed_encrypted_capsule("valid_capsule_", "Bu temiz bir test kapsuludur. Guzel bir gun geciriyoruz.", "Test_Peer_A", 0.8f);
    IngestReport report_1 = learning_module.ingest_envelope(test_capsule_1, test_capsule_1.signature_base64, test_capsule_1.source);
    LOG(LogLevel::INFO, "Test 1 Result: " << static_cast<int>(report_1.result) << " - " << report_1.message);

    // Test Senaryosu 2: Geçersiz İmza
    Capsule test_capsule_2 = create_signed_encrypted_capsule("invalid_sig_capsule_", "Bu kapsulun imzasi gecersiz olmali.", "Unauthorized_Peer", 0.5f);
    test_capsule_2.signature_base64 = "invalid_signature"; // Kasten yanlış imza
    IngestReport report_2 = learning_module.ingest_envelope(test_capsule_2, test_capsule_2.signature_base64, test_capsule_2.source);
    LOG(LogLevel::INFO, "Test 2 Result (Invalid Signature): " << static_cast<int>(report_2.result) << " - " << report_2.message);

    // Test Senaryosu 3: Steganografi İçeren Kapsül
    Capsule test_capsule_3 = create_signed_encrypted_capsule("stego_capsule_", "Normal gorunen bir metin ama icinde hidden_message_tag var.", "Suspicious_Source", 0.7f); // StegoDetector tetiklemeli
    IngestReport report_3 = learning_module.ingest_envelope(test_capsule_3, test_capsule_3.signature_base64, test_capsule_3.source);
    LOG(LogLevel::INFO, "Test 3 Result (Steganography Detected): " << static_cast<int>(report_3.result) << " - " << report_3.message);

    // Test Senaryosu 4: Unicode Temizleme Gerektiren Kapsül
    Capsule test_capsule_4 = create_signed_encrypted_capsule("unicode_capsule_", "Metin\x01\x02\x03içinde kontrol karakterleri var. \t Yeni satir.", "Dirty_Source", 0.9f); // UnicodeSanitizer tetiklemeli
    IngestReport report_4 = learning_module.ingest_envelope(test_capsule_4, test_capsule_4.signature_base64, test_capsule_4.source);
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
    engineTimer.start(1000); // 1 saniye döngü

    early_diagnostic_log << get_current_timestamp_str() << " [EARLY DIAGNOSTIC] Engine timer started. Entering QApplication::exec()." << std::endl;
    early_diagnostic_log.flush();

    int result = app.exec();

    kb.save("knowledge.json");

    early_diagnostic_log << get_current_timestamp_str() << " [EARLY DIAGNOSTIC] Application exited with code: " << result << std::endl;
    early_diagnostic_log.close(); 
    
    return result;
}