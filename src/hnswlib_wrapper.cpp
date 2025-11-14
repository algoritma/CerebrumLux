#include "hnswlib_wrapper.h"
#include "core/logger.h"
#include <stdexcept>
#include <filesystem>
#include <algorithm> // std::reverse için
#include <queue> // std::priority_queue için

namespace CerebrumLux {
namespace HNSW {

namespace fs = std::filesystem;



HNSWIndex::HNSWIndex(int dim, size_t max_elements)
    : dim_(dim), max_elements_(max_elements) {
    LOG_DEFAULT(LogLevel::INFO, "HNSWIndex Kurucusu: Baslatiliyor. Boyut: " << dim << ", Max Eleman: " << max_elements);
    
    // Çökmeye neden olan özel L2Space yerine doğrudan hnswlib'in kendi L2Space'i kullanılır.
    space_ = std::make_unique<hnswlib::L2Space>(dim);

    // app_alg_ nesnesi artık kurucuda değil, load_index veya create_new_index içinde oluşturulur.
    // Bu, gereksiz nesne oluşturmayı önler ve mantığı basitleştirir.
    app_alg_ = nullptr; 
}

HNSWIndex::~HNSWIndex() {
    LOG_DEFAULT(LogLevel::INFO, "HNSWIndex Yikicisi: Cagri yapiliyor.");
    // unique_ptr'lar otomatik olarak temizlenecektir.
    LOG_DEFAULT(LogLevel::INFO, "HNSWIndex Yikicisi: Tamamlandi.");
}

void HNSWIndex::create_new_index() {
    LOG_DEFAULT(LogLevel::INFO, "HNSWIndex::create_new_index(): Yeni (boş) bir HNSW dizini oluşturuluyor.");
    space_ = std::make_unique<hnswlib::L2Space>(dim_);
    app_alg_ = std::make_unique<hnswlib::HierarchicalNSW<float>>(space_.get(), max_elements_);
}

bool HNSWIndex::load_index(const std::string& path) {
    LOG_DEFAULT(LogLevel::INFO, "HNSWIndex::load_index(): Dizin diskten yükleniyor. Yol: " << path);
    if (fs::exists(path)) {
        try {
            // Önce mevcut indeksi temizle
            app_alg_.reset(nullptr);
            space_.reset(nullptr);

            LOG_DEFAULT(LogLevel::TRACE, "HNSWIndex::load_index(): Mevcut dizin dosyası bulundu, yükleniyor...");
            space_ = std::make_unique<hnswlib::L2Space>(dim_);
            app_alg_ = std::make_unique<hnswlib::HierarchicalNSW<float>>(space_.get(), path);
            LOG_DEFAULT(LogLevel::INFO, "HNSWIndex::load_index(): Dizin başarıyla yüklendi. Mevcut eleman sayısı: " << app_alg_->cur_element_count);
            return true;
        } catch (const std::exception& e) {
            LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "HNSWIndex::load_index(): Dizin yükleme hatası: " << e.what());
            return false;
        }
    } else {
        LOG_DEFAULT(LogLevel::WARNING, "HNSWIndex::load_index(): Dizin dosyası bulunamadı: " << path);
        return false;
    }
}

bool HNSWIndex::save_index(const std::string& path) {
    LOG_DEFAULT(LogLevel::INFO, "HNSWIndex::save_index(): Dizin kaydediliyor. Yol: " << path);
    if (!app_alg_) {
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "HNSWIndex::save_index(): HNSW indeksi başlatılmamış. Kaydetme başarısız.");
        return false;
    }
    try {
        app_alg_->saveIndex(path);
        LOG_DEFAULT(LogLevel::INFO, "HNSWIndex::save_index(): Dizin basariyla kaydedildi.");
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "HNSWIndex::save_index(): Dizin kaydedilirken hata olustu: " << e.what());
        return false;
    }
}

void HNSWIndex::add_item(const std::vector<float>& features, hnswlib::labeltype label) {
    if (!app_alg_) {
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "HNSWIndex::add_item(): HNSW indeksi başlatılmamış. Eleman eklenemedi.");
        return;
    }
    if (features.size() != dim_) {
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "HNSWIndex::add_item(): Vektör boyutu yanlis. Beklenen: " << dim_ << ", Gelen: " << features.size());
        return;
    }
    app_alg_->addPoint(features.data(), label);
    LOG_DEFAULT(LogLevel::TRACE, "HNSWIndex::add_item(): Eleman eklendi. Etiket: " << label << ", Index eleman sayisi: " << app_alg_->cur_element_count);
}

std::vector<hnswlib::labeltype> HNSWIndex::search_knn(const std::vector<float>& query, int k) const {
    std::vector<hnswlib::labeltype> result_labels;
    if (!app_alg_) {
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "HNSWIndex::search_knn(): HNSW indeksi başlatılmamış. Arama yapilamadi.");
        return result_labels;
    }
    if (query.size() != dim_) {
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "HNSWIndex::search_knn(): Sorgu vektör boyutu yanlis. Beklenen: " << dim_ << ", Gelen: " << query.size());
        return result_labels;
    }

    std::priority_queue<std::pair<float, hnswlib::labeltype>> result_pq = app_alg_->searchKnn(query.data(), k);

    while (!result_pq.empty()) {
        result_labels.push_back(result_pq.top().second);
        result_pq.pop();
    }
    std::reverse(result_labels.begin(), result_labels.end()); // En yakından uzağa sırala
    LOG_DEFAULT(LogLevel::TRACE, "HNSWIndex::search_knn(): Arama tamamlandi. Bulunan eleman sayisi: " << result_labels.size());
    return result_labels;
}

size_t HNSWIndex::get_current_elements() const {
    if (!app_alg_) {
        return 0;
    }
    return app_alg_->cur_element_count;
}

void HNSWIndex::mark_deleted(hnswlib::labeltype label) {
    if (!app_alg_) {
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "HNSWIndex::mark_deleted(): HNSW indeksi başlatılmamış.");
        return;
    }
    app_alg_->markDelete(label);
    LOG_DEFAULT(LogLevel::TRACE, "HNSWIndex::mark_deleted(): Etiket " << label << " silindi olarak işaretlendi.");
}

} // namespace HNSW
} // namespace CerebrumLux