#ifndef DATATYPES_H
#define DATATYPES_H

#include <QString>
#include <QVector> // QVector için

namespace CerebrumLux { // SimulationData struct'ı bu namespace içine alınacak

struct SimulationData {
    QString id;
    double value;
    long long timestamp; // Unix epoch saniye olarak
};

} // namespace CerebrumLux

#endif // DATATYPES_H