#include "meta_evolution_engine.h" // Kendi başlık dosyasını dahil et
#include "../core/logger.h"        // LOG makrosu için
#include "../core/utils.h"         // intent_to_string, goal_to_string vb. için
#include "../data_models/dynamic_sequence.h" // DynamicSequence'in tam tanımı için
#include <iostream>                // Debug çıktıları için
#include <numeric>                 // std::accumulate için (gerekliyse)
#include <algorithm>               // std::min/max için (gerekliyse)
#include <chrono>                  // YENİ: AIInsight timestamp için
#include <string>                  // YENİ: std::to_string için
#include <iomanip>                 // std::setprecision için
#include "../communication/ai_insights_engine.h" // YENİ: AIInsight tanımı için (Gerekli olduğu için tekrar ekleniyor)
#include "../core/enums.h"         // AIAction için

// === MetaEvolutionEngine Implementasyonlari ===

// Kurucu - LearningModule referansı alacak şekilde güncellendi
MetaEvolutionEngine::MetaEvolutionEngine(
    IntentAnalyzer& analyzer_ref,
    IntentLearner& learner_ref,
    PredictionEngine& predictor_ref,
    GoalManager& goal_manager_ref,
    CryptofigProcessor& cryptofig_processor_ref,
    AIInsightsEngine& insights_engine_ref,
    LearningModule& learning_module_ref
) : 
    analyzer(analyzer_ref),
    learner(learner_ref),
    predictor(predictor_ref),
    goal_manager(goal_manager_ref),
    cryptofig_processor(cryptofig_processor_ref),
    insights_engine(insights_engine_ref),
    learning_module(learning_module_ref), // YENİ: LearningModule referansı başlatıldı
    current_meta_goal(AIGoal::SelfImprovement), // Varsayılan meta-hedef: Kendi Kendini Geliştirmek
    current_adherence_score(1.0f) // Başlangıçta tam bağlılık varsayımı
{
    // Temel prensipleri başlat
    core_principles.push_back({"Ultra-Verimlilik", 0.9f});
    core_principles.push_back({"Adaptiflik", 0.8f});
    core_principles.push_back({"Esneklik", 0.7f});
    core_principles.push_back({"Modülerlik", 0.7f});
    core_principles.push_back({"Veri Ekonomisi", 0.6f});
    // Diğer prensipler eklenebilir

    LOG_DEFAULT(LogLevel::INFO, "MetaEvolutionEngine: Başlatıldı. Varsayılan meta-hedef: " << goal_to_string(current_meta_goal) << ". LearningModule entegrasyonu için ön hazırlık yapıldı.");
}

// Ana kendini geliştirme döngüsünü orkestre eder
void MetaEvolutionEngine::run_meta_evolution_cycle(const DynamicSequence& current_sequence) {
    LOG_DEFAULT(LogLevel::INFO, "MetaEvolutionEngine: Meta-evrim döngüsü çalıştırılıyor...");
    
    // 1. Prensip Bağlılığını Değerlendir
    evaluate_principles_adherence(current_sequence);
    LOG_DEFAULT(LogLevel::INFO, "MetaEvolutionEngine: Mevcut prensip bağlılık skoru: " << current_adherence_score);

    // 2. Meta-Hedef İlerlemesini Kontrol Et
    if (check_meta_goal_progress()) {
        LOG_DEFAULT(LogLevel::INFO, "MetaEvolutionEngine: Meta-hedef (" << goal_to_string(current_meta_goal) << ") üzerinde ilerleme kaydedildi.");
        // İlerlemeye göre yeni bir meta-hedef belirlenebilir veya mevcut hedef pekiştirilebilir.
    } else {
        LOG_DEFAULT(LogLevel::WARNING, "MetaEvolutionEngine: Meta-hedef (" << goal_to_string(current_meta_goal) << ") üzerinde istenen ilerleme kaydedilemedi.");
        // Gelişimi hızlandırmak için ek stratejiler devreye alınabilir.
    }

    // YENİ: LearningModule'e güncel içgörüleri işle
    // Bu metodun AIInsightsEngine'da generate_insights() olarak tanımlı olduğunu varsayıyoruz.
    learning_module.process_ai_insights(insights_engine.generate_insights(current_sequence));

    // ACİL ÖNCELİK: GraphPanel'e Anlamlı Veri Besleme için AIInsight oluştur
    static size_t simulatedStepCount = 0;
    simulatedStepCount++;

    // AIInsight struct'ı sadece observation, suggested_action, urgency alıyor.
    // GraphPanel için değeri urgency alanına yerleştiriyoruz.
    // "StepSimulation" topic'i LearningModule tarafından Capsule oluşturulurken ayarlanmalı.
    AIInsight insight(
        "Simulating meta-evolution step: " + std::to_string(simulatedStepCount), // observation
        AIAction::None, // Düzeltildi: AIAction::Monitor yerine AIAction::None kullanıldı
        static_cast<float>(simulatedStepCount % 100) // urgency (GraphPanel için kullanılacak değer)
    );

    // LearningModule'ün process_ai_insights metodu, AIInsight'ı alıp Capsule'a dönüştürürken,
    // muhtemelen topic'i ve diğer meta verileri de belirleyecek.
    // Bu durumda, AIInsight'ın urgency'sini Capsule'ın confidence/value alanına aktaracağını varsayıyoruz.
    learning_module.process_ai_insights({insight});

    // Log mesajındaki 'value' yerine 'urgency' kullanıldı
    LOG_DEFAULT(LogLevel::DEBUG, "MetaEvolutionEngine: Generated 'StepSimulation' insight with urgency " << insight.urgency);

    // 3. Mimariyi Kriptofig Olarak Analiz Et ve Ayarlamalar Öner
    analyze_architecture_cryptofig(current_sequence);
    propose_architectural_adjustment(current_sequence);

    LOG_DEFAULT(LogLevel::INFO, "MetaEvolutionEngine: Meta-evrim döngüsü tamamlandı.");
}

// AI'ın temel prensiplere ne kadar bağlı olduğunu değerlendirir
void MetaEvolutionEngine::evaluate_principles_adherence(const DynamicSequence& current_sequence) {
    LOG_DEFAULT(LogLevel::DEBUG, "MetaEvolutionEngine: Prensip bağlılığı değerlendiriliyor...");
    float total_weighted_score = 0.0f;
    float total_importance = 0.0f;

    // Örnek: "Ultra-Verimlilik" prensibi için değerlendirme
    // Düşük CPU/RAM kullanımı, hızlı yanıt süreleri gibi metriklerle ilişkilendirilebilir.
    // Şimdilik basitleştirilmiş bir simülasyon yapalım.
    float efficiency_score = 0.0f;
    if (current_sequence.avg_keystroke_interval > 0.0f && current_sequence.avg_keystroke_interval < 200000.0f) { // Yüksek yazım hızı
        efficiency_score = 0.7f;
    } else {
        efficiency_score = 0.3f;
    }
    // Network aktivitesi de verimliliği etkileyebilir
    if (current_sequence.network_activity_level > 10000.0f && current_sequence.current_network_active) { 
        efficiency_score *= 0.8f; // Yüksek ağ aktivitesi verimliliği düşürebilir
    }

    // "Adaptiflik" prensibi için değerlendirme
    // AI'ın farklı niyetlere veya durumlara ne kadar hızlı adapte olduğuyla ilişkilendirilebilir.
    // Şimdilik, tahmin motorunun geçmiş performansı gibi bir metrik kullanabiliriz.
    float adaptability_score = 0.0f;
    // PredictionEngine'dan veya IntentLearner'dan adaptasyon metrikleri alınabilir.
    // Örneğin: learner.get_learning_rate() ne kadar hızlı değişiyor?
    adaptability_score = learner.get_learning_rate() * 5.0f; // Öğrenme oranı yüksekse daha adaptif varsayalım
    adaptability_score = std::min(1.0f, adaptability_score);

    // Her prensip için skorları topla
    for (const auto& principle : core_principles) {
        float principle_current_score = 0.0f;
        if (principle.name == "Ultra-Verimlilik") {
            principle_current_score = efficiency_score;
        } else if (principle.name == "Adaptiflik") {
            principle_current_score = adaptability_score;
        } else {
            principle_current_score = 0.5f; // Diğerleri için varsayılan orta skor
        }
        total_weighted_score += principle_current_score * principle.importance_score;
        total_importance += principle.importance_score;
    }

    if (total_importance > 0.0f) {
        current_adherence_score = total_weighted_score / total_importance;
    } else {
        current_adherence_score = 0.0f;
    }
    LOG_DEFAULT(LogLevel::DEBUG, "MetaEvolutionEngine: Prensip bağlılığı değerlendirmesi tamamlandı. Skor: " << current_adherence_score);
}

// Genel prensip bağlılık skorunu döndürür
float MetaEvolutionEngine::calculate_overall_adherence() const {
    return current_adherence_score;
}

// Potansiyel bir değişikliğin etkisini simüle eder
float MetaEvolutionEngine::simulate_change_impact(const std::string& proposed_change_description) const {
    LOG_DEFAULT(LogLevel::INFO, "MetaEvolutionEngine: Değişiklik etkisi simüle ediliyor: '" << proposed_change_description << "'");
    // Bu, karmaşık bir simülasyon motoru gerektiren kritik bir metod olacaktır.
    // Örneğin, belirli bir kod değişikliğinin performans, enerji tüketimi veya adaptasyon üzerindeki etkisini tahmin edebiliriz.
    // Şimdilik, basit bir rastgele veya kural tabanlı tahmin döndürelim.
    if (proposed_change_description.find("performans") != std::string::npos) {
        return 0.8f; // Performans artışı öngörülebilir
    } else if (proposed_change_description.find("bug") != std::string::npos) {
        return 0.3f; // Hata düzeltme, ancak başka sorunlar yaratabilir
    }
    return 0.5f; // Nötr etki
}

// AI için meta-hedef belirler
void MetaEvolutionEngine::set_meta_goal(AIGoal new_meta_goal) {
    current_meta_goal = new_meta_goal;
    LOG_DEFAULT(LogLevel::INFO, "MetaEvolutionEngine: Yeni meta-hedef ayarlandı: " << goal_to_string(current_meta_goal));
}

// Meta-hedefe ulaşma ilerlemesini kontrol eder
bool MetaEvolutionEngine::check_meta_goal_progress() const {
    LOG_DEFAULT(LogLevel::DEBUG, "MetaEvolutionEngine: Meta-hedef ilerlemesi kontrol ediliyor: " << goal_to_string(current_meta_goal));
    // Bu metod, current_meta_goal'a ulaşmak için AI'ın ne kadar ilerleme kaydettiğini değerlendirir.
    // Örneğin, AIGoal::SelfImprovement ise, IntentLearner'ın veya PredictionEngine'ın hata oranları düşüyor mu?
    if (current_meta_goal == AIGoal::SelfImprovement) {
        // Basit bir örnek: Öğrenme oranı artıyorsa veya hata oranı düşüyorsa ilerleme var diyelim.
        // Daha gerçekçi bir senaryo için AIInsightsEngine'dan metrikler alınabilir.
        if (learner.get_learning_rate() > 0.05f) { // Yüksek öğrenme oranı bir ilerleme işareti olabilir
            return true;
        }
    }
    // Diğer meta-hedefler için benzer mantıklar eklenebilir.
    return false; // Şimdilik varsayılan: ilerleme yok
}

// Mevcut AI mimarisini kriptofig olarak analiz eder
void MetaEvolutionEngine::analyze_architecture_cryptofig(const DynamicSequence& current_sequence) {
    LOG_DEFAULT(LogLevel::INFO, "MetaEvolutionEngine: AI mimarisi kriptofig olarak analiz ediliyor...");
    // Bu metod, AI'ın kendi iç yapısını ve davranışını "kriptofigler" aracılığıyla temsil etmesini sağlar.
    // Örneğin, farklı modüllerin (analyzer, learner, predictor) birbirleriyle olan etkileşimleri,
    // ağırlık dağılımları veya öğrenme eğrileri kriptofig olarak kodlanabilir.
    // Şimdilik, sadece mevcut sequence'in latent kriptofig'ini kullanarak bir loglama yapalım.
    if (!current_sequence.latent_cryptofig_vector.empty()) {
        std::stringstream ss;
        ss << std::fixed << std::setprecision(2);
        ss << "MetaEvolutionEngine: Mevcut mimari kriptofig (latent): [";
        for (size_t i = 0; i < current_sequence.latent_cryptofig_vector.size(); ++i) {
            ss << current_sequence.latent_cryptofig_vector[i];
            if (i + 1 < current_sequence.latent_cryptofig_vector.size()) ss << ", ";
        }
        ss << "]";
        LOG_DEFAULT(LogLevel::DEBUG, ss.str());
    } else {
        LOG_DEFAULT(LogLevel::WARNING, "MetaEvolutionEngine: Mimari analizi için latent kriptofig bulunamadı.");
    }
}

// Kriptofig analizine göre mimari ayarlamalar önerir
void MetaEvolutionEngine::propose_architectural_adjustment(const DynamicSequence& current_sequence) {
    LOG_DEFAULT(LogLevel::INFO, "MetaEvolutionEngine: Mimari ayarlamalar öneriliyor...");
    // Bu metod, analyze_architecture_cryptofig'den elde edilen içgörülere dayanarak
    // AI'ın kendi mimarisinde yapısal veya algoritmik değişiklikler önerebilir.
    // Örneğin, belirli bir niyet şablonunun ağırlıklarının ayarlanması,
    // bir öğrenme hızının değiştirilmesi veya hatta yeni bir modülün entegrasyonu gibi.
    
    // Basit bir örnek: Eğer prensip bağlılık skoru düşükse, adaptasyon prensibini vurgula
    if (current_adherence_score < 0.6f) {
        LOG_DEFAULT(LogLevel::INFO, "MetaEvolutionEngine: Düşük prensip bağlılık skoru nedeniyle 'Adaptiflik' prensibine odaklanma öneriliyor.");
        LOG_DEFAULT(LogLevel::INFO, "MetaEvolutionEngine: Öneri: IntentLearner'ın öğrenme hızını artırın veya PredictionEngine'ın adaptasyon parametrelerini gözden geçirin.");
    }
    // TODO: Daha karmaşık öneri mekanizmaları eklenecek
}