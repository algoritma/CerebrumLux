#include "QTableWorker.h"
#include "../DataTypes.h" // YENİ EKLENDİ: QTableDisplayData tanımı için
#include <QRegularExpression>
#include <algorithm> // std::min için
#include <vector> // std::vector için

namespace CerebrumLux {

QTableWorker::QTableWorker(LearningModule& lmRef, QObject *parent)
    : QObject(parent),
      learningModule(lmRef)
{
    LOG_DEFAULT(LogLevel::INFO, "QTableWorker: Initialized.");
}

QTableWorker::~QTableWorker() {
    LOG_DEFAULT(LogLevel::INFO, "QTableWorker: Destructor called.");
}

QTableDisplayData QTableWorker::createDisplayData(const CerebrumLux::SwarmVectorDB::EmbeddingStateKey& stateKey,
                                                const std::map<CerebrumLux::AIAction, float>& actionQValues) {
    QTableDisplayData data;
    data.stateKey = QString::fromStdString(stateKey);
    for (const auto& pair : actionQValues) {
        data.actionQValues[QString::fromStdString(CerebrumLux::to_string(pair.first))] = pair.second;
    }
    return data;
}

std::vector<CerebrumLux::SwarmVectorDB::EmbeddingStateKey> QTableWorker::filterStates(
    const std::vector<CerebrumLux::SwarmVectorDB::EmbeddingStateKey>& sourceStateKeys,
    const QString& filterText) {
    
    std::vector<CerebrumLux::SwarmVectorDB::EmbeddingStateKey> filtered_state_keys;
    QRegularExpression regex(filterText, QRegularExpression::CaseInsensitiveOption);

    for (const auto& state_key_std_str : sourceStateKeys) {
        QString stateKey = QString::fromStdString(state_key_std_str);
        if (filterText.isEmpty() || stateKey.contains(regex)) {
            filtered_state_keys.push_back(state_key_std_str);
        }
    }
    return filtered_state_keys;
}

void QTableWorker::fetchQTableContent(const QString& filterText, const QString& currentSelectionStateKey) {
    try {
        LOG_DEFAULT(LogLevel::DEBUG, "QTableWorker: Q-Table içeriği çekme işlemi başlatıldı (Worker Thread). Filter: '" << filterText.toStdString() << "'");

        const CerebrumLux::SwarmVectorDB::SparseQTable& q_table_ref = learningModule.getQTable();

        if (q_table_ref.q_values.empty()) {
            LOG_DEFAULT(LogLevel::WARNING, "QTableWorker: LearningModule'den alınan SparseQTable boş görünüyor. Q-Table'da hiç veri yok.");
            // Boş listeleri emit et ve erken çık
            emit qTableContentFetched({}, {}, currentSelectionStateKey);
            return; 
        }
        // LOG_DEFAULT(LogLevel::DEBUG, "QTableWorker: SparseQTable'da bulunan toplam durum anahtarı sayısı: " << q_table_ref.q_values.size());

        // YENİ EKLENDİ: all_q_state_keys oluşturulduktan sonra boyutunu logla
        std::vector<CerebrumLux::SwarmVectorDB::EmbeddingStateKey> all_q_state_keys;
        for (const auto& pair : q_table_ref.q_values) {
            all_q_state_keys.push_back(pair.first);
        }

        std::vector<CerebrumLux::SwarmVectorDB::EmbeddingStateKey> filtered_state_keys = filterStates(all_q_state_keys, filterText);
        // YENİ EKLENDİ: filterStates sonrası boyutunu logla
         // LOG_DEFAULT(LogLevel::DEBUG, "QTableWorker: Filtrelenmiş durum anahtarı sayısı: " << filtered_state_keys.size());

        std::map<QString, QTableDisplayData> displayed_details;
        for (const auto& state_key_std_str : filtered_state_keys) {
            auto it = q_table_ref.q_values.find(state_key_std_str);
            if (it != q_table_ref.q_values.end()) {
                displayed_details[QString::fromStdString(state_key_std_str)] = createDisplayData(state_key_std_str, it->second);
            }
        }
        // YENİ EKLENDİ: displayed_details boyutu logla
        // LOG_DEFAULT(LogLevel::DEBUG, "QTableWorker: Görüntülenmek üzere hazırlanan detaylı durum sayısı: " << displayed_details.size());

        emit qTableContentFetched(all_q_state_keys, displayed_details, currentSelectionStateKey);
        LOG_DEFAULT(LogLevel::DEBUG, "QTableWorker: Q-Table içeriği başarıyla çekildi (Worker Thread). Toplam filtrelenmiş: " << filtered_state_keys.size());

    } catch (const std::exception& e) {
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "QTableWorker: Q-Table çekme sırasında hata: " << e.what());
        emit workerError(QString("Q-Table çekme sırasında hata: %1").arg(e.what()));
    } catch (...) {
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "QTableWorker: Q-Table çekme sırasında bilinmeyen hata.");
        emit workerError("Q-Table çekme sırasında bilinmeyen hata.");
    }
}

} // namespace CerebrumLux