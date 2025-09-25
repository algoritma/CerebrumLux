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

// OpenSSL için gerekli başlıklar (CryptoManager kendi içinde yönetecek)
// #include <openssl/evp.h>

#include "gui/MainWindow.h"
#include "gui/engine_integration.h"
#include "gui/panels/LogPanel.h" // LogPanel'in tam tanımı için eklendi (ÇOK ÖNEMLİ)

// Yeni eklenen: CryptoManager
#include "crypto/CryptoManager.h"
#include "crypto/CryptoUtils.h" // CryptoUtils'ın Base64 fonksiyonları için

// AI core bileşenleri
#include "sensors/atomic_signal.h"
#include "core/enums.h"
#include "core/utils.h"
#include "core/logger.h" // CerebrumLux::Logger ve CerebrumLux::LogLevel için
#include "sensors/simulated_processor.h"
#include "data_models/sequence_manager.h"
#include "brain/intent_analyzer.h"
#include "brain/intent_learner.h"
#include "brain/prediction_engine.h"
#include "brain/autoencoder.h" // CryptofigAutoencoder tanımı için
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
        CerebrumLux::Logger::get_instance().log(CerebrumLux::LogLevel::DEBUG, msg.toStdString(), source_file.c_str(), source_line); // GÜNCELLENDİ
        break;
    case QtInfoMsg:
        CerebrumLux::Logger::get_instance().log(CerebrumLux::LogLevel::INFO, msg.toStdString(), source_file.c_str(), source_line); // GÜNCELLENDİ
        break;
    case QtWarningMsg:
        CerebrumLux::Logger::get_instance().log(CerebrumLux::LogLevel::WARNING, msg.toStdString(), source_file.c_str(), source_line); // GÜNCELLENDİ
        break;
    case QtCriticalMsg:
        CerebrumLux::Logger::get_instance().log_error_to_cerr(CerebrumLux::LogLevel::ERR_CRITICAL, msg.toStdString(), source_file.c_str(), source_line); // GÜNCELLENDİ
        break;
    case QtFatalMsg:
        CerebrumLux::Logger::get_instance().log_error_to_cerr(CerebrumLux::LogLevel::ERR_CRITICAL, msg.toStdString(), source_file.c_str(), source_line); // GÜNCELLENDİ
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
        early_diagnostic_log << CerebrumLux::get_current_timestamp_str() << " [EARLY DIAGNOSTIC] main function entered." << std::endl; // GÜNCELLENDİ
        early_diagnostic_log.flush();
    } else {
        std::cerr << "ERROR: Could not open early_diagnostic_log. This is a critical failure." << std::endl;
    }

    // Eğer konsol istemiyorsanız, bunu CMake'de WIN32 hedefi ile sağlayacağız.
    // QApplication'ı normal argc/argv ile başlatın:
    QApplication app(argc, argv);

    qRegisterMetaType<CerebrumLux::IngestResult>("CerebrumLux::IngestResult"); // Namespace ile güncellendi
    qRegisterMetaType<CerebrumLux::IngestReport>("CerebrumLux::IngestReport"); // Namespace ile güncellendi

     // Logger başlat
    CerebrumLux::Logger::get_instance().init(CerebrumLux::LogLevel::INFO, "cerebrum_lux_gui_log.txt", "MAIN_APP"); // GÜNCELLENDİ
    LOG(CerebrumLux::LogLevel::INFO, "Application starting up. Standard streams (cout/cerr) will output to console. Qt logs and custom logs go to GUI."); // GÜNCELLENDİ

    early_diagnostic_log << CerebrumLux::get_current_timestamp_str() << " [EARLY DIAGNOSTIC] QApplication initialized. Logger ready (buffered)." << std::endl; // GÜNCELLENDİ
    early_diagnostic_log.flush();

    qInstallMessageHandler(customQtMessageHandler);

    // --- AI motoru bileşenleri ---
    CerebrumLux::SequenceManager sequenceManager;
    CerebrumLux::IntentAnalyzer analyzer;
    CerebrumLux::SuggestionEngine suggester(analyzer);
    CerebrumLux::UserProfileManager userProfileManager;
    CerebrumLux::IntentLearner learner(analyzer, suggester, userProfileManager);
    CerebrumLux::PredictionEngine predictor(analyzer, sequenceManager);
    CerebrumLux::CryptofigAutoencoder autoencoder;
    CerebrumLux::CryptofigProcessor cryptofig_processor(analyzer, autoencoder);
    CerebrumLux::AIInsightsEngine insights_engine(analyzer, learner, predictor, autoencoder, cryptofig_processor);
    CerebrumLux::GoalManager goal_manager(insights_engine);
    CerebrumLux::NaturalLanguageProcessor nlp(goal_manager);
    CerebrumLux::Planner planner(analyzer, suggester, goal_manager, predictor, insights_engine);
    CerebrumLux::ResponseEngine responder(analyzer, goal_manager, insights_engine, &nlp);

    early_diagnostic_log << CerebrumLux::get_current_timestamp_str() << " [EARLY DIAGNOSTIC] Core AI components initialized." << std::endl;
    early_diagnostic_log.flush();

    // --- Kripto Yöneticisi ---
    CerebrumLux::Crypto::CryptoManager cryptoManager; // CryptoManager örneği oluşturuldu

    early_diagnostic_log << CerebrumLux::get_current_timestamp_str() << " [EARLY DIAGNOSTIC] CryptoManager initialized." << std::endl;
    early_diagnostic_log.flush();

    // --- Learning Module ---
    CerebrumLux::KnowledgeBase kb;
    kb.load("knowledge.json");
    CerebrumLux::LearningModule learning_module(kb, cryptoManager); // CryptoManager'ı LearningModule'e geçiriyoruz

    early_diagnostic_log << CerebrumLux::get_current_timestamp_str() << " [EARLY DIAGNOSTIC] Learning Module and KnowledgeBase initialized." << std::endl;
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
    CerebrumLux::EngineIntegration integration(meta_engine, sequenceManager, learning_module, kb);
    CerebrumLux::MainWindow window(integration, learning_module);
    window.show();

    early_diagnostic_log << CerebrumLux::get_current_timestamp_str() << " [EARLY DIAGNOSTIC] MainWindow created and shown." << std::endl;
    early_diagnostic_log.flush();

    QTextEdit* guiLogTextEdit = nullptr;
    // LogPanel'in tam tanımı artık mevcut olduğu için doğrudan erişebiliriz
    if (window.getLogPanel()) {
        guiLogTextEdit = window.getLogPanel()->getLogTextEdit();
    }

    if (guiLogTextEdit) {
        CerebrumLux::Logger::get_instance().set_log_panel_text_edit(guiLogTextEdit); // GÜNCELLENDİ
        LOG(CerebrumLux::LogLevel::INFO, "Logger: Direct GUI QTextEdit link established. Buffered logs flushed to GUI."); // GÜNCELLENDİ
        early_diagnostic_log << CerebrumLux::get_current_timestamp_str() << " [EARLY DIAGNOSTIC] Logger linked directly to GUI QTextEdit." << std::endl;

    } else {
        LOG_ERROR_CERR(CerebrumLux::LogLevel::ERR_CRITICAL, "ERROR: GUI LogPanel's QTextEdit could not be found for direct linking. Logs will ONLY go to file and console."); // GÜNCELLENDİ
        early_diagnostic_log << CerebrumLux::get_current_timestamp_str() << " [EARLY DIAGNOSTIC] ERROR: GUI LogPanel's QTextEdit not found for direct linking." << std::endl;
    }
    early_diagnostic_log.flush();

    // --- YENİ: LearningModule::ingest_envelope için Test Senaryoları ---
    LOG(CerebrumLux::LogLevel::INFO, "--- Starting LearningModule::ingest_envelope Test Scenarios ---"); // GÜNCELLENDİ

    // Helper to sign and encrypt capsules for testing
    auto create_signed_encrypted_capsule = [&](const std::string& id_prefix, const std::string& content, const std::string& source_peer, float confidence) {
        CerebrumLux::Capsule c; // Namespace ile güncellendi
        static unsigned int local_capsule_id_counter = 0; // main.cpp için yerel sayaç
        c.id = id_prefix + std::to_string(++local_capsule_id_counter);
        c.content = content;
        c.source = source_peer;
        c.topic = "Test Topic";
        c.confidence = confidence;
        c.plain_text_summary = content.substr(0, std::min((size_t)100, content.length())) + "...";
        c.timestamp_utc = std::chrono::system_clock::now();

        // Embedding ve Cryptofig (LearningModule içinde Base64, CryptoUtils'dan çağırıyor)
        c.embedding = learning_module.compute_embedding(c.content);
        c.cryptofig_blob_base64 = learning_module.cryptofig_encode(c.embedding);

        // AES anahtarını HKDF ile türetme (şimdilik random ile taklit edebiliriz)
        std::vector<unsigned char> aes_key_vec = cryptoManager.generate_random_bytes_vec(32); // 256-bit
        // std::string aes_key_base64 = CerebrumLux::Crypto::base64_encode(cryptoManager.vec_to_str(aes_key_vec)); // Kullanılmıyor, doğrudan vec olarak geçiliyor

        // IV üretimi
        std::vector<unsigned char> iv_vec = cryptoManager.generate_random_bytes_vec(12); // 12 byte GCM IV
        c.encryption_iv_base64 = CerebrumLux::Crypto::base64_encode(cryptoManager.vec_to_str(iv_vec));

        // Şifreleme
        CerebrumLux::Crypto::AESGCMCiphertext encrypted_data =
            cryptoManager.aes256_gcm_encrypt(cryptoManager.str_to_vec(c.content), aes_key_vec, {}); // AAD boş bırakıldı

        c.encrypted_content = encrypted_data.ciphertext_base64;
        c.gcm_tag_base64 = encrypted_data.tag_base64; // Yeni alan

        // Ed25519 İmzalama (kendi anahtarımızla)
        std::string private_key_pem = cryptoManager.get_my_private_key_pem();
        c.signature_base64 = cryptoManager.ed25519_sign(c.encrypted_content, private_key_pem);
        return c;
    };


    // Test Senaryosu 1: Başarılı Kapsül Yutma (Valid Signature, Clean Content)
    CerebrumLux::Capsule test_capsule_1 = create_signed_encrypted_capsule("valid_capsule_", "Bu temiz bir test kapsuludur. Guzel bir gun geciriyoruz.", "Test_Peer_A", 0.8f);
    CerebrumLux::IngestReport report_1 = learning_module.ingest_envelope(test_capsule_1, test_capsule_1.signature_base64, test_capsule_1.source);
    LOG(CerebrumLux::LogLevel::INFO, "Test 1 Result: " << static_cast<int>(report_1.result) << " - " << report_1.message); // GÜNCELLENDİ

    // Test Senaryosu 2: Geçersiz İmza
    CerebrumLux::Capsule test_capsule_2 = create_signed_encrypted_capsule("invalid_sig_capsule_", "Bu kapsulun imzasi gecersiz olmali.", "Unauthorized_Peer", 0.5f);
    test_capsule_2.signature_base64 = "invalid_signature_tampered"; // Kasten yanlış imza
    CerebrumLux::IngestReport report_2 = learning_module.ingest_envelope(test_capsule_2, test_capsule_2.signature_base64, test_capsule_2.source);
    LOG(CerebrumLux::LogLevel::INFO, "Test 2 Result (Invalid Signature): " << static_cast<int>(report_2.result) << " - " << report_2.message); // GÜNCELLENDİ

    // Test Senaryosu 3: Steganografi İçeren Kapsül
    CerebrumLux::Capsule test_capsule_3 = create_signed_encrypted_capsule("stego_capsule_", "Normal gorunen bir metin ama icinde hidden_message_tag var.", "Suspicious_Source", 0.7f); // StegoDetector tetiklemeli
    CerebrumLux::IngestReport report_3 = learning_module.ingest_envelope(test_capsule_3, test_capsule_3.signature_base64, test_capsule_3.source);
    LOG(CerebrumLux::LogLevel::INFO, "Test 3 Result (Steganography Detected): " << static_cast<int>(report_3.result) << " - " << report_3.message); // GÜNCELLENDİ

    // Test Senaryosu 4: Unicode Temizleme Gerektiren Kapsül
    CerebrumLux::Capsule test_capsule_4 = create_signed_encrypted_capsule("unicode_capsule_", "Metin\x01\x02\x03içinde kontrol karakterleri var. \t Yeni satir.", "Dirty_Source", 0.9f); // UnicodeSanitizer tetiklemeli
    CerebrumLux::IngestReport report_4 = learning_module.ingest_envelope(test_capsule_4, test_capsule_4.signature_base64, test_capsule_4.source);
    LOG(CerebrumLux::LogLevel::INFO, "Test 4 Result (Sanitization Needed): " << static_cast<int>(report_4.result) << " - " << report_4.message); // GÜNCELLENDİ
    if (report_4.result == CerebrumLux::IngestResult::SanitizationNeeded) {
        LOG(CerebrumLux::LogLevel::INFO, "    Sanitized Content: " << report_4.processed_capsule.content); // GÜNCELLENDİ
    }

    LOG(CerebrumLux::LogLevel::INFO, "--- Finished LearningModule::ingest_envelope Test Scenarios ---"); // GÜNCELLENDİ
    // --- Test Senaryoları Sonu ---


    // --- Engine döngüsü ---
    QTimer engineTimer;
    QObject::connect(&engineTimer, &QTimer::timeout, [&](){
        meta_engine.run_meta_evolution_cycle(sequenceManager.get_current_sequence_ref());
    });
    engineTimer.start(1000); // 1 saniye döngü

    early_diagnostic_log << CerebrumLux::get_current_timestamp_str() << " [EARLY DIAGNOSTIC] Engine timer started. Entering QApplication::exec()." << std::endl;
    early_diagnostic_log.flush();

    int result = app.exec();

    kb.save("knowledge.json");

    early_diagnostic_log << CerebrumLux::get_current_timestamp_str() << " [EARLY DIAGNOSTIC] Application exited with code: " << result << std::endl;
    early_diagnostic_log.close();

    return result;
}