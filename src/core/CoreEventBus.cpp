#include "CoreEventBus.h"
#include "logger.h" // For logging purposes

namespace CerebrumLux {

CoreEventBus::CoreEventBus(QObject *parent)
    : QObject(parent)
{
    LOG_DEFAULT(LogLevel::INFO, "CoreEventBus: Initialized.");
}

CoreEventBus& CoreEventBus::getInstance() {
    static CoreEventBus instance; // Singleton instance
    return instance;
}

} // namespace CerebrumLux