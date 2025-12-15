#include "IntentLearnerQtBridge.h"
#include "brain/intent_learner.h"

namespace CerebrumLux {

IntentLearnerQtBridge::IntentLearnerQtBridge(
    IntentLearner& core, QObject* parent)
    : QObject(parent), core_(core) {}

}