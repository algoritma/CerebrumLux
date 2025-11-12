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
    HNSWIndex(int dim, size_t max_elements, const std::string& index_path = "");
    ~HNSWIndex();

    bool load_or_create_index(const std::string& path);
    bool save_index(const std::string& path) const;
    void add_item(const std::vector<float>& features, hnswlib::labeltype label);
    std::vector<hnswlib::labeltype> search_knn(const std::vector<float>& query, int k);
    size_t get_current_elements() const;
    int get_dim() const { return dim_; } // YENİ: Dimension getter

private:
    int dim_;
    size_t max_elements_;
    std::unique_ptr<hnswlib::HierarchicalNSW<float>> alg_hnsw_; // hnswlib index nesnesi
    std::string index_path_;
    std::unique_ptr<hnswlib::SpaceInterface<float>> space_; // hnswlib'nin modern API'si (v0.7+) ile uyumlu

    HNSWIndex(const HNSWIndex&) = delete;
    HNSWIndex& operator=(const HNSWIndex&) = delete;
};

} // namespace HNSW
} // namespace CerebrumLux

#endif // HNSWLIB_WRAPPER_H