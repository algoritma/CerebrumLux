#ifndef CEREBRUM_LUX_META_EVOLUTION_ENGINE_H
#define CEREBRUM_LUX_META_EVOLUTION_ENGINE_H

#include <string>   // std::string için
#include <vector>   // std::vector için
#include <map>      // std::map için (gerekliyse)

#include "../core/enums.h"         // Enumlar (AIGoal) için

// Cerebrum Lux modüllerinin başlık dosyaları için TAM TANIMLAR (referans üyeleri için)
// Bu dosyalar doğrudan #include edilmelidir, çünkü MetaEvolutionEngine bu sınıflara referans üyeler tutar.
#include "../brain/intent_analyzer.h"
#include "../brain/intent_learner.h"
#include "../brain/prediction_engine.h"
#include "../planning_execution/goal_manager.h"
#include "../brain/cryptofig_processor.h"
#include "../communication/ai_insights_engine.h"
#include "../learning/LearningModule.h"
#include "../learning/KnowledgeBase.h"  // KnowledgeBase de LearningModule aracılığıyla erişildiği için tam tanım gerekli.
#include "../data_models/dynamic_sequence.h" // DynamicSequence'in tam tanımı için
#include "../data_models/sequence_manager.h" // YENİ: SequenceManager'ın tam tanımı için eklendi (run_self_simulation metodu için)


// Proje prensiplerini temsil eden yapı
struct ProjectPrinciple {
    std::string name;
    float importance_score; // 0.0 - 1.0 arası, prensibin ne kadar kritik olduğunu belirtir
};

// *** MetaEvolutionEngine: Cerebrum Lux'ın meta-yönetim katmanlarının ana koordinatörü ***
class MetaEvolutionEngine {
public:
    // Kurucu
    MetaEvolutionEngine(
        IntentAnalyzer& analyzer_ref,
        IntentLearner& learner_ref,
        PredictionEngine& predictor_ref,
        GoalManager& goal_manager_ref,
        CryptofigProcessor& cryptofig_processor_ref,
        AIInsightsEngine& insights_engine_ref,
        LearningModule& learning_module_ref
    );

    // Genel Metotlar (Boş implementasyonlu bildirimler)
    void run_meta_evolution_cycle(const DynamicSequence& current_sequence); // Ana kendini geliştirme döngüsünü orkestre eder
    void evaluate_principles_adherence(const DynamicSequence& current_sequence); // AI'ın temel prensiplere ne kadar bağlı olduğunu değerlendirir
    float calculate_overall_adherence() const; // Genel prensip bağlılık skorunu döndürür
    float simulate_change_impact(const std::string& proposed_change_description) const; // Potansiyel bir değişikliğin etkisini simüle eder
    void set_meta_goal(AIGoal new_meta_goal); // AI için meta-hedef belirler
    bool check_meta_goal_progress() const; // Meta-hedefe ulaşma ilerlemesini kontrol eder
    void analyze_architecture_cryptofig(const DynamicSequence& current_sequence); // Mevcut AI mimarisini kriptofig olarak analiz eder
    void propose_architectural_adjustment(const DynamicSequence& current_sequence); // Kriptofig analizine göre mimari ayarlamalar önerir

    // YENİ: Kendi kendini simülasyon metodunun bildirimi
    void run_self_simulation(int rounds, SequenceManager& seq_manager); 

private:
    // Bağımlılıklar (diğer AI bileşenlerine referanslar)
    IntentAnalyzer& analyzer;
    IntentLearner& learner;
    PredictionEngine& predictor;
    GoalManager& goal_manager;
    CryptofigProcessor& cryptofig_processor;
    AIInsightsEngine& insights_engine;
    LearningModule& learning_module; // YENİ

    // Meta-yönetim için özel üyeler
    std::vector<ProjectPrinciple> core_principles; // ProjectPrinciple artık tanımlı
    AIGoal current_meta_goal; // AIGoal artık tanımlı (enums.h'den)
    float current_adherence_score; // Mevcut prensip bağlılık skoru

    // Yardımcı metotlar (şimdilik bildirimleri yok, sonra eklenecek)
    // void initialize_core_principles();
    // void update_meta_goal_based_on_adherence();
};

#endif // CEREBRUM_LUX_META_EVOLUTION_ENGINE_H