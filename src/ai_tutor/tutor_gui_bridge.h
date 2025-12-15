// tutor_gui_bridge.h
// Provides Qt signals/slots for GUI <-> TutorBrokerAdapter.

#ifndef TUTOR_GUI_BRIDGE_H
#define TUTOR_GUI_BRGE_H

#include <QObject>
#include <QString>
#include "tutor_broker_adapter.h"

class TutorGUIBridge : public QObject {
    Q_OBJECT
public:
    explicit TutorGUIBridge(QObject* parent = nullptr);

    void setBroker(TutorBrokerAdapter* b);

public slots:
    void slot_sendUserInput(const QString &user, const QString &text);

private:
    TutorBrokerAdapter* broker = nullptr;
};

#endif // TUTOR_GUI_BRGE_H
