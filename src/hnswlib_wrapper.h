#ifndef HNSWLIB_WRAPPER_H
#define HNSWLIB_WRAPPER_H

#include <string>
#include <vector>
#include <memory>
#include <hnswlib/hnswlib.h> // hnswlib kütüphanesini dahil et
#include <algorithm> // std::copy için
#include <queue> // std::priority_queue için

namespace CerebrumLux {
namespace HNSW {

class HNSWIndex {
public:
    HNSWIndex(int dim, size_t max_elements);
    ~HNSWIndex();

    bool load_index(const std::string& path);
    void create_new_index();
    bool save_index(const std::string& path);
    void add_item(const std::vector<float>& features, hnswlib::labeltype label);
    std::vector<hnswlib::labeltype> search_knn(const std::vector<float>& query, int k) const;
    void mark_deleted(hnswlib::labeltype label); // YENİ: Öğeyi silindi olarak işaretlemek için
    size_t get_current_elements() const;
    int get_dim() const { return dim_; } // YENİ: Dimension getter

private:
    int dim_;
    size_t max_elements_;
    std::unique_ptr<hnswlib::HierarchicalNSW<float>> app_alg_; // hnswlib index nesnesi
    std::unique_ptr<hnswlib::SpaceInterface<float>> space_; // hnswlib'nin modern API'si (v0.7+) ile uyumlu

    HNSWIndex(const HNSWIndex&) = delete;
    HNSWIndex& operator=(const HNSWIndex&) = delete;
};

} // namespace HNSW
} // namespace CerebrumLux

#endif // HNSWLIB_WRAPPER_H