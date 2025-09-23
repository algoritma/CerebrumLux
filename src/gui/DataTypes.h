#ifndef DATATYPES_H
#define DATATYPES_H

#include <string>
#include <vector>
#include <chrono> // IngestReport için
#include <map>    // IngestReport için

// LearningModule'den gelen enum ve struct'lar
#include "../learning/LearningModule.h" // IngestResult ve IngestReport için

// Simülasyon verileri
struct SimulationData {
    std::string id; 
    int step;
    float value;
};

// Log verileri
struct LogData {
    std::string message;
    int level;
};

// Grafik verileri
struct GraphData {
    float x;
    float y;
};

// Q_DECLARE_METATYPE ile Qt'nin meta-sistemine kaydetme
// Bu, bu türlerin sinyaller ve slotlar arasında veya QVariant içinde kullanılmasını sağlar.
//Q_DECLARE_METATYPE(IngestResult)
//Q_DECLARE_METATYPE(IngestReport)

#endif // DATATYPES_H