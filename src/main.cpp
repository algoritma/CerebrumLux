#include <QApplication>
#include <QTimer>
#include <iostream>   
#include <memory>     
#include <fstream>    

#include "gui/MainWindow.h"
#include "gui/engine_integration.h"
#include "gui/panels/LogPanel.h" 

#ifdef _WIN32 // Sadece Windows için
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


int main(int argc, char *argv[])
{
    #ifdef _WIN32 // YENİ: Windows konsolunu UTF-8 olarak ayarla
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    // std::cout ve std::cerr'in de UTF-8'i desteklemesini sağlamak için
    _setmode(_fileno(stdout), _O_U8TEXT);
    _setmode(_fileno(stderr), _O_U8TEXT);
    #endif

    // En erken teşhis loglaması (hala main'in ilk satırı)
    std::ofstream early_diagnostic_log("cerebrum_lux_early_diagnostic.log", std::ios_base::app);
    if (early_diagnostic_log.is_open()) {
        early_diagnostic_log << get_current_timestamp_str() << " [EARLY DIAGNOSTIC] main function entered." << std::endl;
        early_diagnostic_log.flush();
    } else {
        std::cerr << "ERROR: Could not open early_diagnostic_log. This is a critical failure." << std::endl;
    }

    QApplication app(argc, argv);

    // Logger init'i ilk burada çağrılıyor. GUI'ye yönlendirme test için devre dışı.
    Logger::get_instance().init(LogLevel::INFO, "cerebrum_lux_gui_log.txt", "MAIN_APP"); 
    LOG(LogLevel::INFO, "Application starting up. Direct GUI logging is not active yet. Logs are buffered internally.");
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
    window.show(); // Pencereyi göster

    early_diagnostic_log << get_current_timestamp_str() << " [EARLY DIAGNOSTIC] MainWindow created and shown." << std::endl;
    early_diagnostic_log.flush();

    // GUI tamamen başlatıldıktan sonra Logger'a LogPanel'in QTextEdit'ini doğrudan bildir.
    QTextEdit* guiLogTextEdit = nullptr;
    if (window.getLogPanel()) {
        guiLogTextEdit = window.getLogPanel()->getLogTextEdit(); 
    }

    if (guiLogTextEdit) {
        Logger::get_instance().set_log_panel_text_edit(guiLogTextEdit); 
        LOG(LogLevel::INFO, "Logger: Direct GUI QTextEdit link established. Buffered logs flushed to GUI.");

        early_diagnostic_log << get_current_timestamp_str() << " [EARLY DIAGNOSTIC] Logger linked directly to GUI QTextEdit. Standard streams NOT redirected." << std::endl;
    } else {
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "ERROR: GUI LogPanel's QTextEdit could not be found for direct linking. Logs will ONLY go to file.");
        early_diagnostic_log << get_current_timestamp_str() << " [EARLY DIAGNOSTIC] ERROR: GUI LogPanel's QTextEdit not found for direct linking." << std::endl;
    }
    early_diagnostic_log.flush();


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