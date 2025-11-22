#include "natural_language_processor.h"
#include "../core/logger.h"
#include "../core/enums.h"         // LogLevel, UserIntent, AbstractState gibi enum'lar için
#include "../core/utils.h"          // intent_to_string, abstract_state_to_string, goal_to_string, action_to_string için
#include "../brain/autoencoder.h"   // CryptofigAutoencoder için (INPUT_DIM, LATENT_DIM)
#include "../learning/Capsule.h"    // Capsule için
#include "../learning/KnowledgeBase.h" // KnowledgeBase için
#include "../data_models/dynamic_sequence.h" // DynamicSequence için

#include <iostream>
#include <algorithm> // std::transform için
#include <cctype>    // std::tolower için
#include <random>    // SafeRNG için
#include <stdexcept> // std::runtime_error için
#include <sstream>   // stringstream için
#include <fstream>   // FastText model yükleme için
#include <functional> // std::hash için (statik embedding için)
#include <algorithm> // std::min için
#include <QCoreApplication> // Çalıştırılabilir dosya yolunu almak için

#include <filesystem>

#ifdef _WIN32
#include <Windows.h>
#endif

namespace CerebrumLux {

std::map<Language, std::unique_ptr<fasttext::FastText>> NaturalLanguageProcessor::s_fastTextModels;

// YENİ: Model yükleme durumu kontrolü (GUI donmasını önlemek için)
#include <future>
#include <atomic>
static std::atomic<bool> s_isFastTextLoading{false};
std::atomic<bool> NaturalLanguageProcessor::s_isModelReady{false}; // Başlangıçta hazır değil

// YENİ: Dil string'ini enum'a çeviren yardımcı fonksiyon
Language string_to_lang(const std::string& lang_str) {
    std::string lower_lang_str = lang_str;
    std::transform(lower_lang_str.begin(), lower_lang_str.end(), lower_lang_str.begin(),
                   [](unsigned char c){ return static_cast<unsigned char>(std::tolower(c)); });
    if (lower_lang_str == "en" || lower_lang_str == "english") return Language::EN;
    if (lower_lang_str == "de" || lower_lang_str == "german") return Language::DE;
    if (lower_lang_str == "tr" || lower_lang_str == "turkish") return Language::TR;
    return Language::UNKNOWN;
}

// YENİ (GUI dostu): FastText modellerini asenkron yükleme
void NaturalLanguageProcessor::load_fasttext_models() {
    // Aynı anda birden fazla yükleme başlatılmasın
    if (s_isFastTextLoading.load()) {
        LOG_DEFAULT(LogLevel::DEBUG, "NLP: FastText modeli zaten yükleniyor, yeni istek atlandı.");
        return;
    }

    // Eğer zaten yüklenmişse tekrar yükleme
    if (!s_fastTextModels.empty() && s_fastTextModels.begin()->second && s_fastTextModels.begin()->second->getDimension() > 0) {
        return;
    }

    s_isFastTextLoading.store(true);

    // Arka planda yükleme (GUI'yi engellemeden)
    static std::future<void> model_loading_future;
    model_loading_future = std::async(std::launch::async, []() {
        try {
            // --- GÜVENİLİR VARLIK YOLU OLUŞTURMA ---
            std::filesystem::path app_dir;
#ifdef _WIN32
            char path[MAX_PATH];
            GetModuleFileNameA(NULL, path, MAX_PATH);
            app_dir = std::filesystem::path(path).parent_path();
#else
            if (QCoreApplication::instance()) {
                app_dir = QCoreApplication::applicationDirPath().toStdString();
            } else {
                app_dir = std::filesystem::current_path();
            }
#endif
            LOG_DEFAULT(LogLevel::DEBUG, "NLP: FastText modelleri için temel dizin aranıyor: " << app_dir.parent_path() / "data" / "fasttext_models"); // YENİ LOG: Temel FastText dizin yolunu gör

            //FastText modelleri için sabit kalıcı yol
            std::filesystem::path fasttext_models_dir = app_dir.parent_path() / "data" / "fasttext_models";

            static const std::map<Language, std::string> model_files = {
                {Language::EN, "cc.en.300.bin"},
                {Language::DE, "cc.de.300.bin"},
                {Language::TR, "cc.tr.300.bin"}
            };

            s_fastTextModels.clear();
            bool any_model_loaded = false;

            for (const auto& pair : model_files) {
                Language lang = pair.first;
                std::filesystem::path model_path = fasttext_models_dir / pair.second; // Yeni yolu kullan
                std::string absolute_path_str = std::filesystem::weakly_canonical(model_path).string();

                LOG_DEFAULT(LogLevel::TRACE, "NLP: FastText modeli yüklemeye çalışılıyor: " << absolute_path_str);
                LOG_DEFAULT(LogLevel::DEBUG, "NLP: Kontrol edilen FastText model yolu: " << absolute_path_str << " (exists: " << std::filesystem::exists(model_path) << ", size > 0: " << (std::filesystem::exists(model_path) ? (std::filesystem::file_size(model_path) > 0 ? "Evet" : "Hayır") : "Bilinmiyor") << ")"); // YENİ LOG: Detaylı dosya kontrolü

                if (std::filesystem::exists(model_path) && std::filesystem::file_size(model_path) > 0) { // YENİ: Dosya boyutu kontrolü
                    try {
                        auto model = std::make_unique<fasttext::FastText>();
                        LOG_DEFAULT(LogLevel::DEBUG, "NLP: FastText modeli yuklemeye baslaniyor (loadModel): " << absolute_path_str); // YENİ LOG: loadModel çağrısı öncesi

                        model->loadModel(absolute_path_str);
                        s_fastTextModels[lang] = std::move(model);
                        s_isModelReady.store(true); // Model artık hazır
                        LOG_DEFAULT(LogLevel::INFO, "NLP: FastText modeli başarıyla yüklendi: " << absolute_path_str);
                        LOG_DEFAULT(LogLevel::DEBUG, "NLP: Yüklenen model boyutu: " << s_fastTextModels[lang]->getDimension()); // YENİ LOG: Yüklenen modelin boyutunu onayla

                        any_model_loaded = true;
                    } catch (const std::exception& e) {
                        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "NLP: FastText modeli yüklenirken hata oluştu (" << absolute_path_str << "): " << e.what());
                    }
                } else { // YENİ: Model dosyası bulunamadığında daha detaylı log
                    LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "NLP: FastText modeli bulunamadi veya boş: " << absolute_path_str << " (Dosya var mi: " << std::filesystem::exists(model_path) << ", Boyut > 0 mi: " << (std::filesystem::exists(model_path) ? (std::filesystem::file_size(model_path) > 0 ? "Evet" : "Hayır") : "Bilinmiyor") << ").");

                    LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "NLP: FastText modeli bulunamadı: " << absolute_path_str);
                }
            }
            if (!any_model_loaded) {
                LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "NLP: Hiçbir FastText modeli yüklenemedi. Embedding'ler fallback placeholder olacaktır.");
            }
        } catch (const std::exception& e) {
            LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "NLP: FastText yükleme sırasında beklenmeyen hata: " << e.what());
        }

        s_isFastTextLoading.store(false);
        LOG_DEFAULT(LogLevel::INFO, "NLP: FastText model yükleme işlemi tamamlandı (async).");
    });
}

NaturalLanguageProcessor::NaturalLanguageProcessor(CerebrumLux::GoalManager& goal_manager_ref, CerebrumLux::KnowledgeBase& kbRef)
    : goal_manager(goal_manager_ref), kbRef_(kbRef)
{
    // YENİ: Tüm FastText modellerini yükle
    //load_fasttext_models();
    // DEĞİŞTİRİLDİ: FastText modellerinin yüklenmesi constructor'dan kaldırıldı.
    // Bu işlem artık main.cpp içindeki QTimer::singleShot ile asenkron olarak çağrılacak.
    LOG_DEFAULT(CerebrumLux::LogLevel::INFO, "NaturalLanguageProcessor: Initialized.");

    // Niyet anahtar kelime haritasını başlat (Mevcut kod aynı kalır)
    this->intent_keyword_map[CerebrumLux::UserIntent::Programming] = {"kod", "compile", "derle", "debug", "hata", "function", "class", "stack"};
    this->intent_keyword_map[CerebrumLux::UserIntent::Gaming]      = {"oyun", "fps", "level", "play", "match", "steam"};
    this->intent_keyword_map[CerebrumLux::UserIntent::MediaConsumption] = {"video", "izle", "film", "müzik", "spotify", "youtube"};
    this->intent_keyword_map[CerebrumLux::UserIntent::CreativeWork] = {"tasarla", "foto", "görsel", "müzik", "compose", "yarat", "üret"};
    this->intent_keyword_map[CerebrumLux::UserIntent::Research]    = {"araştır", "search", "makale", "pdf", "doküman", "read", "oku"};
    this->intent_keyword_map[CerebrumLux::UserIntent::Communication] = {"mail", "mesaj", "sohbet", "reply", "gönder"};
    this->intent_keyword_map[CerebrumLux::UserIntent::Editing]     = {"düzenle", "edit", "revize", "fix", "format"};
    this->intent_keyword_map[CerebrumLux::UserIntent::FastTyping]  = {"hızlı", "yaz", "typing", "type", "speed"};
    this->intent_keyword_map[CerebrumLux::UserIntent::Question] = {"neden", "nasıl", "kim", "ne", "niçin", "soru", "öğren"};
    this->intent_keyword_map[CerebrumLux::UserIntent::Command] = {"yap", "başlat", "durdur", "sil", "oluştur", "git"};
    this->intent_keyword_map[CerebrumLux::UserIntent::Statement] = {"bilgi", "gerçek", "biliyorum", "düşünüyorum"};
    this->intent_keyword_map[CerebrumLux::UserIntent::FeedbackPositive] = {"iyi", "güzel", "teşekkürler", "harika"};
    this->intent_keyword_map[CerebrumLux::UserIntent::FeedbackNegative] = {"kötü", "hayır", "beğenmedim", "yanlış"};
    this->intent_keyword_map[CerebrumLux::UserIntent::Greeting] = {"merhaba", "selam", "hi", "günaydın"};
    this->intent_keyword_map[CerebrumLux::UserIntent::Farewell] = {"güle güle", "hoşça kal", "bay bay", "görüşürüz"};
    this->intent_keyword_map[CerebrumLux::UserIntent::RequestInformation] = {"ver", "göster", "bilgi"};
    this->intent_keyword_map[CerebrumLux::UserIntent::ExpressEmotion] = {"mutlu", "üzgün", "sinirli"};
    this->intent_keyword_map[CerebrumLux::UserIntent::Confirm] = {"evet", "tamam", "onayla"};
    this->intent_keyword_map[CerebrumLux::UserIntent::Deny] = {"hayır", "reddet"};
    this->intent_keyword_map[CerebrumLux::UserIntent::Elaborate] = {"açıkla", "detaylandır"};
    this->intent_keyword_map[CerebrumLux::UserIntent::Clarify] = {"anlamadım", "tekrarla"};
    this->intent_keyword_map[CerebrumLux::UserIntent::CorrectError] = {"düzelt", "hata", "yanlış"};
    this->intent_keyword_map[CerebrumLux::UserIntent::InquireCapability] = {"yapabilir misin", "yeteneğin ne"};
    this->intent_keyword_map[CerebrumLux::UserIntent::ShowStatus] = {"durum", "nedir"};
    this->intent_keyword_map[CerebrumLux::UserIntent::ExplainConcept] = {"anlamı ne", "nedir"};

    // Durum anahtar kelime haritasını başlat (Mevcut kod aynı kalır)
    this->state_keyword_map[CerebrumLux::AbstractState::PowerSaving] = {"pil", "battery", "şarj", "charging", "battery low", "pil zayıf"};
    this->state_keyword_map[CerebrumLux::AbstractState::FaultyHardware] = {"donanım", "arızalı", "error", "çök", "crash", "bozul"};
    this->state_keyword_map[CerebrumLux::AbstractState::Distracted] = {"dikkat", "dikkatim", "dikkat dağılı", "notification"};
    this->state_keyword_map[CerebrumLux::AbstractState::Focused] = {"odak", "focus", "konsantre", "akış"};
    this->state_keyword_map[CerebrumLux::AbstractState::SeekingInformation] = {"ara", "google", "bilgi", "sorgu"};

    // NLP'nin dahili model ağırlıklarını başlat (Mevcut kod aynı kalır)
    for (auto intent_pair : this->intent_keyword_map) {
        this->intent_cryptofig_weights[intent_pair.first].assign(CerebrumLux::CryptofigAutoencoder::LATENT_DIM, 0.0f);
        for (size_t i = 0; i < CerebrumLux::CryptofigAutoencoder::LATENT_DIM; ++i) {
            this->intent_cryptofig_weights[intent_pair.first][i] = static_cast<float>(CerebrumLux::SafeRNG::getInstance().get_generator()()) / CerebrumLux::SafeRNG::getInstance().get_generator().max();
        }
    }
    LOG_DEFAULT(CerebrumLux::LogLevel::INFO, "NaturalLanguageProcessor: Initialized.");
}

CerebrumLux::UserIntent NaturalLanguageProcessor::infer_intent_from_text(const std::string& user_input) const {
    std::string lower_text = user_input;
    std::transform(lower_text.begin(), lower_text.end(), lower_text.begin(),
                   [](unsigned char c){ return static_cast<unsigned char>(std::tolower(c)); });

    // Kural tabanlı tahmin
    CerebrumLux::UserIntent guessed_intent = rule_based_intent_guess(lower_text);
    if (guessed_intent != CerebrumLux::UserIntent::Undefined) {
        LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "NLP: Niyet kural tabanlı olarak tahmin edildi: " << CerebrumLux::intent_to_string(guessed_intent));
        return guessed_intent;
    }

    // YENİ KOD: KnowledgeBase'den semantik arama ile niyet çıkarımı (eğer kural tabanlı başarısız olursa)
    // Kullanıcının girdisine en yakın 1-2 kapsülü ara
    std::vector<float> user_input_embedding = NaturalLanguageProcessor::generate_text_embedding(lower_text, Language::EN); // Statik metod çağrısı
    std::vector<CerebrumLux::Capsule> related_capsules = kbRef_.semantic_search(user_input_embedding, 2); // Embedding ile ara
    if (!related_capsules.empty()) {
        // En alakalı kapsülün konusunu niyete çevirmeye çalış
        // Basitçe: eğer topic bir niyete karşılık geliyorsa, onu kullan.
        // Daha sofistike bir yaklaşımda: niyet sınıflandırıcı, kapsül içeriğiyle eğitilir.
        for (const auto& capsule : related_capsules) {
            if (capsule.topic == "Programming") return CerebrumLux::UserIntent::Programming;
            if (capsule.topic == "Research" || capsule.topic == "WebSearch") return CerebrumLux::UserIntent::Research;
            if (capsule.topic == "AI Insight") return CerebrumLux::UserIntent::Question; // AI Insight topic'i bir soruya yönlendirebilir.
            // Diğer topic'ler için de benzer eşlemeler eklenebilir.
        }
        LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "NLP: Niyet KnowledgeBase'den türetilmeye çalışıldı, ancak doğrudan bir eşleşme bulunamadı.");
    }

    // Daha karmaşık modeller veya öğrenilmiş modeller burada kullanılabilir
    LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "NLP: Niyet tahmin edilemedi, Undefined döndürülüyor.");
    return CerebrumLux::UserIntent::Undefined;
}

CerebrumLux::AbstractState NaturalLanguageProcessor::infer_state_from_text(const std::string& user_input) const {
    std::string lower_text = user_input;
    std::transform(lower_text.begin(), lower_text.end(), lower_text.begin(),
                   [](unsigned char c){ return static_cast<unsigned char>(std::tolower(c)); });

    CerebrumLux::AbstractState guessed_state = rule_based_state_guess(lower_text);
    if (guessed_state != CerebrumLux::AbstractState::Idle) {
        LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "NLP: Durum kural tabanlı olarak tahmin edildi: " << CerebrumLux::abstract_state_to_string(guessed_state));
        return guessed_state;
    }
    LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "NLP: Durum tahmin edilemedi, Idle döndürülüyor.");
    return CerebrumLux::AbstractState::Idle;
}

float NaturalLanguageProcessor::cryptofig_score_for_intent(CerebrumLux::UserIntent intent, const std::vector<float>& latent_cryptofig) const {
    auto it = this->intent_cryptofig_weights.find(intent);
    if (it == this->intent_cryptofig_weights.end() || latent_cryptofig.empty()) {
        return 0.0f;
    }

    const std::vector<float>& weights = it->second;
    if (weights.size() != latent_cryptofig.size()) {
        LOG_DEFAULT(CerebrumLux::LogLevel::WARNING, "NLP: cryptofig_score_for_intent - Ağırlık ve latent kriptofig boyutu uyuşmuyor.");
        return 0.0f;
    }

    float dot_product = 0.0f;
    for (size_t i = 0; i < weights.size(); ++i) {
        dot_product += weights[i] * latent_cryptofig[i];
    }
    // Basitçe dot product döndür, daha sonra sigmoid veya başka bir aktivasyon eklenebilir
    return dot_product;
}

ChatResponse NaturalLanguageProcessor::generate_response_text(
    CerebrumLux::UserIntent current_intent,
    CerebrumLux::AbstractState current_abstract_state,
    CerebrumLux::AIGoal current_goal,
    const CerebrumLux::DynamicSequence& sequence,
    const std::vector<std::string>& relevant_keywords,
    const CerebrumLux::KnowledgeBase& kb
) const {
    LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "NaturalLanguageProcessor: generate_response_text çağrıldı. Niyet: " << intent_to_string(current_intent) << ", Durum: " << abstract_state_to_string(current_abstract_state));

    ChatResponse response_obj;
    std::string generated_text = "";
    std::string reasoning_text = "";
    bool clarification_needed = false;
 
    // 1. Arama Sorgusunu Belirle
    std::string search_term_for_embedding = "";
    
    // ÖNCELİK 1: Sequence geçmişindeki son tam kullanıcı mesajını al (En doğru bağlam)
    if (!sequence.user_input_history.empty()) {
        search_term_for_embedding = sequence.user_input_history.back();
        LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "NLP: Arama sorgusu Sequence tarihçesinden alındı: " << search_term_for_embedding);
    }
    
    // ÖNCELİK 2: Eğer tarihçe boşsa, anahtar kelimeleri veya niyeti kullan
    if (search_term_for_embedding.empty()) {
         if (!relevant_keywords.empty()) {
             search_term_for_embedding = relevant_keywords[0];
             if (relevant_keywords.size() > 1) search_term_for_embedding += " " + relevant_keywords[1];
         } else {
             search_term_for_embedding = intent_to_string(current_intent);
             if (current_intent == CerebrumLux::UserIntent::Question) search_term_for_embedding += " nedir";
         }
    }
 
    // 2. Embedding Oluştur
    std::vector<float> search_query_embedding = NaturalLanguageProcessor::generate_text_embedding(search_term_for_embedding, Language::EN);
 
    LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "NaturalLanguageProcessor: Semantic arama için oluşturulan embedding (ilk 5 float): " 
                << search_query_embedding[0] << ", " << search_query_embedding[1] << ", " << search_query_embedding[2] << ", " << search_query_embedding[3] << ", " << search_query_embedding[4] 
                << " (Sorgu: '" << search_term_for_embedding << "')");
 
    // 3. Arama Stratejisi
    // Öncelikle, temel tanımlayıcı kapsülleri doğrudan ID ile arayalım.
    std::vector<CerebrumLux::Capsule> definitive_results;
    bool found_by_id = false;

    // "Cerebrum Lux nedir?" benzeri bir soru için
    if (search_term_for_embedding.find("Cerebrum Lux") != std::string::npos || search_term_for_embedding.find("nedir") != std::string::npos || search_term_for_embedding.find("tanım") != std::string::npos) {
        std::optional<CerebrumLux::Capsule> def_capsule = kb.find_capsule_by_id("CerebrumLux_Definition_17000000000000000");
        if (def_capsule) {
            definitive_results.push_back(*def_capsule);
            LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "NaturalLanguageProcessor: Doğrudan ID ile 'Cerebrum Lux Tanımı' kapsülü bulundu.");
            found_by_id = true;
        }
    }
    // "Yeteneklerin neler?" veya "Nasıl yardımcı olabilirsin?" benzeri bir soru için
    if (search_term_for_embedding.find("yetenek") != std::string::npos || search_term_for_embedding.find("nasıl yardımcı olabilirsin") != std::string::npos || current_intent == CerebrumLux::UserIntent::InquireCapability) {
        std::optional<CerebrumLux::Capsule> cap_capsule = kb.find_capsule_by_id("CerebrumLux_Capabilities_17000000000000000");
        if (cap_capsule) {
            definitive_results.push_back(*cap_capsule);
            LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "NaturalLanguageProcessor: Doğrudan ID ile 'Yetenekleri' kapsülü bulundu.");
            found_by_id = true;
        }
    }

    // 4. Sonuçları Hazırla
    std::vector<CerebrumLux::Capsule> final_results_for_synthesis;

    if (found_by_id && !definitive_results.empty()) {
        final_results_for_synthesis = definitive_results;
        LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "NaturalLanguageProcessor: Doğrudan tanımlayıcı kapsüller önceliklendirildi.");
    } else {
        // Doğrudan ID ile bulunamazsa, semantik arama yap (Yanıt + Öneriler için daha fazla sonuç al)
        std::vector<CerebrumLux::Capsule> semantic_search_results = kb.semantic_search(search_query_embedding, 6);
        LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "NaturalLanguageProcessor: KnowledgeBase'den semantik arama yapıldı. Bulunan kapsül sayısı: " << semantic_search_results.size() << " (Sorgu: '" << search_term_for_embedding << "')");
 
        // Semantic arama sonuçlarını da final_results'a ekle, ancak boş veya anlamsız olanları filtrele
        for (const auto& capsule : semantic_search_results) {
            // Kapsül içeriğinin anlamsız olup olmadığını kontrol et
            if (capsule.content.empty() ||
                capsule.content.find("Is this data relevant to AI Insight?") != std::string::npos ||
                capsule.content.find("Bilgi bulunamadı.") != std::string::npos ||
                capsule.plain_text_summary.empty() ||
                capsule.plain_text_summary.find("Is this data relevant to AI Insight?") != std::string::npos ||
                capsule.plain_text_summary.find("Bilgi bulunamadı.") != std::string::npos) {
                LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "NaturalLanguageProcessor: Anlamsız veya boş içeriğe sahip kapsül filtrelendi. ID: " << capsule.id);
                continue;
            }
            final_results_for_synthesis.push_back(capsule);
        }
        LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "NaturalLanguageProcessor: Semantik arama sonuçları filtrelendi ve yanıt sentezi için hazırlandı. Sayı: " << final_results_for_synthesis.size());
    }

    // 5. Yanıt Sentezi (Gelişmiş)
    if (!final_results_for_synthesis.empty()) {
        std::stringstream response_stream;
        std::stringstream reasoning_stream;
        std::vector<std::string> cited_capsule_ids;

        response_stream << "Bilgi tabanımda sorunuzla ilgili bazı bilgiler buldum: \n";
        reasoning_stream << "Yanıt, KnowledgeBase'deki ";

        size_t used_capsule_count = 0;
        // En alakalı kapsülleri birleştirerek yanıt oluştur
        for (size_t i = 0; i < final_results_for_synthesis.size(); ++i) {
            const CerebrumLux::Capsule& capsule = final_results_for_synthesis[i];
            cited_capsule_ids.push_back(capsule.id);
            
            // YENİ LOG: Bulunan her kapsülün ID'sini, konusunu ve içeriğini (özet veya tam) logla.
            LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "NaturalLanguageProcessor: Bulunan Kapsül - ID: " << capsule.id 
                        << ", Konu: " << capsule.topic 
                        << ", Özet: '" << capsule.plain_text_summary.substr(0, std::min((size_t)100, capsule.plain_text_summary.length())) << "'"
                        << ", İçerik Boyutu: " << capsule.content.length()
                        << ", İçerik (İlk 100 char): '" << capsule.content.substr(0, std::min((size_t)100, capsule.content.length())) << "...'");

            // Kapsülün içeriğini veya özetini kullanarak daha zengin bir yanıt oluştur
            std::string display_content = capsule.content;
            if (display_content.empty() ||
                display_content.find("Is this data relevant to AI Insight?") != std::string::npos ||
                display_content.find("Bilgi bulunamadı.") != std::string::npos)
            {
                display_content = capsule.plain_text_summary; // İçerik boşsa özeti kullan
                if (display_content.empty() ||
                    display_content.find("Is this data relevant to AI Insight?") != std::string::npos ||
                    display_content.find("Bilgi bulunamadı.") != std::string::npos)
                {
                    display_content = "İlgili bilgi mevcut değil."; // Hem içerik hem özet boşsa veya anlamsızsa
                }
            }
            
            // Metni biraz kırp (çok uzunsa) ve sonuna ... ekle
            std::string content_snippet = display_content.substr(0, std::min((size_t)400, display_content.length()));
            if (display_content.length() > 400) content_snippet += "...";

            response_stream << "\n- **" << capsule.topic << "**: " << content_snippet << " [cite:" << capsule.id << "]";

            reasoning_stream << "'" << capsule.topic << "' (ID: " << capsule.id << ")";
            if (i < final_results_for_synthesis.size() - 1) {
                reasoning_stream << ", ";
            }
            used_capsule_count++;
            
            // İlk 2 kapsülü yanıt için kullan, gerisini önerilere sakla
            if (used_capsule_count >= 2) break;
        }
        reasoning_stream << " kapsüllerinin sentezlenmesiyle oluşturuldu.";

        generated_text = response_stream.str();
        reasoning_text = reasoning_stream.str();

        if (used_capsule_count > 1) {
            generated_text += "\nDaha detaylı bilgi için yukarıdaki referanslara tıklayabilirsiniz.";
        }

        // YENİ: Geriye kalan kapsüllerden öneri soruları oluştur
        for (size_t i = used_capsule_count; i < final_results_for_synthesis.size(); ++i) {
            const CerebrumLux::Capsule& cap = final_results_for_synthesis[i];
            if (!cap.topic.empty() && cap.topic != "AI Insight") { // "AI Insight" başlığı çok genel, onu atla
                 response_obj.suggested_questions.push_back(cap.topic + " hakkında bilgi ver");
            }
        }
        // Eğer hiç öneri çıkmazsa genel bir tane ekle
        if (response_obj.suggested_questions.empty()) {
            response_obj.suggested_questions.push_back("Yeteneklerin neler?");
        }
        
        LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "NaturalLanguageProcessor: KnowledgeBase'den sentezlenmiş yanıt üretildi. Yanıt uzunluğu: " << generated_text.length());
    } else {
        LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "NaturalLanguageProcessor: KnowledgeBase'de ilgili kapsül bulunamadı. Kural tabanlı yanıta dönülüyor.");
        
        // Adım 2: Kural tabanlı yanıt üretimi ve dinamik prompt kullanımı
        std::string dynamic_prompt = generate_dynamic_prompt(current_intent, current_abstract_state, current_goal, sequence, search_term_for_embedding, final_results_for_synthesis); 
        LOG_DEFAULT(CerebrumLux::LogLevel::TRACE, "NaturalLanguageProcessor: Dinamik Olarak Üretilen Prompt: " << dynamic_prompt);
    }

    // Durum ve hedefe dayalı bağlamsal eklemeler yap
    std::string contextual_additions = "";
    std::string contextual_reasoning = "";
    
    if (current_goal == CerebrumLux::AIGoal::OptimizeProductivity) { contextual_additions += " \nVerimliliğinizi artırmaya odaklanıyorum."; }
    
    // Eğer KnowledgeBase'den yanıt alınamadıysa ve generated_text hala boşsa, fallback yanıtı kullan
    if (generated_text.empty() || generated_text == "Bilgi tabanımda sorunuzla ilgili bazı bilgiler buldum: \n") {
        generated_text = fallback_response_for_intent(current_intent, current_abstract_state, sequence);
        reasoning_text = "KnowledgeBase'de doğrudan ilgili bilgi bulunamadığı için kural tabanlı fallback yanıt kullanıldı. " + contextual_reasoning;
        clarification_needed = true;
    }

    generated_text += contextual_additions;

    // ChatResponse objesini doldur ve döndür
    response_obj.text = generated_text;
    response_obj.reasoning = reasoning_text;
    response_obj.needs_clarification = clarification_needed;

    return response_obj;
}

// And the rest of the file
std::string NaturalLanguageProcessor::generate_dynamic_prompt(
    CerebrumLux::UserIntent intent,
    CerebrumLux::AbstractState state,
    CerebrumLux::AIGoal goal,
    const CerebrumLux::DynamicSequence& sequence,
    const std::string& user_input, // Metoda user_input da ekleyebiliriz
     const std::vector<CerebrumLux::Capsule>& relevant_capsules
 ) const {
    std::stringstream ss;
    ss << std::string("Kullanıcının niyeti: ") << CerebrumLux::intent_to_string(intent) << std::string(". ");
    ss << std::string("Mevcut sistem durumu: ") << CerebrumLux::abstract_state_to_string(state) << std::string(". ");
    ss << std::string("AI'ın mevcut hedefi: ") << CerebrumLux::goal_to_string(goal) << std::string(". ");
    ss << std::string("Kullanıcı girdisi (veya anahtar kelimelerden türetilen sorgu): '") << user_input << std::string("'. ");
 
     if (!relevant_capsules.empty()) {
        ss << std::string("Aşağıdaki ilgili bilgi tabanı kapsüllerini dikkate al: ");
         for (const auto& capsule : relevant_capsules) {
            ss << std::string("(ID: ") << capsule.id << std::string(", Konu: ") << capsule.topic << std::string(", Özet: '") << capsule.plain_text_summary.substr(0, std::min((size_t)100, capsule.plain_text_summary.length())) << std::string("...') ");
         }
     } else {
        ss << std::string("İlgili bilgi tabanı kapsülü bulunamadı. ");
     }
 
    ss << std::string("Bu bağlamı kullanarak, kullanıcıya kapsamlı, içgörülü, 2-3 farklı perspektiften değerlendirme içeren, olası sonuçlar ve öneriler sunan bir yanıt üret. Eğer niyet belirsizse, açıklama talep et.");
 
     return ss.str();
}

CerebrumLux::UserIntent NaturalLanguageProcessor::rule_based_intent_guess(const std::string& lower_text) const {
    for (const auto& pair : this->intent_keyword_map) {
        for (const auto& keyword : pair.second) {
            if (lower_text.find(keyword) != std::string::npos) {
                return pair.first;
            }
        }
    }
    return CerebrumLux::UserIntent::Undefined;
}

CerebrumLux::AbstractState NaturalLanguageProcessor::rule_based_state_guess(const std::string& lower_text) const {
    for (const auto& pair : this->state_keyword_map) {
        for (const auto& keyword : pair.second) {
            if (lower_text.find(keyword) != std::string::npos) {
                return pair.first;
            }
        }
    }
    return CerebrumLux::AbstractState::Idle;
}

void NaturalLanguageProcessor::update_model(const std::string& observed_text, CerebrumLux::UserIntent true_intent, const std::vector<float>& latent_cryptofig) {
    if (latent_cryptofig.empty() || latent_cryptofig.size() != CerebrumLux::CryptofigAutoencoder::LATENT_DIM) {
        LOG_DEFAULT(CerebrumLux::LogLevel::WARNING, "NLP::update_model: Gecersiz latent kriptofig boyutu. Model guncellenemedi.");
        return;
    }

    auto& weights = this->intent_cryptofig_weights[true_intent];
    if (weights.empty()) {
        weights.assign(CerebrumLux::CryptofigAutoencoder::LATENT_DIM, 0.0f);
    }

    float learning_rate_nlp = 0.1f;
    for (size_t i = 0; i < latent_cryptofig.size(); ++i) {
        weights[i] += learning_rate_nlp * (latent_cryptofig[i] - weights[i]);
        weights[i] = std::min(10.0f, std::max(-10.0f, weights[i]));
    }
    LOG_DEFAULT(CerebrumLux::LogLevel::DEBUG, "NLP::update_model updated weights for intent " << CerebrumLux::intent_to_string(true_intent));
}

void NaturalLanguageProcessor::trainIncremental(const std::string& input, const std::string& expected_intent) {
    std::string lower_input = input;
    std::transform(lower_input.begin(), lower_input.end(), lower_input.begin(),
                   [](unsigned char c){ return static_cast<unsigned char>(std::tolower(c)); });

    CerebrumLux::UserIntent true_intent = CerebrumLux::UserIntent::Undefined;
    if (expected_intent == "Programming") true_intent = CerebrumLux::UserIntent::Programming;
    else if (expected_intent == "Gaming") true_intent = CerebrumLux::UserIntent::Gaming;
    else if (expected_intent == "MediaConsumption") true_intent = CerebrumLux::UserIntent::MediaConsumption;
    else if (expected_intent == "CreativeWork") true_intent = CerebrumLux::UserIntent::CreativeWork;
    else if (expected_intent == "Research") true_intent = CerebrumLux::UserIntent::Research;
    else if (expected_intent == "Communication") true_intent = CerebrumLux::UserIntent::Communication;
    else if (expected_intent == "Editing") true_intent = CerebrumLux::UserIntent::Editing;
    else if (expected_intent == "FastTyping") true_intent = CerebrumLux::UserIntent::FastTyping;
    else if (expected_intent == "Question") true_intent = CerebrumLux::UserIntent::Question;
    else if (expected_intent == "Command") true_intent = CerebrumLux::UserIntent::Command;
    else if (expected_intent == "Statement") true_intent = CerebrumLux::UserIntent::Statement;
    else if (expected_intent == "FeedbackPositive") true_intent = CerebrumLux::UserIntent::FeedbackPositive;
    else if (expected_intent == "FeedbackNegative") true_intent = CerebrumLux::UserIntent::FeedbackNegative;
    else if (expected_intent == "Greeting") true_intent = CerebrumLux::UserIntent::Greeting;
    else if (expected_intent == "Farewell") true_intent = CerebrumLux::UserIntent::Farewell;
    else if (expected_intent == "RequestInformation") true_intent = CerebrumLux::UserIntent::RequestInformation;
    else if (expected_intent == "ExpressEmotion") true_intent = CerebrumLux::UserIntent::ExpressEmotion;
    else if (expected_intent == "Confirm") true_intent = CerebrumLux::UserIntent::Confirm;
    else if (expected_intent == "Deny") true_intent = CerebrumLux::UserIntent::Deny;
    else if (expected_intent == "Elaborate") true_intent = CerebrumLux::UserIntent::Elaborate;
    else if (expected_intent == "Clarify") true_intent = CerebrumLux::UserIntent::Clarify;
    else if (expected_intent == "CorrectError") true_intent = CerebrumLux::UserIntent::CorrectError;
    else if (expected_intent == "InquireCapability") true_intent = CerebrumLux::UserIntent::InquireCapability;
    else if (expected_intent == "ShowStatus") true_intent = CerebrumLux::UserIntent::ShowStatus;
    else if (expected_intent == "ExplainConcept") true_intent = CerebrumLux::UserIntent::ExplainConcept;


    if (true_intent == CerebrumLux::UserIntent::Undefined) {
        LOG_DEFAULT(CerebrumLux::LogLevel::WARNING, "NLP::trainIncremental: Bilinmeyen niyet: " << expected_intent << ". Öğrenme atlandı.");
        return;
    }

    std::vector<float> dummy_cryptofig(CerebrumLux::CryptofigAutoencoder::LATENT_DIM, 0.5f);

    update_model(input, true_intent, dummy_cryptofig);
    LOG_DEFAULT(CerebrumLux::LogLevel::INFO, "NLP::trainIncremental: '" << expected_intent << "' niyeti için artımlı öğrenme tamamlandı.");
}

void NaturalLanguageProcessor::trainFromKnowledgeBase(const CerebrumLux::KnowledgeBase& kb) {
    LOG_DEFAULT(CerebrumLux::LogLevel::INFO, "NaturalLanguageProcessor: KnowledgeBase'den eğitim başlatıldı.");
    std::vector<CerebrumLux::Capsule> all_capsules = kb.get_all_capsules();
    for (const auto& capsule : all_capsules) {
        LOG_DEFAULT(CerebrumLux::LogLevel::TRACE, "NaturalLanguageProcessor: Kapsülden eğitim (placeholder). ID: " << capsule.id << ", Konu: " << capsule.topic);
    }
    LOG_DEFAULT(CerebrumLux::LogLevel::INFO, "NaturalLanguageProcessor: KnowledgeBase'den eğitim tamamlandı (placeholder).");
}


void NaturalLanguageProcessor::load_model(const std::string& path) {
    LOG_DEFAULT(CerebrumLux::LogLevel::INFO, "NLP: Model yüklendi (placeholder): " << path);
}

void NaturalLanguageProcessor::save_model(const std::string& path) const {
    LOG_DEFAULT(CerebrumLux::LogLevel::INFO, "NLP: Model kaydedildi (placeholder): " << path);
}

std::string NaturalLanguageProcessor::fallback_response_for_intent(CerebrumLux::UserIntent intent, CerebrumLux::AbstractState state, const CerebrumLux::DynamicSequence& sequence) const {
    std::string response = "Bu konuda emin değilim. ";
    if (intent == CerebrumLux::UserIntent::Undefined || intent == CerebrumLux::UserIntent::Unknown) {
        response += "Niyetinizi tam olarak anlayamadım.";
    } else {
        response += "Niyetiniz '" + CerebrumLux::intent_to_string(intent) + "' gibi görünüyor, ancak daha fazla bilgiye ihtiyacım var.";
    }
    return response;
}

// Metin girdisinden embedding hesaplama (FastText kullanılarak)
std::vector<float> NaturalLanguageProcessor::generate_text_embedding(const std::string& text, Language lang) {
    // Model henüz yüklenmediyse yüklemeyi tetikle
    if (!s_isModelReady.load()) {
        load_fasttext_models();
    }

    const int embedding_dim = 128; // Hedef embedding boyutu
    std::vector<float> embedding(embedding_dim, 0.0f);

    // Model hazır değilse placeholder döndür (Race condition engellendi)
    if (!s_isModelReady.load()) {
        LOG_DEFAULT(LogLevel::WARNING, "NLP: FastText modeli henüz hazır değil. Placeholder embedding kullanılıyor.");
        // Fallback: Rastgele ama deterministik
        std::hash<std::string> hasher;
        size_t hash = hasher(text);
        std::mt19937 gen(hash);
        std::uniform_real_distribution<> dis(-0.1, 0.1);
        for (int i = 0; i < embedding_dim; ++i) embedding[i] = dis(gen);
        return embedding;
    }

    auto model_it = s_fastTextModels.find(lang);
    if (model_it != s_fastTextModels.end() && model_it->second && model_it->second->getDimension() > 0) {
        fasttext::FastText& model = *(model_it->second); // unique_ptr'dan referans al
        fasttext::Vector ft_embedding(model.getDimension()); // FastText'in kendi Vector sınıfı
        
        if (text.empty()) { // Boş metin için boş embedding döndür
            return embedding;
        }

        std::stringstream text_stream(text);
        model.getSentenceVector(text_stream, ft_embedding);


        // FastText embedding'ini hedef boyutumuza (128) sığdır
        for (int i = 0; i < std::min((int)ft_embedding.size(), embedding_dim); ++i) {
            embedding[i] = ft_embedding[i];
        }
        // Eğer FastText embedding boyutu (model.getDimension()) hedef embedding_dim'den büyükse,
        // kalan boyutları sıfırla. Eğer küçükse, zaten padding yapılmış olacaktır.
        for (int i = ft_embedding.size(); i < embedding_dim; ++i) {
            embedding[i] = 0.0f;
        }
        LOG_DEFAULT(LogLevel::TRACE, "NLP: Metin '" << text.substr(0, std::min(text.length(), (size_t)50)) << "...' için FastText embedding olusturuldu. Boyut: " << embedding.size());
        return embedding;
    }
    // Fallback: Model yüklenemedi veya hazır değilse deterministik kelime tabanlı hash placeholder üret
    LOG_DEFAULT(LogLevel::WARNING, "NLP: FastText modeli veya belirtilen dil için model kullanılamıyor. Placeholder embedding üretiliyor.");
    
    if (text.empty()) {
        return embedding; // Sıfırlarla dolu embedding
    }

    for (int i = 0; i < embedding_dim; ++i) {
        embedding[i] = SafeRNG::getInstance().get_float(-1.0f, 1.0f);
    }
    LOG_DEFAULT(LogLevel::TRACE, "NLP: Metin '" << text.substr(0, std::min(text.length(), (size_t)50)) << "...' için anlamsal placeholder embedding olusturuldu.");
    return embedding;
}

} // namespace CerebrumLux