#include "engine_integration.h"
#include "../core/logger.h" // Logger'a erişim için
#include "../learning/LearningModule.h" // Öğrenme modülü için
#include "../learning/KnowledgeBase.h" // YENİ: Bilgi tabanı için (nokta hatası düzeltildi)
#include "../meta/meta_evolution_engine.h" // YENİ: MetaEvolutionEngine'ın tam tanımı için eklendi


// Constructor güncellendi
EngineIntegration::EngineIntegration(MetaEvolutionEngine& meta, SequenceManager& seq, LearningModule& learner_ref, KnowledgeBase& kb_ref)
    : metaEngine(meta), sequenceManager(seq), learningModule(learner_ref), knowledgeBase(kb_ref)
{
}

// Fonksiyon implementasyonları
void EngineIntegration::runSelfSimulation(int steps) {
    // Simülasyon kodları
    // Örneğin, metaEngine.run_meta_evolution_cycle() çağrılabilir
    (void)steps; // Kullanılmayan parametre uyarısını engelle
    LOG_DEFAULT(LogLevel::INFO, "[EngineIntegration] runSelfSimulation cagrildi.\n");
}

std::string EngineIntegration::getLatestLogs() {
    // Logger'dan en son logları alacak bir mekanizma burada olmalıydı.
    // Şimdilik sadece bir placeholder döndürüyoruz.
    // Gerçek implementasyon için Logger sınıfının logları bir buffer'da tutması gerekir.
    LOG_DEFAULT(LogLevel::TRACE, "[EngineIntegration] getLatestLogs cagrildi, bos string donduruluyor.\n");
    return "Simulasyon loglari..."; // Örnek bir log
}

float EngineIntegration::getCurrentAdherenceScore() {
    // MetaEvolutionEngine'dan prensip bağlılık skorunu döndür
    return metaEngine.calculate_overall_adherence(); 
}

std::vector<SimulationData> EngineIntegration::getSimulationData() {
    // Gerçek simülasyon verilerini döndür
    return {}; // Örnek
}

std::vector<LogData> EngineIntegration::getLogData() {
    // Gerçek log verilerini döndür (LogPanel'in beklediği formatta)
    return {}; // Örnek
}

std::vector<GraphData> EngineIntegration::getGraphData() {
    // Gerçek grafik verilerini döndür
    return {}; // Örnek
}