#ifndef QTABLE_WORKER_H
#define QTABLE_WORKER_H

#include <QObject>
#include <QString>
#include <vector>
#include <map>
#include <QDate> // QDate için (Eğer worker'da filtreleme yapılıyorsa)
#include <QSet> // QSet için (Eğer worker'da benzersiz konular toplanıyorsa)
#include <chrono> // std::chrono::system_clock::time_point için (Eğer worker'da Capsule ile çalışılıyorsa)

#include "../../core/logger.h"
#include "../../learning/LearningModule.h"
#include "../../swarm_vectordb/DataModels.h" // SparseQTable ve EmbeddingStateKey için
#include "../../core/enums.h" // AIAction için
#include "../../core/utils.h" // action_to_string için
#include "../DataTypes.h" // YENİ EKLENDİ: QTableDisplayData tanımı için

namespace CerebrumLux { 
    // Forward declaration for SparseQTable if needed
    namespace SwarmVectorDB { struct SparseQTable; } 

} // namespace CerebrumLux


namespace CerebrumLux { // Worker sınıfı CerebrumLux namespace'i içinde

class QTableWorker : public QObject
{
    Q_OBJECT
public:
    explicit QTableWorker(LearningModule& lmRef, QObject *parent = nullptr);
    ~QTableWorker();

public slots:
    void fetchQTableContent(const QString& filterText, const QString& currentSelectionStateKey);

signals:
    void qTableContentFetched(const std::vector<CerebrumLux::SwarmVectorDB::EmbeddingStateKey>& all_q_state_keys,
                              const std::map<QString, QTableDisplayData>& displayed_q_table_details,
                              const QString& restoreSelectionStateKey);
    void workerError(const QString& error_message);

private:
    LearningModule& learningModule;

    QTableDisplayData createDisplayData(const CerebrumLux::SwarmVectorDB::EmbeddingStateKey& stateKey,
                                        const std::map<CerebrumLux::AIAction, float>& actionQValues);

    std::vector<CerebrumLux::SwarmVectorDB::EmbeddingStateKey> filterStates(
        const std::vector<CerebrumLux::SwarmVectorDB::EmbeddingStateKey>& sourceStateKeys,
        const QString& filterText);
};

} // namespace CerebrumLux

#endif // QTABLE_WORKER_H