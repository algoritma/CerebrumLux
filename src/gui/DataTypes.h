#ifndef DATATYPES_H
#define DATATYPES_H

#include <string>
#include <vector>

// Simülasyon verileri
struct SimulationData {
    int id;    // YENİ: Capsule ile uyumlu olması için ID eklendi
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

#endif // DATATYPES_H