// tutor_gui_bridge.cpp
#include "tutor_gui_bridge.h"
#include "core/CoreEventBus.h" // YENİ: CoreEventBus için, düzeltilmiş include yolu

TutorGUIBridge::TutorGUIBridge(QObject* parent) : QObject(parent) {}

void TutorGUIBridge::setBroker(TutorBrokerAdapter* b) {
    broker = b;
    if (!broker) return;
    // register GUI callback to emit CoreEventBus signal
    broker->set_gui_callback([this](const std::string &from, const std::string &type, const std::string &payload){
        CerebrumLux::CoreEventBus::getInstance().tutorBrokerMessage(
            QString::fromStdString(from),
            QString::fromStdString(type),
            QString::fromStdString(payload)
        );
    });
}

void TutorGUIBridge::slot_sendUserInput(const QString &user, const QString &text) {
    if (!broker) return;
    broker->push_user_input(user.toStdString(), text.toStdString());
}