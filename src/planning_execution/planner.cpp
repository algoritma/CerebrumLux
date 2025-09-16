#include "planner.h" // Kendi başlık dosyasını dahil et
#include "../core/logger.h"
#include "../core/utils.h"       // intent_to_string
#include "../data_models/dynamic_sequence.h" // DynamicSequence için
#include "../brain/intent_analyzer.h" // IntentAnalyzer için
#include "../brain/prediction_engine.h" // PredictionEngine için
#include "../communication/ai_insights_engine.h" // AIInsightsEngine için
#include "../planning_execution/goal_manager.h" // GoalManager için (gerekliyse, çapraz bağımlılık olabilir)
#include <algorithm> // std::min/max için
#include <iostream>  // std::cout, std::cerr için
#include <sstream>   // std::stringstream için

// === ActionPlanStep Implementasyonu ===
ActionPlanStep::ActionPlanStep(AIAction act, UserIntent intent, const std::string& desc)
    : action(act), triggered_by_intent(intent), description(desc) {}

// === Planner Implementasyonlari ===
Planner::Planner(IntentAnalyzer& analyzer_ref, SuggestionEngine& suggester_ref,
            GoalManager& goal_manager_ref, PredictionEngine& predictor_ref,
            AIInsightsEngine& insights_engine_ref)
    : analyzer(analyzer_ref), suggester(suggester_ref), goal_manager(goal_manager_ref),
      predictor(predictor_ref), insights_engine(insights_engine_ref) {}

std::vector<ActionPlanStep> Planner::create_action_plan(UserIntent current_intent, AbstractState current_abstract_state, AIGoal current_goal, const DynamicSequence& sequence) const {
    std::vector<ActionPlanStep> plan;

    // AIInsightsEngine'dan gelen önerileri kontrol et ve öncelik ver
    std::vector<AIInsight> insights = insights_engine.generate_insights(sequence);
    for (const auto& insight : insights) {
        if (insight.suggested_action == AIAction::SuggestSelfImprovement && insight.urgency > 0.5f) { // Yüksek aciliyetli kendi kendini geliştirme önerileri
            std::stringstream ss;
            ss << "[AI-Icgoru]: " << insight.observation << " (Aciliyet: " << static_cast<int>(insight.urgency * 100) << "%)";
            plan.emplace_back(AIAction::SuggestSelfImprovement, current_intent, ss.str());
        }
    }

    // Eğer ana hedef kendi kendini geliştirmeyse, diğer planları bypass et ve sadece buna odaklan
    if (current_goal == AIGoal::SelfImprovement) {
        if (plan.empty()) { // Eğer zaten içgörüden bir plan oluşmadıysa, genel bir kendi kendini geliştirme planı sun
            plan.emplace_back(AIAction::SuggestSelfImprovement, current_intent, "Şu anda temel hedefim kendimi geliştirmek. Bu konuda bana nasıl yardımcı olabilirsiniz?");
            plan.emplace_back(AIAction::SuggestSelfImprovement, current_intent, "Öğrenme performansımı artırmak için daha fazla veri analiz etmem gerekiyor.");
        }
        return plan; // Sadece kendi kendini geliştirme planını döndür
    }

    if (current_goal == AIGoal::OptimizeProductivity) {
        std::vector<ActionPlanStep> productivity_plan = _plan_for_productivity(current_intent, current_abstract_state, sequence);
        plan.insert(plan.end(), productivity_plan.begin(), productivity_plan.end());
    } else if (current_goal == AIGoal::MaximizeBatteryLife) {
        std::vector<ActionPlanStep> battery_plan = _plan_for_battery_life(current_intent, current_abstract_state, sequence);
        plan.insert(plan.end(), battery_plan.begin(), battery_plan.end());
    } else if (current_goal == AIGoal::ReduceDistractions) {
        if (current_abstract_state == AbstractState::Distracted || current_intent == UserIntent::IdleThinking) {
            plan.emplace_back(AIAction::MuteNotifications, current_intent, "Dikkat dağıtıcıları azaltmak için tüm bildirimleri sessize al.");
            plan.emplace_back(AIAction::DimScreen, current_intent, "Odaklanmayı artırmak için ekranı karart.");
            plan.emplace_back(AIAction::LaunchApplication, current_intent, "Odaklanmanızı artırmak için ana uygulamanızı tekrar açın.");
        } else {
            plan.emplace_back(AIAction::None, current_intent, "Dikkat dağıtıcıları azaltmaya yönelik özel bir eylem önerisi yok.");
        }
    } 
    // YENİ HEDEF PLANLARI
    else if (current_goal == AIGoal::EnhanceCreativity) {
        if (current_intent == UserIntent::CreativeWork || current_abstract_state == AbstractState::CreativeFlow) {
            plan.emplace_back(AIAction::MuteNotifications, current_intent, "Yaratıcılığınızı artırmak için bildirimleri sessize alın.");
            plan.emplace_back(AIAction::SuggestBreak, current_intent, "Kısa bir mola yeni fikirler getirebilir.");
        }
        else {
            plan.emplace_back(AIAction::None, current_intent, "Yaratıcılığı artırmaya yönelik özel bir eylem önerisi yok.");
        }
    } else if (current_goal == AIGoal::ImproveGamingExperience) {
        if (current_intent == UserIntent::Gaming) {
            plan.emplace_back(AIAction::OptimizeForGaming, current_intent, "Oyun deneyiminizi optimize etmek için sistem ayarlarını düzenle.");
            plan.emplace_back(AIAction::MuteNotifications, current_intent, "Oyun sırasında dikkatinizi dağıtmamak için bildirimleri kapat.");
        }
        else {
            plan.emplace_back(AIAction::None, current_intent, "Oyun deneyimini iyileştirmeye yönelik özel bir eylem önerisi yok.");
        }
    }
    else if (current_goal == AIGoal::FacilitateResearch) {
        if (current_intent == UserIntent::Research || current_abstract_state == AbstractState::SeekingInformation) {
            plan.emplace_back(AIAction::EnableFocusMode, current_intent, "Araştırmaya odaklanmak için özel bir 'Odak Modu' etkinleştir.");
            plan.emplace_back(AIAction::OpenDocumentation, current_intent, "İlgili dokümantasyonu veya web kaynaklarını otomatik olarak aç.");
        }
        else {
            plan.emplace_back(AIAction::None, current_intent, "Araştırmayı kolaylaştırmaya yönelik özel bir eylem önerisi yok.");
        }
    }
    else {
        // Eğer hiçbir hedefe yönelik plan oluşturulmadıysa ve içgörü de yoksa, genel fallback planı
        if (plan.empty()) { 
             plan.emplace_back(AIAction::None, current_intent, "Mevcut hedefinize yönelik özel bir plan oluşturulamadı. AI öğrenmeye devam ediyor.");
        }
    }
    
    // Genel fallback planları (eğer plan hala boşsa veya sadece 'None' içeriyorsa)
    if (plan.empty() || (plan.size() == 1 && plan[0].action == AIAction::None && current_intent != UserIntent::Unknown && current_intent != UserIntent::None)) {
        plan.clear(); 
        plan.emplace_back(AIAction::None, current_intent, "AI, ogrenmeye devam ediyor. Lutfen etkilesiminize devam edin.");
        plan.emplace_back(AIAction::None, current_intent, "Belki hizli yazmaya baslamak istersiniz?");
        plan.emplace_back(AIAction::None, current_intent, "Veya bir seyleri duzenlemeye baslamak?");
    }
    return plan;
}

void Planner::execute_plan(const std::vector<ActionPlanStep>& plan) {
    if (plan.empty()) {
        LOG_DEFAULT(LogLevel::INFO, "[AI-Planlayici] Bos eylem plani.\n");
        return;
    }
    LOG_DEFAULT(LogLevel::INFO, "[AI-Planlayici] Eylem plani yurutuluyor:\n");
    for (const auto& step : plan) {
        LOG_DEFAULT(LogLevel::INFO, "  - " << step.description << " (Tetkikleyen Niyet: " << intent_to_string(step.triggered_by_intent) << ")\n");
        
        if (step.action == AIAction::DimScreen) {
            LOG_DEFAULT(LogLevel::INFO, "    [AI-Eylem] Ekrani karartma eylemi simule ediliyor.\n");
        } else if (step.action == AIAction::MuteNotifications) {
            LOG_DEFAULT(LogLevel::INFO, "    [AI-Eylem] Bildirimleri sessize alma eylemi simule ediliyor.\n");
        } else if (step.action == AIAction::LaunchApplication) {
            LOG_DEFAULT(LogLevel::INFO, "    [AI-Eylem] Uygulama baslatma eylemi simule ediliyor.\n");
        } else if (step.action == AIAction::SetReminder) {
            LOG_DEFAULT(LogLevel::INFO, "    [AI-Eylem] Hatirlatici kurma eylemi simule ediliyor.\n");
        }
        // YENİ EYLEMLERİN SİMÜLASYONU
        else if (step.action == AIAction::SuggestBreak) {
            LOG_DEFAULT(LogLevel::INFO, "    [AI-Eylem] Ara verme onerisi simule ediliyor.\n");
        } else if (step.action == AIAction::OptimizeForGaming) {
            LOG_DEFAULT(LogLevel::INFO, "    [AI-Eylem] Oyun performansi optimizasyonu simule ediliyor.\n");
        } else if (step.action == AIAction::EnableFocusMode) {
            LOG_DEFAULT(LogLevel::INFO, "    [AI-Eylem] Odaklanma modu etkinlestirme simule ediliyor.\n");
        } else if (step.action == AIAction::AdjustAudioVolume) {
            LOG_DEFAULT(LogLevel::INFO, "    [AI-Eylem] Ses seviyesi ayari simule ediliyor.\n");
        } else if (step.action == AIAction::OpenDocumentation) {
            LOG_DEFAULT(LogLevel::INFO, "    [AI-Eylem] Dokümantasyon acma simule ediliyor.\n");
        } else if (step.action == AIAction::SuggestSelfImprovement) { 
            LOG_DEFAULT(LogLevel::INFO, "    [AI-Eylem] Kendi kendini geliştirme önerisi simule ediliyor: " << step.description << "\n");
        }
    }
}

std::vector<ActionPlanStep> Planner::_plan_for_productivity(UserIntent current_intent, AbstractState current_abstract_state, const DynamicSequence& sequence) const { 
    std::vector<ActionPlanStep> plan;

    if (current_intent == UserIntent::FastTyping) {
        if (sequence.network_activity_level / 15000.0f > 0.7f) { 
            plan.emplace_back(AIAction::DimScreen, current_intent, "Yuksek ag aktivitesi fark edildi, odaklanmak için ekrani biraz karart.");
        }
        plan.emplace_back(AIAction::DisableSpellCheck, current_intent, "Hizinizi kesmemek icin yazim denetimini devre disi birak.");
        plan.emplace_back(AIAction::EnableCustomDictionary, current_intent, "Terminolojinizi kolaylastirmak icin ozel sozluk kullan.");
        plan.emplace_back(AIAction::OpenFile, current_intent, "Hizli bir sekilde yeni bir dokuman acarak uretkenligini surdur.");
        plan.emplace_back(AIAction::None, current_intent, "Harika bir hizla yaziyorsunuz! Odaginizi koruyun.");

    } else if (current_intent == UserIntent::Editing) {
        if (sequence.keystroke_variability / 1000.0f > 0.75f || sequence.control_key_frequency > 0.5f) { 
            plan.emplace_back(AIAction::ShowUndoHistory, current_intent, "Duzenleme paterninde duzensizlikler var, geri alma gecmisini kontrol et.");
        }
        plan.emplace_back(AIAction::CompareVersions, current_intent, "Duzenleme islemini tamamlamadan once versiyonlari karsilastirarak guvenlik onlemi al.");
        plan.emplace_back(AIAction::SimulateOSAction, current_intent, "Duzenleme araclarini optimize etmek icin bir OS eylemi simule et (örneğin, dosya yedekleme).");
        plan.emplace_back(AIAction::SetReminder, current_intent, "Bu gorevi bitirdiginizde hatirlatici kurun: 'Bitirildi'.");
        plan.emplace_back(AIAction::None, current_intent, "Duzenleme modundasiniz. Odaklanmanize yardimci olabilirim.");

    } else if (current_intent == UserIntent::IdleThinking) {
        if (sequence.mouse_movement_intensity / 500.0f > 0.1f || sequence.mouse_click_frequency > 0.1f || sequence.network_activity_level / 15000.0f > 0.1f) { 
            plan.emplace_back(AIAction::MuteNotifications, current_intent, "Dusunme/ara verme modundayken dikkat dagiticilari azaltmak icin bildirimleri sessize al.");
            plan.emplace_back(AIAction::DimScreen, current_intent, "Odaklanmayi artirmak icin ekrani karart.");
        }
        plan.emplace_back(AIAction::SetReminder, current_intent, "Dusunme suren bittiginde seni uyarmasi icin bir hatirlatici kur.");
        plan.emplace_back(AIAction::LaunchApplication, current_intent, "Dusunme/ara verme sonrasi ihtiyac duyacagin bir uygulamayi (örn. metin editoru) baslat.");
        plan.emplace_back(AIAction::EnableCustomDictionary, current_intent, "Dusunme sonrasi hizli yazima gecis icin ozel sozluk etkinlestiriliyor.");
        plan.emplace_back(AIAction::None, current_intent, "Su an dusunuyor gibi gorunuyorsunuz. Belki bir sonraki adimi planlayabiliriz.");
    
    } else if (current_intent == UserIntent::Programming) { 
        plan.emplace_back(AIAction::OpenDocumentation, current_intent, "Programlama modundasınız. Gerekirse ilgili dokümanları açabilirim.");
        if (current_abstract_state == AbstractState::Debugging) {
             plan.emplace_back(AIAction::ShowUndoHistory, current_intent, "Hata ayıklama yapıyor gibisiniz. Geri alma geçmişini veya versiyonları kontrol edelim mi?");
        }
        plan.emplace_back(AIAction::EnableFocusMode, current_intent, "Odaklanmanızı sağlamak için 'Odak Modu'nu etkinleştir.");
    } else if (current_intent == UserIntent::Gaming) { 
        plan.emplace_back(AIAction::OptimizeForGaming, current_intent, "Oyun oynuyor gibisiniz. Sistem performansını oyun için optimize edelim mi?");
        plan.emplace_back(AIAction::MuteNotifications, current_intent, "Oyun sırasında dikkatinizi dağıtmamak için bildirimleri sessize alalım.");
        plan.emplace_back(AIAction::AdjustAudioVolume, current_intent, "Oyun için ses seviyesini optimize edelim mi?");
    } else if (current_intent == UserIntent::MediaConsumption) { 
        plan.emplace_back(AIAction::DimScreen, current_intent, "Medya tüketiyorsunuz. Ekranı daha da karartarak pil tasarrufu sağlayabiliriz.");
        plan.emplace_back(AIAction::MuteNotifications, current_intent, "Kesintisiz bir deneyim için bildirimleri sessize alalım.");
        plan.emplace_back(AIAction::AdjustAudioVolume, current_intent, "Medya için ses seviyesini ayarla.");
    } else if (current_intent == UserIntent::CreativeWork) { 
        plan.emplace_back(AIAction::EnableFocusMode, current_intent, "Yaratıcı çalışıyorsunuz. Odaklanma modu ile tüm dikkat dağıtıcıları engelleyelim.");
        plan.emplace_back(AIAction::SuggestBreak, current_intent, "Yaratıcılığınızı tazelemek için kısa bir mola öneririm.");
    }
    else if (current_intent == UserIntent::Research) { 
        plan.emplace_back(AIAction::EnableFocusMode, current_intent, "Araştırma yapıyor gibisiniz. Verimli okuma için odak modunu etkinleştir.");
        plan.emplace_back(AIAction::OpenDocumentation, current_intent, "Araştırmanızla ilgili kaynakları otomatik olarak açabilirim.");
    }
    else if (current_intent == UserIntent::Communication) { 
        plan.emplace_back(AIAction::MuteNotifications, current_intent, "İletişim kuruyorsunuz. Gereksiz bildirimleri sessize alarak daha akıcı bir sohbet sağlayabiliriz.");
        plan.emplace_back(AIAction::EnableCustomDictionary, current_intent, "Hızlı ve doğru yazım için özel sözlüğü etkinleştir.");
    }
    else if (current_intent == UserIntent::Unknown) { 
        plan.emplace_back(AIAction::None, current_intent, "Mevcut niyetiniz belirsiz. Daha fazla veri toplanarak ogrenme devam ediyor.");
        plan.emplace_back(AIAction::MuteNotifications, current_intent, "Belirsiz durumda dikkat dagiticilari azaltmak faydali olabilir.");
    }
    
    if (current_abstract_state == AbstractState::Distracted || current_abstract_state == AbstractState::LowProductivity) {
        bool already_suggested_mute = false;
        bool already_suggested_dim = false;
        for(const auto& step : plan) {
            if (step.action == AIAction::MuteNotifications) already_suggested_mute = true;
            if (step.action == AIAction::DimScreen) already_suggested_dim = true;
        }
        if (!already_suggested_mute) plan.emplace_back(AIAction::MuteNotifications, current_intent, "Genel uretkenlik dusukse tum bildirimleri kapatmak iyi bir fikir olabilir.");
        if (!already_suggested_dim) plan.emplace_back(AIAction::DimScreen, current_intent, "Odaklanmayi artirmak icin ekrani karart.");
    }
    // Hardware Anomaly durumunda özel planlar
    if (current_abstract_state == AbstractState::HardwareAnomaly) {
        plan.clear(); // Diğer tüm planları geçersiz kıl, acil durum!
        plan.emplace_back(AIAction::SimulateOSAction, current_intent, "Donanım anormalliği tespit edildi! Sistem kontrolü ve teşhis başlatılıyor.");
        if (sequence.current_battery_percentage < 30 && !sequence.current_battery_charging) {
            plan.emplace_back(AIAction::None, current_intent, "Kritik batarya seviyesi. Lütfen şarj cihazını takın.");
        }
        if (sequence.network_activity_level == 0 && sequence.current_network_active) {
            plan.emplace_back(AIAction::SimulateOSAction, current_intent, "Ağ bağlantısı sorunları olabilir. Bağlantıları kontrol et.");
        }
    }


    if (plan.empty() || (plan.size() == 1 && plan[0].action == AIAction::None && current_intent != UserIntent::Unknown && current_intent != UserIntent::None)) {
        plan.clear(); 
        plan.emplace_back(AIAction::None, current_intent, "AI, ogrenmeye devam ediyor. Lutfen etkilesiminize devam edin.");
        plan.emplace_back(AIAction::None, current_intent, "Belki hizli yazmaya baslamak istersiniz?");
        plan.emplace_back(AIAction::None, current_intent, "Veya bir seyleri duzenlemeye baslamak?");
    }
    return plan;
}

std::vector<ActionPlanStep> Planner::_plan_for_battery_life(UserIntent current_intent, AbstractState current_abstract_state, const DynamicSequence& sequence) const { 
    std::vector<ActionPlanStep> plan; 

    if (sequence.current_battery_percentage < 25 && !sequence.current_battery_charging) {
        plan.emplace_back(AIAction::DimScreen, current_intent, "Batarya kritik seviyede, ekrani hemen karartiliyor.");
        plan.emplace_back(AIAction::MuteNotifications, current_intent, "Batarya kritik seviyede, tum bildirimler sessize aliniyor.");
        plan.emplace_back(AIAction::None, current_intent, "Lutfen sarj aletinizi takin.");
        plan.emplace_back(AIAction::SimulateOSAction, current_intent, "Kritik batarya icin OS guc tasarrufu modunu etkinlestir.");
    } else if (sequence.current_battery_percentage < 45 && !sequence.current_battery_charging) {
        plan.emplace_back(AIAction::DimScreen, current_intent, "Batarya dusuk, ekrani karartiliyor.");
        plan.emplace_back(AIAction::MuteNotifications, current_intent, "Batarya dusuk, bildirimler sessize aliniyor.");
    } else if (sequence.current_battery_charging) {
        if (sequence.avg_brightness / 255.0f > 0.7f) { 
             plan.emplace_back(AIAction::DimScreen, current_intent, "Batarya sarj oluyor ama ekran çok parlak, parlakligi azalt.");
        }
        if (sequence.network_activity_level / 10000.0f > 0.8f) { 
             plan.emplace_back(AIAction::None, current_intent, "Yuksek ag aktivitesi, batarya sarj olurken enerji tuketimini dusurmek için dikkat et.");
        }
    }
    
    if (plan.empty()) {
        plan.emplace_back(AIAction::None, current_intent, "Batarya tasarrufu için ozel bir eylem onerisi yok.");
    }
    return plan;
}
