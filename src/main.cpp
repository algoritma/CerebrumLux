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
#include "learning/KnowledgeBase.h"
#include "learning/LearningModule.h"
#include "learning/Capsule.h"


void customQtMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    std::string source_file = context.file ? context.file : "unknown";
    int source_line = context.line;

    switch (type) {
    case QtDebugMsg:
        CerebrumLux::Logger::get_instance().log(CerebrumLux::LogLevel::DEBUG, msg.toStdString(), source_file.c_str(), source_line);
        break;
    case QtInfoMsg:
        CerebrumLux::Logger::get_instance().log(CerebrumLux::LogLevel::INFO, msg.toStdString(), source_file.c_str(), source_line);
        break;
    case QtWarningMsg:
        CerebrumLux::Logger::get_instance().log(CerebrumLux::LogLevel::WARNING, msg.toStdString(), source_file.c_str(), source_line);
        break;
    case QtCriticalMsg:
        CerebrumLux::Logger::get_instance().log_error_to_cerr(CerebrumLux::LogLevel::ERR_CRITICAL, msg.toStdString(), source_file.c_str(), source_line);
        break;
    case QtFatalMsg:
        CerebrumLux::Logger::get_instance().log_error_to_cerr(CerebrumLux::LogLevel::ERR_CRITICAL, msg.toStdString(), source_file.c_str(), source_line);
        abort();
    }
}


int main(int argc, char *argv[])
{
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

     // Logger başlat - Log seviyesini DEBUG olarak ayarlıyoruz.
    CerebrumLux::Logger::get_instance().init(CerebrumLux::LogLevel::DEBUG, "cerebrum_lux_gui_log.txt", "MAIN_APP");
    LOG_DEFAULT(CerebrumLux::LogLevel::INFO, "Application starting up. Standard streams (cout/cerr) will output to console if no FileSink or GuiSink is active.");

    early_diagnostic_log << CerebrumLux::get_current_timestamp_str() << " [EARLY DIAGNOSTIC] QApplication initialized. Logger ready (buffered)." << std::endl;
    early_diagnostic_log.flush();

    qInstallMessageHandler(customQtMessageHandler);

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

    CerebrumLux::NaturalLanguageProcessor nlp(goal_manager);
    early_diagnostic_log << CerebrumLux::get_current_timestamp_str() << " [EARLY DIAGNOSTIC] NaturalLanguageProcessor init complete." << std::endl;
    early_diagnostic_log.flush();

    CerebrumLux::Planner planner(analyzer, suggester, goal_manager, predictor, insights_engine);
    early_diagnostic_log << CerebrumLux::get_current_timestamp_str() << " [EARLY DIAGNOSTIC] Planner init complete." << std::endl;
    early_diagnostic_log.flush();

    CerebrumLux::ResponseEngine responder(analyzer, goal_manager, insights_engine, &nlp);
    early_diagnostic_log << CerebrumLux::get_current_timestamp_str() << " [EARLY DIAGNOSTIC] ResponseEngine init complete." << std::endl;
    early_diagnostic_log.flush();

    early_diagnostic_log << CerebrumLux::get_current_timestamp_str() << " [EARLY DIAGNOSTIC] All core AI components initialized." << std::endl;
    early_diagnostic_log.flush();

    // --- Kripto Yöneticisi ---
    CerebrumLux::Crypto::CryptoManager cryptoManager;
    early_diagnostic_log << CerebrumLux::get_current_timestamp_str() << " [EARLY DIAGNOSTIC] CryptoManager initialized." << std::endl;
    early_diagnostic_log.flush();

    // --- Learning Module ---
    CerebrumLux::KnowledgeBase kb;
    early_diagnostic_log << CerebrumLux::get_current_timestamp_str() << " [EARLY DIAGNOSTIC] KnowledgeBase created." << std::endl;
    early_diagnostic_log.flush();
    kb.load("knowledge.json");
    early_diagnostic_log << CerebrumLux::get_current_timestamp_str() << " [EARLY DIAGNOSTIC] KnowledgeBase loaded." << std::endl;
    early_diagnostic_log.flush();
    CerebrumLux::LearningModule learning_module(kb, cryptoManager);
    early_diagnostic_log << CerebrumLux::get_current_timestamp_str() << " [EARLY DIAGNOSTIC] Learning Module initialized." << std::endl;
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
    early_diagnostic_log << CerebrumLux::get_current_timestamp_str() << " [EARLY DIAGNOSTIC] EngineIntegration initialized." << std::endl;
    early_diagnostic_log.flush();

    CerebrumLux::MainWindow window(integration, learning_module);
    early_diagnostic_log << CerebrumLux::get_current_timestamp_str() << " [EARLY DIAGNOSTIC] MainWindow created." << std::endl;
    early_diagnostic_log.flush();

    window.show();
    early_diagnostic_log << CerebrumLux::get_current_timestamp_str() << " [EARLY DIAGNOSTIC] MainWindow shown." << std::endl;
    early_diagnostic_log.flush();
    
    QTextEdit* guiLogTextEdit = nullptr;
    if (window.getLogPanel()) {
        guiLogTextEdit = window.getLogPanel()->getLogTextEdit();
    }

    if (guiLogTextEdit) {
        CerebrumLux::Logger::get_instance().set_log_panel_text_edit(guiLogTextEdit);
        LOG_DEFAULT(CerebrumLux::LogLevel::INFO, "Logger: Direct GUI QTextEdit link established. Buffered logs flushed to GUI.");
        early_diagnostic_log << CerebrumLux::get_current_timestamp_str() << " [EARLY DIAGNOSTIC] Logger linked directly to GUI QTextEdit." << std::endl;

    } else {
        LOG_ERROR_CERR(CerebrumLux::LogLevel::ERR_CRITICAL, "ERROR: GUI LogPanel's QTextEdit could not be found for direct linking. Logs will ONLY go to file and console.");
        early_diagnostic_log << CerebrumLux::get_current_timestamp_str() << " [EARLY DIAGNOSTIC] ERROR: GUI LogPanel's QTextEdit not found for direct linking." << std::endl;
    }
    early_diagnostic_log.flush();

    // --- LearningModule::ingest_envelope Test Senaryoları bloğu TEKRAR YORUM SATIRI YAPILDI ---
    /* (Bu blok şu anki çökmenin ana nedeni olarak değerlendirildi ve pasif bırakıldı) */

    // --- Engine döngüsü QTimer bloğu --- (QTimer'ı heap üzerinde oluşturuyoruz)
    QTimer* engineTimer = new QTimer(&app); // Heap üzerinde oluşturuldu ve parent olarak app verildi.
    QObject::connect(engineTimer, &QTimer::timeout, [&](){
        try { // meta_engine.run_meta_evolution_cycle() çağrısını try-catch bloğu ile sarmalıyoruz
            LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "MAIN_APP: Meta-evolution cycle başlatılıyor.");
            meta_engine.run_meta_evolution_cycle(sequenceManager.get_current_sequence_ref());
            LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "MAIN_APP: Meta-evolution cycle tamamlandı.");
        } catch (const std::exception& e) {
            LOG_ERROR_CERR(CerebrumLux::LogLevel::ERR_CRITICAL, "MAIN_APP: Meta-evolution cycle sirasinda hata: " << e.what());
            // Hata sonrası timer'ı durdurabiliriz veya başka bir kurtarma mekanizması ekleyebiliriz.
            // engineTimer->stop(); // Gerekirse timer'ı durdur
        } catch (...) {
            LOG_ERROR_CERR(CerebrumLux::LogLevel::ERR_CRITICAL, "MAIN_APP: Meta-evolution cycle sirasinda bilinmeyen bir hata olustu.");
            // engineTimer->stop(); // Gerekirse timer'ı durdur
        }
    });
    engineTimer->start(1000); // 1 saniye döngü


    early_diagnostic_log << CerebrumLux::get_current_timestamp_str() << " [EARLY DIAGNOSTIC] Entering QApplication::exec()." << std::endl;
    early_diagnostic_log.flush();

    int result = app.exec();

    early_diagnostic_log << CerebrumLux::get_current_timestamp_str() << " [EARLY DIAGNOSTIC] Exiting QApplication::exec()." << std::endl;
    early_diagnostic_log.flush();

    kb.save("knowledge.json");

    early_diagnostic_log << CerebrumLux::get_current_timestamp_str() << " [EARLY DIAGNOSTIC] Application exited with code: " << result << std::endl;
    early_diagnostic_log.close();

    // Heap üzerinde oluşturulan engineTimer'ı temizle (parent'ı app olduğu için normalde app kapanırken silinir, ama manuel silmek daha güvenli).
    // Ancak QTimer'ın parent'ı app olduğu için, app silindiğinde QTimer da silinecektir.
    // delete engineTimer; // Bu satır gerekli değildir ve double-free'ye yol açabilir.
    
    return result;
}