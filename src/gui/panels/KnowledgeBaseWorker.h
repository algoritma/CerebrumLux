#ifndef KNOWLEDGEBASE_WORKER_H
#define KNOWLEDGEBASE_WORKER_H

#include <QObject>
#include <QVector>
#include <QString>
#include <vector>
#include <QDate> // QDate için
#include <QSet> // QSet için
#include <map> // std::map için
#include <chrono> // std::chrono::system_clock::time_point için

#include "../../learning/KnowledgeBase.h"
#include "../../learning/Capsule.h"
#include "../../communication/natural_language_processor.h" // generate_text_embedding için
#include "../../core/logger.h"
#include "../../core/enums.h"

namespace CerebrumLux {

// KnowledgeBasePanel'in detay görünümü için veri yapısı (forward declaration)
struct KnowledgeCapsuleDisplayData; 

class KnowledgeBaseWorker : public QObject
{
    Q_OBJECT
public:
    explicit KnowledgeBaseWorker(KnowledgeBase& kbRef, QObject *parent = nullptr);
    ~KnowledgeBaseWorker();

public slots:
    // DÜZELTİLDİ: fetchAllCapsulesAndDetails slotu 6 argüman alacak şekilde güncellendi.
    void fetchAllCapsulesAndDetails(const QString& filterText,
                                    const QString& topicFilter,
                                    const QDate& startDate,
                                    const QDate& endDate,
                                    const QString& specialFilter,
                                    const QString& selectedCapsuleId); // Seçimi geri yüklemek için ID
    void fetchRelatedCapsules(const std::string& current_capsule_id, const std::vector<float>& current_capsule_embedding);

signals:
    // DÜZELTİLDİ: allCapsulesFetched sinyaline restoreSelectionId eklendi.
    void allCapsulesFetched(const std::vector<Capsule>& all_capsules,
                            const std::map<QString, KnowledgeCapsuleDisplayData>& displayed_details,
                            const QSet<QString>& unique_topics,
                            const QString& restoreSelectionId); // Seçimi geri yüklemek için ID
    void relatedCapsulesFetched(const std::vector<Capsule>& related_capsules, const std::string& for_capsule_id);
    void workerError(const QString& error_message);

private:
    KnowledgeBase& knowledgeBase;

    KnowledgeCapsuleDisplayData createDisplayData(const Capsule& capsule);

    std::vector<Capsule> filterCapsules(const std::vector<Capsule>& sourceCapsules,
                                        const QString& filterText,
                                        const QString& topicFilter,
                                        const QDate& startDate,
                                        const QDate& endDate,
                                        const QString& specialFilter);
};

} // namespace CerebrumLux

#endif // KNOWLEDGEBASE_WORKER_H