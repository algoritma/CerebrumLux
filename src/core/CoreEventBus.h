#ifndef CORE_EVENT_BUS_H
#define CORE_EVENT_BUS_H

#include <QObject>
#include <QString>
#include <QVariant> // Generic data handling for learningUpdate

namespace CerebrumLux {

class CoreEventBus : public QObject
{
    Q_OBJECT
public:
    explicit CoreEventBus(QObject *parent = nullptr);

    // Global singleton instance for easy access
    static CoreEventBus& getInstance();

signals:
    // Emitted when a response is ready from the core logic
    void responseReady(const QString& text);
    // Emitted when a learning metric is updated
    void learningUpdate(const QString& metric, float value);
    // YENİ: TutorBrokerAdapter'dan gelen mesajları iletmek için sinyal
    void tutorBrokerMessage(const QString& from, const QString& type, const QString& payload);

public slots:
    // Optionally, slots could be added here to receive events from core logic
    // if core logic objects are QObjects. For simplicity, direct calls to signals
    // from non-QObjects might be used.
};

} // namespace CerebrumLux

#endif // CORE_EVENT_BUS_H