// IntentLearnerQtBridge.h
#pragma once
#include <QObject>

namespace CerebrumLux {

class IntentLearner; // forward declaration (ZORUNLU)

 class IntentLearnerQtBridge : public QObject {
     Q_OBJECT
 public:
     explicit IntentLearnerQtBridge(IntentLearner& core, QObject* parent=nullptr);

 signals:
     void intentLearned(int intentId);

 private:
     IntentLearner& core_;
 };
} // namespace CerebrumLux
