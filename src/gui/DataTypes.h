#ifndef DATATYPES_H
#define DATATYPES_H

#include <QString>
#include <QVector> // QVector için
#include <map>

namespace CerebrumLux {

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