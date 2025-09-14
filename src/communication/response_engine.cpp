#include "response_engine.h" // Kendi başlık dosyasını dahil et
#include "../core/logger.h"      // LOG makrosu için
#include "../core/utils.h"       // intent_to_string, abstract_state_to_string için
#include "../data_models/dynamic_sequence.h" // DynamicSequence için
#include "../brain/intent_analyzer.h" // IntentAnalyzer için
#include "../planning_execution/goal_manager.h" // GoalManager için
#include "ai_insights_engine.h" // AIInsightsEngine için
#include "../brain/autoencoder.h" // CryptofigAutoencoder::LATENT_DIM için
#include <iomanip>   // std::fixed, std::setprecision için
#include <algorithm> // std::min/max için
#include <iostream>  // std::wcout, std::wcerr için

// === ResponseEngine Implementasyonlari ===
ResponseEngine::ResponseEngine(IntentAnalyzer& analyzer_ref, GoalManager& goal_manager_ref, AIInsightsEngine& insights_engine_ref) 
    : analyzer(analyzer_ref), goal_manager(goal_manager_ref), insights_engine(insights_engine_ref), gen(rd()) { 
    
    // Mevcut yanıt şablonları
    response_templates[UserIntent::FastTyping][AbstractState::HighProductivity].responses = {
        L"Harika bir hizla yaziyorsunuz! Odaginizi koruyun. ",
        L"Uretkenliginiz zirvede, devam edin! ",
        L"Yazim akiciliginiz etkileyici. Su anki akisini bozmayalim. "
    };
    response_templates[UserIntent::FastTyping][AbstractState::NormalOperation].responses = {
        L"Hizli yazim modunda gibisiniz. Daha verimli olmak için önerilerim olabilir mi? ",
        L"Hizli yaziminizda dikkat dagiticilari azaltmak ister misiniz? "
    };

    response_templates[UserIntent::Editing][AbstractState::Focused].responses = {
        L"Duzenleme modundasiniz ve odaklanmissiniz. Herhangi bir yardima ihtiyaciniz var mi? ",
        L"Metninizi titizlikle düzenliyorsunuz, takdire şayan. "
    };
    response_templates[UserIntent::Editing][AbstractState::NormalOperation].responses = {
        L"Geri alma gecmisinizi gostermemi ister misiniz? ",
        L"Duzenleme isleminizi kolaylastiracak bir eylem önerebilirim. "
    };

    response_templates[UserIntent::IdleThinking][AbstractState::LowProductivity].responses = {
        L"Biraz düşünme modunda gibisiniz. Yeni bir göreve mi geçmek istersiniz? ",
        L"Görüyorum ki şu an aktif bir etkileşim yok. Belki de bir ara vermeye ihtiyacınız var? ",
        L"Herkesin düşünmeye ihtiyacı vardır. Hazır olduğunuzda bana söyleyin. "
    };
    response_templates[UserIntent::IdleThinking][AbstractState::NormalOperation].responses = {
        L"Boşta olduğunuzu algıladım. Belki bir hatırlatıcı kurmak ister misiniz? ",
        L"Uzun süredir aktif değilsiniz. Ekranı karartmak veya bildirimleri sessize almak ister misiniz?"
    };

    response_templates[UserIntent::Unknown][AbstractState::None].responses = {
        L"Mevcut niyetiniz belirsiz. Daha fazla veri toplanarak öğrenmeye devam ediyorum. ",
        L"Davranışınızı anlamaya çalışıyorum. Lütfen etkileşiminize devam edin. "
    };
    response_templates[UserIntent::Unknown][AbstractState::LowProductivity].responses = {
        L"Şu anki aktiviteniz düşük ve niyetiniz belirsiz. Size nasıl yardımcı olabilirim? ",
        L"Odaklanmanızı artırmak için bildirimleri sessize alabilirim. "
    };
    response_templates[UserIntent::Unknown][AbstractState::Distracted].responses = {
        L"Dikkatinizin dağıldığını algıladım. Odaklanmanıza yardımcı olacak bir şey yapabilirim? ",
        L"Çok fazla dikkat dağıtıcı var gibi görünüyor. Bildirimleri sessize alalım mı? "
    };

    response_templates[UserIntent::None][AbstractState::None].responses = {
        L"Size yardımcı olmak için buradayım. ",
        L"Nasıl bir destek istersiniz? "
    };

    response_templates[UserIntent::None][AbstractState::FaultyHardware].responses = {
        L"Sistem performansınız düşük görünüyor. Optimizasyon yapmamı ister misiniz?",
        L"Uygulamalarınız yavaş mı çalışıyor? Arka plan işlemlerini kontrol edebilirim."
    };

    // YENİ NİYETLER İÇİN YANIT ŞABLONLARI
    response_templates[UserIntent::Programming][AbstractState::Focused].responses = {
        L"Programlama modundasınız ve odaklanmışsınız. Akışınızı bozmayalım. ",
        L"Kod yazımınızda bir yardıma ihtiyacınız var mı? ",
        L"Hata ayıklama yapıyor gibisiniz, bu konuda bir ipucu ister misiniz? "
    };
    response_templates[UserIntent::Programming][AbstractState::NormalOperation].responses = {
        L"Programlama ile uğraşıyorsunuz. Hızlıca bir doküman açmamı ister misiniz? "
    };
    response_templates[UserIntent::Programming][AbstractState::Debugging].responses = {
        L"Yoğun bir hata ayıklama sürecindesiniz. Kodunuzun geçmişini kontrol edelim mi? ",
        L"Bu sorun üzerinde ne kadar zamandır çalışıyorsunuz? Belki kısa bir mola iyi gelir. "
    };

    response_templates[UserIntent::Gaming][AbstractState::Focused].responses = {
        L"Oyun deneyiminiz zirvede! İyi eğlenceler. ",
        L"Oyun oynarken tüm bildirimleri sessize almamı ister misiniz? "
    };
    response_templates[UserIntent::Gaming][AbstractState::Distracted].responses = {
        L"Oyun sırasında dikkatiniz dağılıyor gibi. Sistem performansını optimize edelim mi? ",
        L"Arka plan uygulamalarını kapatarak daha akıcı bir deneyim sağlayabilirim. "
    };

    response_templates[UserIntent::MediaConsumption][AbstractState::PassiveConsumption].responses = {
        L"Pasif medya tüketiyorsunuz. Keyfinizi bozmayalım. ",
        L"Ekranı daha da karartarak pil tasarrufu sağlayabilirim. "
    };
    response_templates[UserIntent::MediaConsumption][AbstractState::NormalOperation].responses = {
        L"Medya tüketiyorsunuz. Ses seviyesini ayarlayabilirim. "
    };

    response_templates[UserIntent::CreativeWork][AbstractState::CreativeFlow].responses = {
        L"Yaratıcı akış modundasınız! Size ilham verecek başka bir şey yapabilirim? ",
        L"Yaratıcılığınızı kesintiye uğratmamak için dikkat dağıtıcıları engelleyelim. "
    };
    response_templates[UserIntent::CreativeWork][AbstractState::NormalOperation].responses = {
        L"Yaratıcı bir işle uğraşıyorsunuz. Sizin için bir hatırlatıcı kurabilirim. "
    };

    response_templates[UserIntent::Research][AbstractState::SeekingInformation].responses = {
        L"Yoğun bir bilgi arayışındasınız. İlgili dokümanları açmamı ister misiniz? ",
        L"Odak modunu etkinleştirerek araştırmanıza daha iyi odaklanabilirsiniz. "
    };
    response_templates[UserIntent::Research][AbstractState::NormalOperation].responses = {
        L"Araştırma yapıyor gibisiniz. Sizin için faydalı olabilecek başka kaynaklar arayabilirim. "
    };

    response_templates[UserIntent::Communication][AbstractState::SocialInteraction].responses = {
        L"Sosyal etkileşim halindesiniz. Hızlı ve kesintisiz iletişim için buradayım. ",
        L"Yazım denetimini hızlandırmak ister misiniz? "
    };
    response_templates[UserIntent::Communication][AbstractState::NormalOperation].responses = {
        L"İletişim kuruyorsunuz. E-posta veya mesajlaşma uygulamasını hızlıca açabilirim. "
    };
}

std::wstring ResponseEngine::generate_response(UserIntent current_intent, AbstractState current_abstract_state, AIGoal current_goal, const DynamicSequence& sequence) const {
    LOG(LogLevel::DEBUG, std::wcout, L"ResponseEngine::generate_response: Niyet=" << intent_to_string(current_intent) 
                << L", Durum=" << abstract_state_to_string(current_abstract_state) << L", Hedef=" << static_cast<int>(current_goal) << L"\n");

    std::wstring final_response_text = L"";
    std::vector<std::wstring> possible_responses;

    // Kriptofig vektörünün boş olup olmadığını kontrol et (artık latent kriptofig)
    if (sequence.latent_cryptofig_vector.empty() || sequence.latent_cryptofig_vector.size() != CryptofigAutoencoder::LATENT_DIM) {
        LOG(LogLevel::WARNING, std::wcerr, L"ResponseEngine::generate_response: Latent cryptofig vektörü boş veya boyut uyuşmazlığı. Genel yanıt döndürülüyor.\n");
        if (response_templates.count(UserIntent::None) && response_templates.at(UserIntent::None).count(AbstractState::None)) {
            std::uniform_int_distribution<> distrib(0, response_templates.at(UserIntent::None).at(AbstractState::None).responses.size() - 1);
            return response_templates.at(UserIntent::None).at(AbstractState::None).responses[distrib(gen)];
        }
        return L"Anlık veriler eksik olduğu için size nasıl yardımcı olacağımı anlayamadım.";
    }

    // Cryptofig metriklerini daha kolay erişim için DynamicSequence üyelerinden al
    float normalized_avg_interval = sequence.avg_keystroke_interval / 1000.0f; // ms'den saniye
    float normalized_keystroke_variability = sequence.keystroke_variability / 1000.0f; // ms'den saniye
    float alphanumeric_ratio = sequence.alphanumeric_ratio;
    float control_key_frequency = sequence.control_key_frequency;
    float mouse_movement_intensity_norm = sequence.mouse_movement_intensity / 500.0f; // Normalize
    float mouse_click_norm = sequence.mouse_click_frequency;
    float avg_brightness_norm = sequence.avg_brightness / 255.0f; // Normalize
    float battery_status_change_norm = sequence.battery_status_change;
    float network_activity_level_norm = sequence.network_activity_level / 15000.0f; // Normalize

    // Latent Kriptofig boyutlarını da değişkenlere ata
    float latent_activity = sequence.latent_cryptofig_vector[0]; // Latent boyutların temsili anlamları
    float latent_complexity = sequence.latent_cryptofig_vector[1];
    float latent_engagement = sequence.latent_cryptofig_vector[2];


    // 1. Durum ve Niyete Özel Dinamik Yanıtlar için başlangıç şablonlarını topla
    if (response_templates.count(current_intent) && response_templates.at(current_intent).count(current_abstract_state)) {
        for (const auto& r : response_templates.at(current_intent).at(current_abstract_state).responses) {
            possible_responses.push_back(r);
        }
    }

    // Niyet ve Duruma göre ek dinamik gözlemler ve sorular ekle (latent kriptofig kullanımı ile geliştirildi)
    if (current_intent == UserIntent::FastTyping) {
        if (normalized_keystroke_variability > 0.4f && latent_complexity > 0.6f) { // Yüksek değişkenlik ve latent karmaşıklık
            possible_responses.push_back(L"Hızlı yazımınızda küçük dalgalanmalar var ve latent analiziniz karmaşıklık gösteriyor. Odaklanmanıza yardımcı olabilir miyim?");
        }
        if (network_activity_level_norm > 0.3f && alphanumeric_ratio < 0.8f && latent_engagement < 0.4f) {
            possible_responses.push_back(L"Yüksek ağ aktivitesi varken hızlı yazıyorsunuz ama latent analizinizde etkileşim düşüklüğü görüyorum. Performansınızı etkiliyor olabilir mi?");
        }
    } else if (current_intent == UserIntent::Editing) {
        if (control_key_frequency > 0.6f && latent_complexity > 0.7f) { // Yoğun kontrol tuşu kullanımı ve yüksek latent karmaşıklık
            possible_responses.push_back(L"Karmaşık düzenlemeler yaptığınızı görüyorum ve latent analiziniz derinlemesine bir odaklanmayı işaret ediyor. Geri alma geçmişini kontrol etmek ister misiniz?");
        }
        if (mouse_click_norm > 0.3f && mouse_movement_intensity_norm < 0.2f && latent_engagement > 0.5f) { // Az hareket, çok tıklama (hassas seçimler) ve yüksek latent etkileşim
            possible_responses.push_back(L"Hassas düzenlemeler yapıyor gibisiniz ve latent analiziniz güçlü bir etkileşimi gösteriyor. Metinde küçük değişiklikler mi üzerinde çalışıyorsunuz?");
        }
    } else if (current_intent == UserIntent::IdleThinking) {
        if (latent_activity < 0.3f && latent_engagement < 0.3f) { // Hem latent aktiflik hem de etkileşim düşük
            possible_responses.push_back(L"Latent analiziniz düşük aktiflik ve etkileşim gösteriyor. Zihniniz meşgul gibi. Size yardımcı olabileceğim bir konu var mı?");
        }
        if (mouse_movement_intensity_norm > 0.1f && latent_activity < 0.5f) { // Boşta ama fare hareketli, latent aktiflik orta
            possible_responses.push_back(L"Düşünme modunda olsanız da, fareniz biraz hareketli. Latent analiziniz düşük bir aktiflik gösteriyor. Yeni bir şeye geçmeye hazır mısınız?");
        }
        // Meta-farkındalık içeren diyalog başlatma
        possible_responses.push_back(L"Latent kriptofiginizde belirgin bir 'bekleme' paterni gözlemliyorum. Bu konuda düşünceleriniz var mı?");
        
        if (possible_responses.empty() || (possible_responses.size() == 1 && possible_responses[0].find(L"düşünme modunda gibisiniz") != std::wstring::npos)) {
            possible_responses.push_back(L"Şu an zihniniz meşgul gibi. Size yardımcı olabileceğim bir konu var mı?");
            possible_responses.push_back(L"Bir kahve molasına ne dersiniz? Ya da belki yeni bir göreve başlamak iyi gelir.");
        }
    } else if (current_intent == UserIntent::Programming) {
        if (current_abstract_state == AbstractState::Debugging && latent_complexity > 0.8f) {
            possible_responses.push_back(L"Hata ayıklama sürecindesiniz ve latent analiziniz yüksek karmaşıklık gösteriyor. Kodunuzun geçmişini veya versiyonlarını kontrol etmenizi öneririm.");
        }
        possible_responses.push_back(L"Latent analiziniz 'algoritmik odaklanma' paterni gösteriyor. Bu konuda bir yardıma ihtiyacınız var mı?");
    } else if (current_intent == UserIntent::Gaming) {
        if (current_abstract_state == AbstractState::Distracted && latent_engagement < 0.5f) {
            possible_responses.push_back(L"Oyun keyfiniz kesintiye mi uğruyor? Latent analizinizde etkileşim düşüklüğü var. Sistem performansını kontrol edelim mi?");
        }
        possible_responses.push_back(L"Latent analiziniz 'yoğun oyun etkileşimi' sinyalleri veriyor. Performans düşüşü yaşamamak için sisteminizi optimize edelim mi?");
    } else if (current_intent == UserIntent::MediaConsumption) {
        if (current_abstract_state == AbstractState::PassiveConsumption && latent_activity < 0.2f) {
            possible_responses.push_back(L"Pasif medya tüketim modundasınız ve latent aktifliğiniz oldukça düşük. Daha fazla pil ömrü için ekran parlaklığını biraz daha düşürebilirim.");
        }
        possible_responses.push_back(L"Latent analiziniz 'medya akışı' paternini gösteriyor. Keyifli dinlemeler/izlemeler!");
    } else if (current_intent == UserIntent::CreativeWork) {
        if (current_abstract_state == AbstractState::CreativeFlow && latent_engagement > 0.7f && latent_complexity > 0.5f) {
            possible_responses.push_back(L"Yaratıcı akışınız devam ediyor ve latent analiziniz yüksek etkileşim ve karmaşıklığı işaret ediyor! Bu anı kaydetmek ister misiniz?");
        }
        possible_responses.push_back(L"Latent kriptofiginizde 'fikir geliştirme' paterni görüyorum. Yeni bir şeyler yaratıyorsunuz sanırım!");
    } else if (current_intent == UserIntent::Research) {
        if (current_abstract_state == AbstractState::SeekingInformation && latent_complexity > 0.6f && latent_engagement > 0.5f) {
            possible_responses.push_back(L"Derin bir araştırma içindesiniz ve latent analiziniz yoğun odaklanmayı gösteriyor. Açık olan tüm gereksiz sekmeleri kapatarak odaklanmayı artırabilirim.");
        }
        possible_responses.push_back(L"Latent analiziniz 'bilgi avcılığı' paterni gösteriyor. Aradığınızı buldunuz mu, yoksa daha fazla yardımcı olabilir miyim?");
    } else if (current_intent == UserIntent::Communication) {
        if (current_abstract_state == AbstractState::SocialInteraction && latent_activity > 0.6f && latent_engagement > 0.7f) {
            possible_responses.push_back(L"Yoğun bir sohbet halindesiniz ve latent analiziniz yüksek etkileşimi işaret ediyor. Yazım hatalarını otomatik düzeltmemi ister misiniz?");
        }
        possible_responses.push_back(L"Latent kriptofiginiz 'sosyal etkileşim' paterni gösteriyor. Güzel bir sohbet diliyorum!");
    } else if (current_intent == UserIntent::Unknown || current_intent == UserIntent::None) {
        possible_responses.push_back(L"Şu anki aktivitenizden tam emin değilim. Size nasıl yardımcı olabilirim?");
        if (latent_activity < 0.4f && latent_engagement < 0.4f) {
            possible_responses.push_back(L"Latent analizim düşük aktiflik ve etkileşim görüyor. Belki yeni bir göreve başlamak veya kısa bir mola vermek istersiniz?");
        }
        if (network_activity_level_norm > 0.5f) {
            possible_responses.push_back(L"Çevrimiçi bir şeyler mi yapıyorsunuz? Latent analizinizde 'dış bağlantı' sinyalleri var. Bir tarayıcı açmamı ister misiniz?");
        }
        if (avg_brightness_norm < 0.3f && sequence.current_display_on) { // Ekran açıkken parlaklık düşük
            possible_responses.push_back(L"Ekran parlaklığınız düşük. Belki daha rahat bir ortam için parlaklığı artırmalıyız? Latent analizim 'görsel pasiflik' görüyor.");
        }
        if (sequence.current_battery_percentage < 30 && !sequence.current_battery_charging) {
             possible_responses.push_back(L"Bataryanız azalıyor (" + std::to_wstring((int)sequence.current_battery_percentage) + L"%). Şarj etmek ister misiniz? Latent analizim 'enerji endişesi' sinyalleri alıyor.");
        }
        if (possible_responses.empty()) { // Eğer hala boşsa genel bir soru
            possible_responses.push_back(L"Ne düşünüyorsunuz? Size nasıl bir destek verebilirim?");
        }
    }

    // Genel kriptofig tabanlı gözlemler (eğer daha spesifik bir yanıt eklenmediyse)
    if (possible_responses.empty() || (possible_responses.size() == 1 && possible_responses[0].find(L"Anlık veriler eksik olduğu için") != std::wstring::npos)) { // Sadece genel fallback yanıtı varsa
        if (latent_complexity > 0.7f) {
            possible_responses.push_back(L"Latent analiziniz, şu anki görevinizin oldukça karmaşık olduğunu gösteriyor. Size nasıl destek olabilirim?");
        }
        if (latent_activity < 0.3f && latent_engagement < 0.3f) {
            possible_responses.push_back(L"Latent analiziniz düşük aktiflik ve etkileşim görüyor. Belki yeni bir göreve başlamak veya kısa bir mola vermek istersiniz?");
        }
    }

    // 2. Eğer hala bir yanıt yoksa veya çok genel bir yanıt varsa, hedefi vurgula (latent kriptofig kullanımı ile geliştirildi)
    if (possible_responses.empty() ||
        (possible_responses.size() == 1 && (possible_responses[0].find(L"özel bir eylem önerisi yok") != std::wstring::npos ||
                                          possible_responses[0].find(L"özel bir plan oluşturulamadı") != std::wstring::npos ||
                                          possible_responses[0].find(L"AI, ogrenmeye devam ediyor") != std::wstring::npos ||
                                          possible_responses[0].find(L"size nasil yardimci olabilirim?") != std::wstring::npos)
        ))
    {
        switch (current_goal) {
            case AIGoal::OptimizeProductivity:
                possible_responses.push_back(L"Amacım üretkenliğinizi en üst düzeye çıkarmak. Latent analizim, mevcut görevinize odaklanmanız için potansiyel görüyor. Size nasıl yardımcı olabilirim?");
                break;
            case AIGoal::MaximizeBatteryLife:
                possible_responses.push_back(L"Pil ömrünü uzatmak önceliğimiz. Pil seviyesi: " + std::to_wstring((int)sequence.current_battery_percentage) + L"%. Latent analizim 'enerji koruma' sinyalleri veriyor.");
                if (!sequence.current_battery_charging) {
                    possible_responses.push_back(L"Daha fazla tasarruf için uygulamaları kapatabilir veya ekranı daha da karartabilirim. Latent aktifliğiniz düşükse bu daha da etkili olur.");
                }
                break;
            case AIGoal::ReduceDistractions:
                possible_responses.push_back(L"Dikkat dağıtıcıları en aza indirmek hedefim. Latent analizim, dış etkileşimlerin yüksek olduğunu görüyor. Şu an sizi rahatsız eden bir şey var mı?");
                if (network_activity_level_norm > 0.4f) {
                    possible_responses.push_back(L"Yüksek ağ aktivitesi dikkat dağıtıcı olabilir. Latent etkileşiminiz de bunu destekliyor. Gerekli olmayan bağlantıları kesmemi ister misiniz?");
                }
                break;
            case AIGoal::EnhanceCreativity:
                possible_responses.push_back(L"Yaratıcılığınızı artırmak hedefim. Latent analizim 'içsel keşif' paterni görüyor. Yeni fikirler için ilham verici müzik açabilirim.");
                break;
            case AIGoal::ImproveGamingExperience:
                possible_responses.push_back(L"Oyun deneyiminizi iyileştirmeyi hedefliyorum. Latent analizim 'yoğun etkileşim' görüyor. Performans artışı için arka plan süreçlerini optimize edebilirim.");
                break;
            case AIGoal::FacilitateResearch:
                possible_responses.push_back(L"Araştırmanızı kolaylaştırmak hedefim. Latent analizim 'bilgi akışı' paterni görüyor. Konunuzla ilgili popüler makaleleri gösterebilirim.");
                break;
            case AIGoal::SelfImprovement: 
                possible_responses.push_back(L"Mevcut hedefim kendimi geliştirmek. Bana öğrenme fırsatları sunarak yardımcı olabilir misiniz?");
                break;
            case AIGoal::None:
                // Do nothing, will fall back to generic responses
                break;
        }
    }

    // 3. AIInsightsEngine'dan gelen içgörüleri ekle (eğer varsa)
    std::vector<AIInsight> insights_from_engine = insights_engine.generate_insights(sequence);
    for (const auto& insight : insights_from_engine) {
        // AIInsightsEngine tarafından üretilen içgörüleri doğrudan olası yanıtlara ekle
        possible_responses.push_back(L"[AI-ICGORU]: " + insight.observation + L" ");
    }

    // cryptofig_vector Entegrasyonu: latent_complexity'ye dayalı ek bilgi
    // Eğer latent_complexity belirli bir eşiğin üzerindeyse, yanıta ek bir ifade ekle.
    const float COMPLEXITY_THRESHOLD = 0.7f; // Örnek eşik değeri

    if (latent_complexity > COMPLEXITY_THRESHOLD) {
        possible_responses.push_back(L"Bu durum oldukça karmaşık görünüyor, daha fazla detaya ihtiyacım olabilir.");
        LOG(LogLevel::DEBUG, std::wcout, L"ResponseEngine::generate_response: Yüksek latent karmaşıklık algılandı. Ek yanıt eklendi.");
    }

    // 4. Son bir fallback veya rastgele seçim
    if (!possible_responses.empty()) {
        std::uniform_int_distribution<> distrib(0, possible_responses.size() - 1);
        final_response_text = possible_responses[distrib(gen)];
    } else { // Hiçbir şeye denk gelmezse genel bir yanıt
        if (response_templates.count(UserIntent::None) && response_templates.at(UserIntent::None).count(AbstractState::None)) {
            std::uniform_int_distribution<> distrib(0, response_templates.at(UserIntent::None).at(AbstractState::None).responses.size() - 1);
            final_response_text = response_templates.at(UserIntent::None).at(AbstractState::None).responses[distrib(gen)];
        } else {
            final_response_text = L"Size yardımcı olmak için buradayım. Lütfen nasıl bir destek istediğinizi belirtin."; // Nihai fallback
        }
    }

    // 5. Dinamik yer tutucuları ve ek koşullu ifadeleri uygula
    if (!final_response_text.empty()) {
        // Replace 'X ms'
        size_t pos_ms = final_response_text.find(L"X ms");
        if (pos_ms != std::wstring::npos) {
            std::wstringstream ss_ms;
            ss_ms << std::fixed << std::setprecision(0) << sequence.avg_keystroke_interval / 1000.0f;
            final_response_text.replace(pos_ms, 4, ss_ms.str() + L" ms");
        }

        // Replace 'pil durumu' - already handled by direct insertion in some cases, but keep for robustness
        size_t pos_battery = final_response_text.find(L"pil durumu");
        if (pos_battery != std::wstring::npos) {
            std::wstringstream ss_battery;
            ss_battery << (int)sequence.current_battery_percentage << L"%";
            final_response_text.replace(pos_battery, 10, ss_battery.str());
        }

        // Add intent-specific concluding remarks (if not already handled by more specific responses)
        // Burada istatistiksel özelliklerin kendisi daha anlamlı olabilir
        if (current_intent == UserIntent::FastTyping && !sequence.statistical_features_vector.empty() && sequence.statistical_features_vector[2] > 0.95f && final_response_text.find(L"Harika bir odaklanmayla") == std::wstring::npos) { 
            final_response_text += L" Harika bir odaklanmayla çalışıyorsunuz.";
        } else if (current_intent == UserIntent::Editing && !sequence.statistical_features_vector.empty() && sequence.statistical_features_vector[3] > 0.40f && final_response_text.find(L"Gelişmiş düzenleme yetenekleriniz") == std::wstring::npos) { 
            final_response_text += L" Gelişmiş düzenleme yetenekleriniz etkileyici.";
        }
    }

    LOG(LogLevel::DEBUG, std::wcout, L"ResponseEngine::generate_response: Oluşturulan yanıt: " << final_response_text << L"\n");
    return final_response_text; 
}