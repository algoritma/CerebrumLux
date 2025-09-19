#include <QApplication>
#include <QTimer>
#include "gui/MainWindow.h"
#include "gui/engine_integration.h"

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
    QApplication app(argc, argv);

    Logger::get_instance().init(LogLevel::INFO, "cerebrum_lux_gui_log.txt");

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
    // ResponseEngine kurucusundaki NaturalLanguageProcessor parametresi pointer'a çevrildi
    ResponseEngine responder(analyzer, goal_manager, insights_engine, &nlp); // DÜZELTİLDİ: &nlp eklendi

    // --- Learning Module ---
    KnowledgeBase kb;
    kb.load("knowledge.json");
    LearningModule learning_module(kb);

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

    // --- GUI entegrasyonu ---
    EngineIntegration integration(meta_engine, sequenceManager, learning_module, kb);
    MainWindow window(integration, learning_module); 
    window.show();

    // --- Engine döngüsü ---
    QTimer engineTimer;
    QObject::connect(&engineTimer, &QTimer::timeout, [&](){
        meta_engine.run_meta_evolution_cycle(sequenceManager.get_current_sequence_ref()); 
    });
    engineTimer.start(1000);

    int result = app.exec();

    kb.save("knowledge.json");
    return result;
}