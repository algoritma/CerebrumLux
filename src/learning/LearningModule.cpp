#include "LearningModule.h"
#include <iostream>
#include "../learning/WebFetcher.h" // YENİ: WebFetcher için dahil edildi
#include "../communication/ai_insights_engine.h" // YENİ: AIInsight için dahil edildi (process_ai_insights'ta kullanıldığı için)


void LearningModule::learnFromText(const std::string& text,
                                   const std::string& source,
                                   const std::string& topic,
                                   float confidence)
{
    Capsule c;
    // Basit bir ID ataması yapalım (örneğin, bir hash veya sayaç)
    static int text_id_counter = 1000; // Insights'tan farklı bir başlangıç
    c.id = ++text_id_counter; 

    c.topic = topic;
    c.source = source;
    c.content = text; // Şifrelemeden önce orijinal içeriği sakla
    c.encrypted_content = knowledgeBase.encrypt(text); // encrypt metodu KnowledgeBase'de
    c.confidence = confidence;

    knowledgeBase.addCapsule(c);
    knowledgeBase.save(); // Parametresiz save metodu çağrılıyor
}

void LearningModule::learnFromWeb(const std::string& query) {
    WebFetcher fetcher;
    auto results = fetcher.search(query);

    for (auto& r : results) {
        // Özel kriptofik format: XOR ile şifreli saklama
        learnFromText(r.content, r.source, query, 0.9f);
    }
}

// YENİ: AI Insights verisini işleme metodu implementasyonu
void LearningModule::process_ai_insights(const std::vector<AIInsight>& insights) {
    std::cout << "[LearningModule] AI Insights isleniyor: " << insights.size() << " adet içgörü.\n";
    for (const auto& insight : insights) {
        // Her bir AIInsight'ı bir Capsule'a dönüştürüp KnowledgeBase'e ekleyelim
        Capsule c;
        // Basit bir ID ataması yapalım (örneğin, bir hash veya sayaç)
        static int insight_id_counter = 0;
        c.id = ++insight_id_counter; 
        c.content = insight.observation; // Orijinal içeriği kaydet
        c.source = "AIInsightsEngine";
        c.topic = "AI Insight"; // Veya daha spesifik bir konu
        c.confidence = insight.urgency; // Urgency'i confidence olarak kullanabiliriz
        
        // İçeriği şifrele
        c.encrypted_content = knowledgeBase.encrypt(c.content); 

        knowledgeBase.addCapsule(c);
        std::cout << "[LearningModule] KnowledgeBase'e içgörü kapsülü eklendi: " << c.content.substr(0, std::min((size_t)30, c.content.length())) << "...\n";
    }
    // Tüm içgörüler işlendikten sonra KnowledgeBase'i kaydet
    knowledgeBase.save(); 
}