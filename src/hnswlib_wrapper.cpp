#include "hnswlib_wrapper.h"
#include "core/logger.h"
#include <stdexcept>
#include <filesystem>
#include <algorithm> // std::reverse için
#include <queue> // std::priority_queue için

namespace CerebrumLux {
namespace HNSW {

namespace fs = std::filesystem;



HNSWIndex::HNSWIndex(int dim, size_t max_elements, const std::string& index_path)
    : dim_(dim), max_elements_(max_elements), index_path_(index_path) {
    LOG_DEFAULT(LogLevel::INFO, "HNSWIndex Kurucusu: Baslatiliyor. Boyut: " << dim << ", Max Eleman: " << max_elements);
    
    // Çökmeye neden olan özel L2Space yerine doğrudan hnswlib'in kendi L2Space'i kullanılır.
    space_ = std::make_unique<hnswlib::L2Space>(dim);

    // alg_hnsw_ nesnesi artık kurucuda değil, load_or_create_index içinde oluşturulur.
    // Bu, gereksiz nesne oluşturmayı önler ve mantığı basitleştirir.
    alg_hnsw_ = nullptr; 
}

HNSWIndex::~HNSWIndex() {
    LOG_DEFAULT(LogLevel::INFO, "HNSWIndex Yikicisi: Cagri yapiliyor.");
    LOG_DEFAULT(LogLevel::INFO, "HNSWIndex Yikicisi: Tamamlandi.");
}

bool HNSWIndex::load_or_create_index(const std::string& path) {
    index_path_ = path;
    LOG_DEFAULT(LogLevel::INFO, "HNSWIndex::load_or_create_index(): Dizin yükleniyor veya oluşturuluyor. Yol: " << index_path_);

    // space_ nesnesinin geçerli olduğundan emin ol (kurucuda oluşturulmuş olmalı)
    if (!space_) {
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "HNSWIndex::load_or_create_index(): space_ başlatılmamış. Bu kritik bir hatadır.");
        return false;
    }

    if (fs::exists(index_path_)) {
        try {
            alg_hnsw_ = std::make_unique<hnswlib::HierarchicalNSW<float>>(space_.get(), index_path_);
            LOG_DEFAULT(LogLevel::INFO, "HNSWIndex::load_or_create_index(): Dizin basariyla yüklendi. Eleman sayisi: " << alg_hnsw_->cur_element_count);
            
            // Eğer dizin yüklendi ancak boşsa (hiç eleman içermiyorsa),
            // bu, HNSW kütüphanesinin kapasite ayarlarını düzgün yapmamasına neden olabilir.
            // Bu durumda, yeni bir dizin oluşturmak daha güvenlidir.
            if (alg_hnsw_->cur_element_count == 0) {
                LOG_DEFAULT(LogLevel::WARNING, "HNSWIndex::load_or_create_index(): Yüklenen dizin boş. Yeni bir dizin olusturuluyor.");
                alg_hnsw_ = std::make_unique<hnswlib::HierarchicalNSW<float>>(space_.get(), max_elements_, 16, 200);
            }
            return true;
        } catch (const std::exception& e) {
            LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "HNSWIndex::load_or_create_index(): Dizin yüklenirken hata olustu: " << e.what() << ". Yeni dizin olusturuluyor.");
            alg_hnsw_ = std::make_unique<hnswlib::HierarchicalNSW<float>>(space_.get(), max_elements_, 16, 200);
            return true;
        }
    } else {
        LOG_DEFAULT(LogLevel::INFO, "HNSWIndex::load_or_create_index(): Dizin bulunamadi. Yeni dizin olusturuluyor.");
        alg_hnsw_ = std::make_unique<hnswlib::HierarchicalNSW<float>>(space_.get(), max_elements_, 16, 200);
        return true;
    }
}

bool HNSWIndex::save_index(const std::string& path) const {
    LOG_DEFAULT(LogLevel::INFO, "HNSWIndex::save_index(): Dizin kaydediliyor. Yol: " << path);
    if (!alg_hnsw_) {
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "HNSWIndex::save_index(): HNSW indeksi başlatılmamış. Kaydetme başarısız.");
        return false;
    }
    try {
        alg_hnsw_->saveIndex(path);
        LOG_DEFAULT(LogLevel::INFO, "HNSWIndex::save_index(): Dizin basariyla kaydedildi.");
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "HNSWIndex::save_index(): Dizin kaydedilirken hata olustu: " << e.what());
        return false;
    }
}

void HNSWIndex::add_item(const std::vector<float>& features, hnswlib::labeltype label) {
    if (!alg_hnsw_) {
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "HNSWIndex::add_item(): HNSW indeksi başlatılmamış. Eleman eklenemedi.");
        return;
    }
    if (features.size() != dim_) {
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "HNSWIndex::add_item(): Vektör boyutu yanlis. Beklenen: " << dim_ << ", Gelen: " << features.size());
        return;
    }
    alg_hnsw_->addPoint(features.data(), label);
    LOG_DEFAULT(LogLevel::TRACE, "HNSWIndex::add_item(): Eleman eklendi. Etiket: " << label << ", Index eleman sayisi: " << alg_hnsw_->cur_element_count);
}

std::vector<hnswlib::labeltype> HNSWIndex::search_knn(const std::vector<float>& query, int k) {
    std::vector<hnswlib::labeltype> result_labels;
    if (!alg_hnsw_) {
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "HNSWIndex::search_knn(): HNSW indeksi başlatılmamış. Arama yapilamadi.");
        return result_labels;
    }
    if (query.size() != dim_) {
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "HNSWIndex::search_knn(): Sorgu vektör boyutu yanlis. Beklenen: " << dim_ << ", Gelen: " << query.size());
        return result_labels;
    }

    std::priority_queue<std::pair<float, hnswlib::labeltype>> result_pq = alg_hnsw_->searchKnn(query.data(), k);

    while (!result_pq.empty()) {
        result_labels.push_back(result_pq.top().second);
        result_pq.pop();
    }
    std::reverse(result_labels.begin(), result_labels.end()); // En yakından uzağa sırala
    LOG_DEFAULT(LogLevel::TRACE, "HNSWIndex::search_knn(): Arama tamamlandi. Bulunan eleman sayisi: " << result_labels.size());
    return result_labels;
}

size_t HNSWIndex::get_current_elements() const {
    if (!alg_hnsw_) {
        return 0;
    }
    return alg_hnsw_->cur_element_count;
}

} // namespace HNSW
} // namespace CerebrumLux