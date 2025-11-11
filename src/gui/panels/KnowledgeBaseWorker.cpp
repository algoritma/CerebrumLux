#include "KnowledgeBaseWorker.h"
#include "KnowledgeBasePanel.h" // KnowledgeCapsuleDisplayData tanımı için
#include <QDateTime>
#include <QSet>
#include <QRegularExpression>
#include <chrono>
#include <algorithm> // std::remove_if için
#include <vector> // std::vector için

namespace CerebrumLux {

KnowledgeBaseWorker::KnowledgeBaseWorker(KnowledgeBase& kbRef, QObject *parent)
    : QObject(parent),
      knowledgeBase(kbRef)
{
    LOG_DEFAULT(LogLevel::INFO, "KnowledgeBaseWorker: Initialized.");
}

KnowledgeBaseWorker::~KnowledgeBaseWorker() {
    LOG_DEFAULT(LogLevel::INFO, "KnowledgeBaseWorker: Destructor called.");
}

// KnowledgeCapsuleDisplayData struct'ı KnowledgeBasePanel.h'de tanımlandığı için burada tekrar tanımlanmamalı.
KnowledgeCapsuleDisplayData KnowledgeBaseWorker::createDisplayData(const Capsule& capsule) {
    KnowledgeCapsuleDisplayData data;
    data.id = QString::fromStdString(capsule.id);
    data.topic = QString::fromStdString(capsule.topic);
    data.source = QString::fromStdString(capsule.source);
    data.summary = QString::fromStdString(capsule.plain_text_summary);
    data.fullContent = QString::fromStdString(capsule.content);
    data.cryptofigBlob = QString::fromStdString(capsule.cryptofig_blob_base64);
    data.confidence = capsule.confidence;
    data.code_file_path = QString::fromStdString(capsule.code_file_path);
    data.embedding = capsule.embedding;
    data.timestamp_utc = capsule.timestamp_utc; // DÜZELTİLDİ: timestamp_utc ataması eklendi.
    return data;
}

std::vector<Capsule> KnowledgeBaseWorker::filterCapsules(const std::vector<Capsule>& sourceCapsules,
                                                      const QString& filterText,
                                                      const QString& topicFilter,
                                                      const QDate& startDate,
                                                      const QDate& endDate,
                                                      const QString& specialFilter) {
    std::vector<Capsule> filtered_capsules;
    QRegularExpression regex(filterText, QRegularExpression::CaseInsensitiveOption);

    for (const auto& capsule : sourceCapsules) {
        QString capsuleId = QString::fromStdString(capsule.id);
        QString topic = QString::fromStdString(capsule.topic);
        QString capsuleSource = QString::fromStdString(capsule.source);
        QString capsuleSummary = QString::fromStdString(capsule.plain_text_summary);
        QString codeFilePath = QString::fromStdString(capsule.code_file_path);

        // Zaman damgası dönüşümü
        auto epoch_nanos = capsule.timestamp_utc.time_since_epoch();
        auto epoch_secs = std::chrono::duration_cast<std::chrono::seconds>(epoch_nanos);
        QDateTime dt = QDateTime::fromSecsSinceEpoch(epoch_secs.count()); 
        QDate capsuleDate = dt.date();

        // Özel filtre: yalnızca CodeDevelopment kapsülleri
        if (specialFilter == "Sadece Code Development" && topic != "CodeDevelopment") {
            continue;
        }

        bool matchesSearch = (filterText.isEmpty() ||
                              capsuleId.contains(regex) ||
                              topic.contains(regex) ||
                              capsuleSource.contains(regex) ||
                              capsuleSummary.contains(regex) ||
                              codeFilePath.contains(regex));

        bool matchesTopic = (topicFilter == "Tümü" || topic.compare(topicFilter, Qt::CaseInsensitive) == 0);
        bool matchesStartDate = (!startDate.isValid() || capsuleDate >= startDate);
        bool matchesEndDate = (!endDate.isValid() || capsuleDate <= endDate);

        if (matchesSearch && matchesTopic && matchesStartDate && matchesEndDate) {
            filtered_capsules.push_back(capsule);
        }
    }
    return filtered_capsules;
}

void KnowledgeBaseWorker::fetchAllCapsulesAndDetails(const QString& filterText,
                                                     const QString& topicFilter,
                                                     const QDate& startDate,
                                                     const QDate& endDate,
                                                     const QString& specialFilter,
                                                     const QString& selectedCapsuleId) { // DÜZELTİLDİ: selectedCapsuleId eklendi.
    try {
        LOG_DEFAULT(LogLevel::DEBUG, "KnowledgeBaseWorker: Kapsülleri ve detayları çekme işlemi başlatıldı (Worker Thread).");

        std::vector<Capsule> all_capsules = knowledgeBase.get_all_capsules();

        std::vector<Capsule> filtered_capsules = filterCapsules(all_capsules, filterText, topicFilter, startDate, endDate, specialFilter);

        QSet<QString> unique_topics;
        for (const auto& capsule : all_capsules) {
            unique_topics.insert(QString::fromStdString(capsule.topic));
        }

        std::map<QString, KnowledgeCapsuleDisplayData> displayed_details;
        for (const auto& capsule : filtered_capsules) {
            displayed_details[QString::fromStdString(capsule.id)] = createDisplayData(capsule);
        }

        // DÜZELTİLDİ: allCapsulesFetched sinyaline selectedCapsuleId eklendi.
        emit allCapsulesFetched(filtered_capsules, displayed_details, unique_topics, selectedCapsuleId);
        LOG_DEFAULT(LogLevel::DEBUG, "KnowledgeBaseWorker: Kapsüller ve detaylar başarıyla çekildi (Worker Thread). Toplam filtrelenmiş: " << filtered_capsules.size());

    } catch (const std::exception& e) {
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "KnowledgeBaseWorker: Kapsül çekme sırasında hata: " << e.what());
        emit workerError(QString("Kapsül çekme sırasında hata: %1").arg(e.what()));
    } catch (...) {
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "KnowledgeBaseWorker: Kapsül çekme sırasında bilinmeyen hata.");
        emit workerError("Kapsül çekme sırasında bilinmeyen hata.");
    }
}

void KnowledgeBaseWorker::fetchRelatedCapsules(const std::string& current_capsule_id, const std::vector<float>& current_capsule_embedding) {
    try {
        LOG_DEFAULT(LogLevel::DEBUG, "KnowledgeBaseWorker: İlgili kapsülleri çekme işlemi başlatıldı (Worker Thread). ID: " << current_capsule_id);

        std::vector<Capsule> related_capsules;
        if (!current_capsule_embedding.empty()) {
            related_capsules = knowledgeBase.semantic_search(current_capsule_embedding, 5);
        } else {
            LOG_DEFAULT(LogLevel::WARNING, "KnowledgeBaseWorker: current_capsule_embedding boş, ilgili kapsüller aranmadi. ID: " << current_capsule_id);
        }

        related_capsules.erase(std::remove_if(related_capsules.begin(), related_capsules.end(),
                                               [&](const Capsule& c) { return c.id == current_capsule_id; }),
                                 related_capsules.end());

        emit relatedCapsulesFetched(related_capsules, current_capsule_id);
        LOG_DEFAULT(LogLevel::DEBUG, "KnowledgeBaseWorker: İlgili kapsüller başarıyla çekildi (Worker Thread). Toplam: " << related_capsules.size());

    } catch (const std::exception& e) {
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "KnowledgeBaseWorker: İlgili kapsülleri çekme sırasında hata: " << e.what());
        emit workerError(QString("İlgili kapsülleri çekme sırasında hata: %1").arg(e.what()));
    } catch (...) {
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "KnowledgeBaseWorker: İlgili kapsülleri çekme sırasında bilinmeyen hata.");
        emit workerError("İlgili kapsülleri çekme sırasında bilinmeyen hata.");
    }
}

} // namespace CerebrumLux