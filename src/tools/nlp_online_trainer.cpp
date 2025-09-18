#include <iostream>
#include <string>
#include "communication/natural_language_processor.h"
#include "core/logger.h"

int main() {
    // Initialize logger
    Logger::get_instance().init(LogLevel::INFO);

    // Use the default constructor for NLP
    NaturalLanguageProcessor nlp;

    std::cout << "🔹 NLP Online Trainer başlatıldı." << std::endl;

    try {
        nlp.load_model("data/models/nlp_model.dat");
        LOG_DEFAULT(LogLevel::INFO, "Model başarıyla yüklendi.");
    } catch (...) {
        LOG_DEFAULT(LogLevel::WARNING, "Model bulunamadı, yeni model oluşturulacak.");
    }

    std::string input, expected;
    while (true) {
        std::cout << "\nCümle girin (veya 'exit' ile çık): ";
        std::getline(std::cin, input);
        if (input == "exit") break;

        std::cout << "Beklenen intent girin: ";
        std::getline(std::cin, expected);
        if (expected.empty()) {
            LOG_DEFAULT(LogLevel::WARNING, "Boş intent girildi, atlanıyor.");
            continue;
        }

        // 🔹 Incremental training
        nlp.trainIncremental(input, expected);
        LOG_DEFAULT(LogLevel::INFO, "Incremental training tamamlandı.");

        std::string predicted = nlp.predict_intent(input);
        std::cout << "📌 Tahmin edilen intent: " << predicted << std::endl;

        nlp.save_model("data/models/nlp_model.dat");
        LOG_DEFAULT(LogLevel::INFO, "Model kaydedildi.");
    }

    std::cout << "✅ Eğitim oturumu sona erdi." << std::endl;
    return 0;
}
