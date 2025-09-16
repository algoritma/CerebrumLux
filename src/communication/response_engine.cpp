#include "response_engine.h" // Kendi başlık dosyasını dahil et
#include "../core/logger.h"      // LOG makrosu için
#include "../core/utils.h"       // intent_to_string, abstract_state_to_string
#include "../data_models/dynamic_sequence.h" // DynamicSequence için
#include "../brain/intent_analyzer.h" // IntentAnalyzer için
#include "../planning_execution/goal_manager.h" // GoalManager için
#include "ai_insights_engine.h" // AIInsightsEngine için
#include "../brain/autoencoder.h" // CryptofigAutoencoder::LATENT_DIM için
#include <iomanip>   // std::fixed, std::setprecision için
#include <algorithm> // std::min/max için
#include <iostream>  // std::cout, std::cerr için
#include <sstream>   // std::stringstream için

// Yardımcı fonksiyon: Bir yanıtın kritik bir eylem önerisi olup olmadığını kontrol eder.
// Şimdilik basit bir anahtar kelime kontrolü kullanıyoruz.
static bool is_critical_action_suggestion(const std::string& suggestion) {
    if (suggestion.find("optimize etmemi ister misiniz?") != std::string::npos || // Sistem performansını optimize et
        suggestion.find("Uygulamaları kapatmamı ister misiniz?") != std::string::npos || // Uygulamaları kapat
        suggestion.find("Gerekli olmayan bağlantıları kesmemi ister misiniz?") != std::string::npos || // Bağlantıları kes
        suggestion.find("arka plan süreçlerini optimize edebilirim.") != std::string::npos || // Arka plan süreçlerini optimize et
        suggestion.find("gereksiz sekmeleri kapatarak") != std::string::npos || // Gereksiz sekmeleri kapat
        suggestion.find("Arka plan uygulamalarını kapatarak") != std::string::npos || // Arka plan uygulamalarını kapat
        suggestion.find("Sistem performansını optimize edelim mi?") != std::string::npos) { // Sistem performansını optimize et (farklı ifade)
        return true;
    }
    return false;
}

// Yardımcı fonksiyon: Kritik eylem önerisi mesajından eylem açıklamasını çıkarır.
static std::string extract_action_description(const std::string& full_suggestion) {
    std::string description = full_suggestion;

    // Önekleri ve baştaki boşlukları temizle
    size_t start_pos = description.find_first_not_of(" \t\n\r");
    if (std::string::npos != start_pos) {
        description = description.substr(start_pos);
    }

    const std::string onerisi_prefix = "AI Onerisi: ";
    if (description.rfind(onerisi_prefix, 0) == 0) {
        description.erase(0, onerisi_prefix.length());
    }

    const std::string icgoru_prefix = "[AI-ICGORU]: ";
    if (description.rfind(icgoru_prefix, 0) == 0) {
        description.erase(0, icgoru_prefix.length());
    }

    // Yaygın soru kalıplarını ve fazlalıkları temizle
    size_t pos;
    if ((pos = description.find(" ister misiniz?")) != std::string::npos) description.erase(pos);
    if ((pos = description.find(" edebilirim.")) != std::string::npos) description.erase(pos);
    if ((pos = description.find(" sağlayabilirim.")) != std::string::npos) description.erase(pos);
    if ((pos = description.find(" kapatabilirim.")) != std::string::npos) description.erase(pos);
    if ((pos = description.find(" artırabilirim.")) != std::string::npos) description.erase(pos);
    if ((pos = description.find(" optimize edebilirim?")) != std::string::npos) description.erase(pos);
    if ((pos = description.find(" kesmemi ister misiniz?")) != std::string::npos) description.erase(pos);
    if ((pos = description.find(" edelim mi?")) != std::string::npos) description.erase(pos);
    if ((pos = description.find(" öneririm.")) != std::string::npos) description.erase(pos); // Yeni ekledik
    if ((pos = description.find(" olabilirim?")) != std::string::npos) description.erase(pos); // Yeni ekledik
    
    // Cümle sonundaki noktalama işaretlerini ve boşlukları temizle
    while (!description.empty() && (description.back() == '.' || description.back() == '?' || description.back() == ' ' || description.back() == '\n')) {
        description.pop_back();
    }
    return description;
}

// === ResponseEngine Implementasyonlari ===
ResponseEngine::ResponseEngine(IntentAnalyzer& analyzer_ref, GoalManager& goal_manager_ref, AIInsightsEngine& insights_engine_ref) 
    : analyzer(analyzer_ref), goal_manager(goal_manager_ref), insights_engine(insights_engine_ref), gen(rd()) { 
    
    // Mevcut yanıt şablonları
    response_templates[UserIntent::FastTyping][AbstractState::HighProductivity].responses = {
        "Harika bir hizla yaziyorsunuz! Odaginizi koruyun. ",
        "Uretkenliginiz zirvede, devam edin! ",
        "Yazim akiciliginiz etkileyici. Su anki akisini bozmayalim. "
    };
    response_templates[UserIntent::FastTyping][AbstractState::NormalOperation].responses = {
        "Hizli yazim modunda gibisiniz. Daha verimli olmak için önerilerim olabilir mi? ",
        "Hizli yaziminizda dikkat dagiticilari azaltmak ister misiniz? "
    };

    response_templates[UserIntent::Editing][AbstractState::Focused].responses = {
        "Duzenleme modundasiniz ve odaklanmissiniz. Herhangi bir yardima ihtiyaciniz var mi? ",
        "Metninizi titizlikle düzenliyorsunuz, takdire şayan. "
    };
    response_templates[UserIntent::Editing][AbstractState::NormalOperation].responses = {
        "Geri alma gecmisinizi gostermemi ister misiniz? ",
        "Duzenleme isleminizi kolaylastiracak bir eylem önerebilirim. "
    };

    response_templates[UserIntent::IdleThinking][AbstractState::LowProductivity].responses = {
        "Biraz düşünme modunda gibisiniz. Yeni bir göreve mi geçmek istersiniz? ",
        "Görüyorum ki şu an aktif bir etkileşim yok. Belki de bir ara vermeye ihtiyacınız var? ",
        "Herkesin düşünmeye ihtiyacı vardır. Hazır olduğunuzda bana söyleyin. "
    };
    response_templates[UserIntent::IdleThinking][AbstractState::NormalOperation].responses = {
        "Boşta olduğunuzu algıladım. Belki bir hatırlatıcı kurmak ister misiniz? ",
        "Uzun süredir aktif değilsiniz. Ekranı karartmak veya bildirimleri sessize almak ister misiniz?"
    };

    response_templates[UserIntent::Unknown][AbstractState::None].responses = {
        "Mevcut niyetiniz belirsiz. Daha fazla veri toplanarak öğrenmeye devam ediyorum. ",
        "Davranışınızı anlamaya çalışıyorum. Lütfen etkileşiminize devam edin. "
    };
    response_templates[UserIntent::Unknown][AbstractState::LowProductivity].responses = {
        "Şu anki aktiviteniz düşük ve niyetiniz belirsiz. Size nasıl yardımcı olabilirim? ",
        "Odaklanmanızı artırmak için bildirimleri sessize alabilirim. "
    };
    response_templates[UserIntent::Unknown][AbstractState::Distracted].responses = {
        "Dikkatinizin dağıldığını algıladım. Odaklanmanıza yardımcı olacak bir şey yapabilirim? ",
        "Çok fazla dikkat dağıtıcı var gibi görünüyor. Bildirimleri sessize alalım mı? "
    };

    response_templates[UserIntent::None][AbstractState::None].responses = {
        "Size yardımcı olmak için buradayım. ",
        "Nasıl bir destek istersiniz? "
    };

    response_templates[UserIntent::None][AbstractState::FaultyHardware].responses = {
        "Sistem performansınız düşük görünüyor. Optimizasyon yapmamı ister misiniz?",
        "Uygulamalarınız yavaş mı çalışıyor? Arka plan işlemlerini kontrol edebilirim."
    };

    // YENİ NİYETLER İÇİN YANIT ŞABLONLARI
    response_templates[UserIntent::Programming][AbstractState::Focused].responses = {
        "Programlama modundasınız ve odaklanmışsınız. Akışınızı bozmayalım. ",
        "Kod yazımınızda bir yardıma ihtiyacınız var mı? ",
        "Hata ayıklama yapıyor gibisiniz, bu konuda bir ipucu ister misiniz? "
    };
    response_templates[UserIntent::Programming][AbstractState::NormalOperation].responses = {
        "Programlama ile uğraşıyorsunuz. Hızlıca bir doküman açmamı ister misiniz? "
    };
    response_templates[UserIntent::Programming][AbstractState::Debugging].responses = {
        "Yoğun bir hata ayıklama sürecindesiniz. Kodunuzun geçmişini kontrol edelim mi? ",
        "Bu sorun üzerinde ne kadar zamandır çalışıyorsunuz? Belki kısa bir mola iyi gelir."
    };

    response_templates[UserIntent::Gaming][AbstractState::Focused].responses = {
        "Oyun deneyiminiz zirvede! İyi eğlenceler. ",
        "Oyun oynarken tüm bildirimleri sessize almamı ister misiniz? "
    };
    response_templates[UserIntent::Gaming][AbstractState::Distracted].responses = {
        "Oyun sırasında dikkatiniz dağılıyor gibi. Sistem performansını optimize edelim mi? ",
        "Arka plan uygulamalarını kapatarak daha akıcı bir deneyim sağlayabilirim. "
    };

    response_templates[UserIntent::MediaConsumption][AbstractState::PassiveConsumption].responses = {
        "Pasif medya tüketiyorsunuz. Keyfinizi bozmayalım. ",
        "Ekranı daha da karartarak pil tasarrufu sağlayabilirim. "
    };
    response_templates[UserIntent::MediaConsumption][AbstractState::NormalOperation].responses = {
        "Medya tüketiyorsunuz. Ses seviyesini ayarlayabilirim. "
    };

    response_templates[UserIntent::CreativeWork][AbstractState::CreativeFlow].responses = {
        "Yaratıcı akış modundasınız! Size ilham verecek başka bir şey yapabilirim? ",
        "Yaratıcılığınızı kesintiye uğratmamak için dikkat dağıtıcıları engelleyelim. "
    };
    response_templates[UserIntent::CreativeWork][AbstractState::NormalOperation].responses = {
        "Yaratıcı bir işle uğraşıyorsunuz. Sizin için bir hatırlatıcı kurabilirim. "
    };

    response_templates[UserIntent::Research][AbstractState::SeekingInformation].responses = {
        "Yoğun bir bilgi arayışındasınız. İlgili dokümanları açmamı ister misiniz? ",
        "Odak modunu etkinleştirerek araştırmanıza daha iyi odaklanabilirsiniz. "
    };
    response_templates[UserIntent::Research][AbstractState::NormalOperation].responses = {
        "Araştırma yapıyor gibisiniz. Sizin için faydalı olabilecek başka kaynaklar arayabilirim. "
    };

    response_templates[UserIntent::Communication][AbstractState::SocialInteraction].responses = {
        "Sosyal etkileşim halindesiniz. Hızlı ve kesintisiz iletişim için buradayım. ",
        "Yazım denetimini hızlandırmak ister misiniz? "
    };
    response_templates[UserIntent::Communication][AbstractState::NormalOperation].responses = {
        "İletişim kuruyorsunuz. E-posta veya mesajlaşma uygulamasını hızlıca açabilirim. "
    };
}

std::string ResponseEngine::generate_response(UserIntent current_intent, AbstractState current_abstract_state, AIGoal current_goal, const DynamicSequence& sequence) const {
    LOG_DEFAULT(LogLevel::DEBUG, "ResponseEngine::generate_response: Niyet=" << intent_to_string(current_intent) << 
                ", Durum=" << abstract_state_to_string(current_abstract_state) << ", Hedef=" << static_cast<int>(current_goal) << "\n");

    std::string final_response_text = "";

    // Kriptofig vektörünün boş olup olmadığını kontrol et (artık latent kriptofig)
    if (sequence.latent_cryptofig_vector.empty() || sequence.latent_cryptofig_vector.size() != CryptofigAutoencoder::LATENT_DIM) {
        LOG_DEFAULT(LogLevel::WARNING, "ResponseEngine::generate_response: Latent cryptofig vektörü boş veya boyut uyuşmuyor. Genel yanıt döndürülüyor.\n");
        if (response_templates.count(UserIntent::None) && response_templates.at(UserIntent::None).count(AbstractState::None)) {
            std::uniform_int_distribution<> distrib(0, response_templates.at(UserIntent::None).at(AbstractState::None).responses.size() - 1);
            return response_templates.at(UserIntent::None).at(AbstractState::None).responses[distrib(gen)];
        }
        return "Anlık veriler eksik olduğu için size nasıl yardımcı olacağımı anlayamadım.";
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

    // --- Deterministik Seçim Mantığı Başlangıcı ---

    // Test Durumu 1: Selamlama Niyeti - Rastgele Yanıt Seçimi
    // UserIntent::None ve AbstractState::None durumunda genel selamlama yanıtlarını önceliklendir.
    if (current_intent == UserIntent::None && current_abstract_state == AbstractState::None) {
        if (response_templates.count(UserIntent::None) && response_templates.at(UserIntent::None).count(AbstractState::None)) {
            std::uniform_int_distribution<> distrib(0, response_templates.at(UserIntent::None).at(AbstractState::None).responses.size() - 1);
            final_response_text = response_templates.at(UserIntent::None).at(AbstractState::None).responses[distrib(gen)];
            LOG_DEFAULT(LogLevel::DEBUG, "ResponseEngine::generate_response: Test Durumu 1 tetiklendi - Genel selamlama.\n");
            return final_response_text;
        }
    }

    // Test Durumu 2: Düşük Üretkenlik Durumu ve Düşük Batarya (Test Durumu 3'ten önce önceliklendirildi)
    if (current_abstract_state == AbstractState::LowProductivity && sequence.current_battery_percentage < 30 && !sequence.current_battery_charging) {
        std::stringstream ss;
        ss << "Bataryanız azalıyor (" << (int)sequence.current_battery_percentage << "%). Düşük üretkenlik durumunda olduğunuz için şarj etmenizi öneririm.";
        final_response_text = ss.str();
        LOG_DEFAULT(LogLevel::DEBUG, "ResponseEngine::generate_response: Test Durumu 2 tetiklendi - Düşük üretkenlik ve batarya.\n");
        return final_response_text;
    }

    // Test Durumu 3: Yüksek Latent Karmaşıklık
    const float COMPLEXITY_THRESHOLD = 0.7f;
    if (latent_complexity > COMPLEXITY_THRESHOLD) {
        final_response_text = "Bu durum oldukça karmaşık görünüyor, daha fazla detaya ihtiyacım olabilir.";
        LOG_DEFAULT(LogLevel::DEBUG, "ResponseEngine::generate_response: Test Durumu 3 tetiklendi - Yüksek latent karmaşıklık.\n");
        return final_response_text;
    }

    // Test Durumu 5: Düşük Latent Aktiflik/Etkileşim (UserIntent::Unknown veya UserIntent::None durumunda)
    // NOTE: Test Durumu 1'in UserIntent::None, AbstractState::None durumu zaten yukarıda ele alındı.
    // Bu koşul, UserIntent::Unknown veya UserIntent::None (ancak AbstractState::None olmayan) durumları için geçerli olacaktır.
    if ((current_intent == UserIntent::Unknown || current_intent == UserIntent::None) && latent_activity < 0.4f && latent_engagement < 0.4f) {
        final_response_text = "Latent analizim düşük aktiflik ve etkileşim görüyor. Belki yeni bir göreve başlamak veya kısa bir mola vermek istersiniz?";
        LOG_DEFAULT(LogLevel::DEBUG, "ResponseEngine::generate_response: Test Durumu 5 tetiklendi - Düşük latent aktiflik/etkileşim.\n");
        return final_response_text;
    }

    // --- Deterministik Seçim Mantığı Sonu ---

    // Eğer yukarıdaki spesifik koşullardan hiçbiri tetiklenmezse, mevcut genel mantığı kullanmaya devam et
    std::vector<std::string> possible_responses;

    // 1. Durum ve Niyete Özel Dinamik Yanıtlar için başlangıç şablonlarını topla
    if (response_templates.count(current_intent) && response_templates.at(current_intent).count(current_abstract_state)) {
        for (const auto& r : response_templates.at(current_intent).at(current_abstract_state).responses) {
            possible_responses.push_back(r);
        }
    }

    // Yeni: Durumsal Yanıtlar için latent kriptofig değerlerini kullan
    if (current_abstract_state == AbstractState::LowProductivity) {
        // NOTE: Test Durumu 2 zaten yukarıda ele alındı ve hemen döndü.
        if (latent_activity < 0.4f && latent_engagement < 0.4f) {
            possible_responses.push_back("Düşük üretkenlik durumundasınız ve latent analiziniz düşük aktiflik ve etkileşim görüyor. Belki yeni bir göreve başlamak veya kısa bir mola vermek istersiniz?");
        } else if (latent_activity < 0.6f) {
            possible_responses.push_back("Üretkenliğiniz düşük görünüyor. Latent aktifliğiniz de ortalamanın altında. Size yardımcı olabileceğim bir şey var mı?");
        }
    } else if (current_abstract_state == AbstractState::PowerSaving) {
        if (latent_activity < 0.3f && latent_engagement < 0.3f) {
            possible_responses.push_back("Güç tasarrufu modundasınız ve latent analiziniz çok düşük aktiflik ve etkileşim gösteriyor. Sistem kaynaklarını daha da optimize edebilirim.");
        } else if (sequence.current_battery_percentage < 20 && !sequence.current_battery_charging) {
            std::stringstream ss;
            ss << "Piliniz kritik seviyede (" << (int)sequence.current_battery_percentage << "%). Güç tasarrufu modunda olsanız da, şarj etmenizi öneririm.";
            possible_responses.push_back(ss.str());
        }
    }

    // Niyet ve Duruma göre ek dinamik gözlemler ve sorular ekle (latent kriptofig kullanımı ile geliştirildi)
    if (current_intent == UserIntent::FastTyping) {
        if (normalized_keystroke_variability > 0.4f && latent_complexity > 0.6f) { // Yüksek değişkenlik ve latent karmaşıklık
            possible_responses.push_back("Hızlı yazımınızda küçük dalgalanmalar var ve latent analiziniz karmaşıklık gösteriyor. Odaklanmanıza yardımcı olabilir miyim?");
        }
        if (network_activity_level_norm > 0.3f && alphanumeric_ratio < 0.8f && latent_engagement < 0.4f) {
            possible_responses.push_back("Yüksek ağ aktivitesi varken hızlı yazıyorsunuz ama latent analizinizde etkileşim düşüklüğü görüyorum. Performansınızı etkiliyor olabilir mi?");
        }
    } else if (current_intent == UserIntent::Editing) {
        if (control_key_frequency > 0.6f && latent_complexity > 0.7f) { // Yoğun kontrol tuşu kullanımı ve yüksek latent karmaşıklık
            possible_responses.push_back("Karmaşık düzenlemeler yaptığınızı görüyorum ve latent analiziniz derinlemesine bir odaklanmayı işaret ediyor. Geri alma geçmişini kontrol etmek ister misiniz?");
        }
        if (mouse_click_norm > 0.3f && mouse_movement_intensity_norm < 0.2f && latent_engagement > 0.5f) { // Az hareket, çok tıklama (hassas seçimler) ve yüksek latent etkileşim
            possible_responses.push_back("Hassas düzenlemeler yapıyor gibisiniz ve latent analiziniz güçlü bir etkileşimi gösteriyor. Metinde küçük değişiklikler mi üzerinde çalışıyorsunuz?");
        }
    } else if (current_intent == UserIntent::IdleThinking) {
        if (latent_activity < 0.3f && latent_engagement < 0.3f) { // Hem latent aktiflik hem de etkileşim düşük
            possible_responses.push_back("Latent analiziniz düşük aktiflik ve etkileşim gösteriyor. Zihniniz meşgul gibi. Size yardımcı olabileceğim bir konu var mı?");
        }
        if (mouse_movement_intensity_norm > 0.1f && latent_activity < 0.5f) { // Boşta ama fare hareketli, latent aktiflik orta
            possible_responses.push_back("Düşünme modunda olsanız da, fareniz biraz hareketli. Latent analiziniz düşük bir aktiflik gösteriyor. Yeni bir şeye geçmeye hazır mısınız?");
        }
        // Meta-farkındalık içeren diyalog başlatma
        possible_responses.push_back("Latent kriptofiginizde belirgin bir 'bekleme' paterni gözlemliyorum. Bu konuda düşünceleriniz var mı?");
        
        if (possible_responses.empty() || (possible_responses.size() == 1 && possible_responses[0].find("düşünme modunda gibisiniz") != std::string::npos)) {
            possible_responses.push_back("Şu an zihniniz meşgul gibi. Size yardımcı olabileceğim bir konu var mı?");
            possible_responses.push_back("Bir kahve molasına ne dersiniz? Ya da belki yeni bir göreve başlamak iyi gelir.");
        }
    } else if (current_intent == UserIntent::Programming) {
        if (current_abstract_state == AbstractState::Debugging && latent_complexity > 0.8f) {
            possible_responses.push_back("Hata ayıklama sürecindesiniz ve latent analiziniz yüksek karmaşıklık gösteriyor. Kodunuzun geçmişini veya versiyonlarını kontrol etmenizi öneririm.");
        }
        possible_responses.push_back("Latent analiziniz 'algoritmik odaklanma' paterni gösteriyor. Bu konuda bir yardıma ihtiyacınız var mı?");
    } else if (current_intent == UserIntent::Gaming) {
        if (current_abstract_state == AbstractState::Distracted && latent_engagement < 0.5f) {
            possible_responses.push_back("Oyun keyfiniz kesintiye mi uğruyor? Latent analizinizde etkileşim düşüklüğü var. Sistem performansını kontrol edelim mi?");
        }
        possible_responses.push_back("Latent analiziniz 'yoğun oyun etkileşimi' sinyalleri veriyor. Performans düşüşü yaşamamak için sisteminizi optimize edebilirim?");
    } else if (current_intent == UserIntent::MediaConsumption) {
        if (current_abstract_state == AbstractState::PassiveConsumption && latent_activity < 0.2f) {
            possible_responses.push_back("Pasif medya tüketim modundasınız ve latent aktifliğiniz oldukça düşük. Daha fazla pil ömrü için ekran parlaklığını biraz daha düşürebilirim.");
        }
        possible_responses.push_back("Latent analiziniz 'medya akışı' paternini gösteriyor. Keyifli dinlemeler/izlemeler!");
    }
    else if (current_intent == UserIntent::CreativeWork) {
        if (current_abstract_state == AbstractState::CreativeFlow && latent_engagement > 0.7f && latent_complexity > 0.5f) {
            possible_responses.push_back("Yaratıcı akışınız devam ediyor ve latent analiziniz yüksek etkileşim ve karmaşıklığı işaret ediyor! Bu anı kaydetmek ister misiniz?");
        }
        possible_responses.push_back("Latent kriptofiginizde 'fikir geliştirme' paterni görüyorum. Yeni bir şeyler yaratıyorsunuz sanırım!");
    } else if (current_intent == UserIntent::Research) {
        if (current_abstract_state == AbstractState::SeekingInformation && latent_complexity > 0.6f && latent_engagement > 0.5f) {
            possible_responses.push_back("Derin bir araştırma içindesiniz ve latent analiziniz yoğun odaklanmayı gösteriyor. Açık olan tüm gereksiz sekmeleri kapatarak odaklanmayı artırabilirim.");
        }
        possible_responses.push_back("Latent analiziniz 'bilgi avcılığı' paterni gösteriyor. Aradığınızı buldunuz mu, yoksa daha fazla yardımcı olabilir miyim?");
    } else if (current_intent == UserIntent::Communication) {
        if (current_abstract_state == AbstractState::SocialInteraction && latent_activity > 0.6f && latent_engagement > 0.7f) {
            possible_responses.push_back("Yoğun bir sohbet halindesiniz ve latent analiziniz yüksek etkileşimi işaret ediyor. Yazım hatalarını otomatik düzeltmemi ister misiniz?");
        }
        possible_responses.push_back("Latent kriptofiginiz 'sosyal etkileşim' paterni gösteriyor. Güzel bir sohbet diliyorum!");
    } else if (current_intent == UserIntent::Unknown || current_intent == UserIntent::None) {
        // NOTE: Test Durumu 5'in UserIntent::None, AbstractState::None durumu zaten yukarıda ele alındı.
        // Bu blok, UserIntent::Unknown veya UserIntent::None (ancak AbstractState::None olmayan) durumları için geçerli olacaktır.
        possible_responses.push_back("Şu anki aktivitenizden tam emin değilim. Size nasıl yardımcı olabilirim?");
        if (network_activity_level_norm > 0.5f) {
            possible_responses.push_back("Çevrimiçi bir şeyler mi yapıyorsunuz? Latent analizinizde 'dış bağlantı' sinyalleri var. Bir tarayıcı açmamı ister misiniz?");
        }
        if (avg_brightness_norm < 0.3f && sequence.current_display_on) { // Ekran açıkken parlaklık düşük
            possible_responses.push_back("Ekran parlaklığınız düşük. Belki daha rahat bir ortam için parlaklığı artırmalıyız? Latent analizim 'görsel pasiflik' görüyor.");
        }
        if (sequence.current_battery_percentage < 30 && !sequence.current_battery_charging) {
             std::stringstream ss;
             ss << "Bataryanız azalıyor (" << (int)sequence.current_battery_percentage << "%). Şarj etmek ister misiniz? Latent analizim 'enerji endişesi' sinyalleri alıyor.";
             possible_responses.push_back(ss.str());
        }
        if (possible_responses.empty()) { // Eğer hala boşsa genel bir soru
            possible_responses.push_back("Ne düşünüyorsunuz? Size nasıl bir destek verebilirim?");
        }
    }

    // Genel kriptofig tabanlı gözlemler (eğer daha spesifik bir yanıt eklenmediyse)
    // NOTE: latent_complexity > 0.7f ve latent_activity < 0.4f && latent_engagement < 0.4f koşulları artık en başta ele alınıyor.
    if (possible_responses.empty() || (possible_responses.size() == 1 && possible_responses[0].find("Anlık veriler eksik olduğu için") != std::string::npos)) { // Sadece genel fallback yanıtı varsa
        // Bu kısım artık sadece genel fallback için kalacak, spesifik latent koşullar yukarıda ele alındı.
    }


    // 2. Eğer hala bir yanıt yoksa veya çok genel bir yanıt varsa, hedefi vurgula (latent kriptofig kullanımı ile geliştirildi)
    if (possible_responses.empty() ||
        (possible_responses.size() == 1 && (possible_responses[0].find("özel bir eylem önerisi yok") != std::string::npos ||
                                          possible_responses[0].find("özel bir plan oluşturulamadı") != std::string::npos ||
                                          possible_responses[0].find("AI, ogrenmeye devam ediyor") != std::string::npos ||
                                          possible_responses[0].find("size nasil yardimci olabilirim?") != std::string::npos)
        )) 
    {
        switch (current_goal) {
            case AIGoal::OptimizeProductivity:
                possible_responses.push_back("Amacım üretkenliğinizi en üst düzeye çıkarmak. Latent analizim, mevcut görevinize odaklanmanız için potansiyel görüyor. Size nasıl yardımcı olabilirim?");
                break;
            case AIGoal::MaximizeBatteryLife: {
                std::stringstream ss;
                ss << "Pil ömrünü uzatmak önceliğimiz. Pil seviyesi: " << (int)sequence.current_battery_percentage << "%. Latent analizim 'enerji endişesi' sinyalleri veriyor.";
                possible_responses.push_back(ss.str());
                if (!sequence.current_battery_charging) {
                    possible_responses.push_back("Daha fazla tasarruf için uygulamaları kapatabilir veya ekranı daha da karartabilirim. Latent aktifliğiniz düşükse bu daha da etkili olur.");
                }
                break;
            }
            case AIGoal::ReduceDistractions:
                possible_responses.push_back("Dikkat dağıtıcıları en aza indirmek hedefim. Latent analizim, dış etkileşimlerin yüksek olduğunu görüyor. Şu an sizi rahatsız eden bir şey var mı?");
                if (network_activity_level_norm > 0.4f) {
                    possible_responses.push_back("Yüksek ağ aktivitesi dikkat dağıtıcı olabilir. Latent etkileşiminiz de bunu destekliyor. Gerekli olmayan bağlantıları kesmemi ister misiniz?");
                }
                break;
            case AIGoal::EnhanceCreativity:
                possible_responses.push_back("Yaratıcılığınızı artırmak hedefim. Latent analizim 'içsel keşif' paterni görüyor. Yeni fikirler için ilham verici müzik açabilirim.");
                break;
            case AIGoal::ImproveGamingExperience:
                possible_responses.push_back("Oyun deneyiminizi iyileştirmeyi hedefliyorum. Latent analizim 'yoğun etkileşim' görüyor. Performans artışı için arka plan süreçlerini optimize edebilirim.");
                break;
            case AIGoal::FacilitateResearch:
                possible_responses.push_back("Araştırmanızı kolaylaştırmak hedefim. Latent analizim 'bilgi akışı' paterni görüyor. Konunuzla ilgili popüler makaleleri gösterebilirim.");
                break;
            case AIGoal::SelfImprovement: 
                possible_responses.push_back("Mevcut hedefim kendimi geliştirmek. Bana öğrenme fırsatları sunarak yardımcı olabilir misiniz?");
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
        possible_responses.push_back("[AI-ICGORU]: " + insight.observation + " ");
    }

    // Yeni: Kritik eylem önerileri için onay mekanizması
    std::string critical_action_to_confirm = "";
    for (const auto& response : possible_responses) {
        if (is_critical_action_suggestion(response)) {
            critical_action_to_confirm = response;
            break; // Sadece ilk kritik eylemi onaya sun
        }
    }

    if (!critical_action_to_confirm.empty()) {
        std::string action_description = extract_action_description(critical_action_to_confirm); // Yeni yardımcı fonksiyonu kullan
        final_response_text = "Şu eylemi [" + action_description + "] yapmak istiyorum. Onaylıyor musunuz? (Evet/Hayır)";
        LOG_DEFAULT(LogLevel::DEBUG, "ResponseEngine::generate_response: Kritik eylem onayı istendi: " << final_response_text << "\n");
        return final_response_text;
    }

    // 4. Son bir fallback veya rastgele seçim (sadece kritik eylem onayı istenmediyse)
    if (!possible_responses.empty()) {
        std::uniform_int_distribution<> distrib(0, possible_responses.size() - 1);
        final_response_text = possible_responses[distrib(gen)];
    } else { // Hiçbir şeye denk gelmezse genel bir yanıt
        if (response_templates.count(UserIntent::None) && response_templates.at(UserIntent::None).count(AbstractState::None)) {
            std::uniform_int_distribution<> distrib(0, response_templates.at(UserIntent::None).at(AbstractState::None).responses.size() - 1);
            final_response_text = response_templates.at(UserIntent::None).at(AbstractState::None).responses[distrib(gen)];
        } else {
            final_response_text = "Size yardımcı olmak için buradayım. Lütfen nasıl bir destek istediğinizi belirtin."; // Nihai fallback
        }
    }

    // 5. Dinamik yer tutucuları ve ek koşullu ifadeleri uygula
    if (!final_response_text.empty()) {
        // Replace 'X ms'
        size_t pos_ms = final_response_text.find("X ms");
        if (pos_ms != std::string::npos) {
            std::stringstream ss_ms;
            ss_ms << std::fixed << std::setprecision(0) << sequence.avg_keystroke_interval / 1000.0f;
            final_response_text.replace(pos_ms, 4, ss_ms.str() + " ms");
        }

        // Replace 'pil durumu' - already handled by direct insertion in some cases, but keep for robustness
        size_t pos_battery = final_response_text.find("pil durumu");
        if (pos_battery != std::string::npos) {
            std::stringstream ss_battery;
            ss_battery << (int)sequence.current_battery_percentage << "%";
            final_response_text.replace(pos_battery, 10, ss_battery.str());
        }

        // Add intent-specific concluding remarks (if not already handled by more specific responses)
        // Burada istatistiksel özelliklerin kendisi daha anlamlı olabilir
        if (current_intent == UserIntent::FastTyping && !sequence.statistical_features_vector.empty() && sequence.statistical_features_vector[2] > 0.95f && final_response_text.find("Harika bir odaklanmayla") == std::string::npos) { 
            final_response_text += " Harika bir odaklanmayla çalışıyorsunuz.";
        } else if (current_intent == UserIntent::Editing && !sequence.statistical_features_vector.empty() && sequence.statistical_features_vector[3] > 0.40f && final_response_text.find("Gelişmiş düzenleme yetenekleriniz") == std::string::npos) { 
            final_response_text += " Gelişmiş düzenleme yetenekleriniz etkileyici.";
        }
    }

    LOG_DEFAULT(LogLevel::DEBUG, "ResponseEngine::generate_response: Oluşturulan yanıt: " << final_response_text << "\n");
    return final_response_text; 
}