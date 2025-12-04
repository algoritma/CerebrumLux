#ifndef DATATYPES_H
#define DATATYPES_H

#include <QString>
#include <string>
#include <QVector> // QVector için
#include <map>
#include <vector> // std::vector için

namespace CerebrumLux {

// YENİ EKLENEN STRUCT: Chat yanıtını ve ek meta-bilgileri taşımak için
// NaturalLanguageProcessor.h'den buraya taşındı.
struct ChatResponse {
    std::string text;
    std::string reasoning; // Yanıtın nasıl üretildiğine dair açıklama
    std::vector<std::string> suggested_questions; // YENİ: Önerilen takip soruları
    bool needs_clarification = false; // Yanıtın belirsiz olup olmadığı ve kullanıcının onayına ihtiyaç duyup duymadığı
    double latency_ms = 0.0; // Yanıtın üretilme süresi (ms cinsinden)
};

struct SimulationData {
    QString id;
    double value;
    long long timestamp; // Unix epoch saniye olarak
};

// Q-Table detaylarını görüntülemek için yardımcı bir yapı
struct QTableDisplayData {
    QString stateKey;
    std::map<QString, float> actionQValues; // Eylem adı -> Q-Value
};

} // namespace CerebrumLux

#endif // DATATYPES_H