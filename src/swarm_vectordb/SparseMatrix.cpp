#include "SparseMatrix.h"
#include <cmath> // std::sqrt için
#include <numeric> // std::inner_product için

namespace CerebrumLux {
namespace SwarmVectorDB {

// --- SparseQTable Implementasyonu ---

SparseQTable::SparseQTable(int num_states, int num_actions)
    : num_states_(num_states), num_actions_(num_actions), q_table_(num_states, num_actions) {
    LOG_DEFAULT(LogLevel::INFO, "SparseQTable: Constructor called. States: " << num_states_ << ", Actions: " << num_actions_);
}

void SparseQTable::initialize() {
    std::lock_guard<std::mutex> lock(mutex_); // Thread güvenliği
    q_table_.setZero(); // Tüm değerleri sıfırla
    LOG_DEFAULT(LogLevel::INFO, "SparseQTable: Başlatıldı. Tüm değerler sıfırlandı.");
}

// CryptofigVector embedding'inden bir durum indeksi türetir (basit hash)
int SparseQTable::get_state_index(const Eigen::VectorXf& embedding) const {
    // Çok basit bir hash fonksiyonu. Gerçek uygulamada daha sofistike bir yöntem gerekir.
    // Özellikle çok sayıda durum olduğunda çakışmalar olacaktır.
    // Vektörün ilk birkaç elemanını kullanarak basit bir hash.
    size_t hash_val = 0;
    for (int i = 0; i < std::min((int)embedding.size(), 4); ++i) { // İlk 4 elemanı kullan
        hash_val = (hash_val * 31) + static_cast<size_t>(std::abs(embedding(i) * 1000));
    }
    return static_cast<int>(hash_val % num_states_);
}


// Cosine benzerliğini hesaplar (iki vektör arasında)
float compute_cosine_similarity(const Eigen::VectorXf& v1, const Eigen::VectorXf& v2) {
    if (v1.size() == 0 || v2.size() == 0) return 0.0f;
    if (v1.size() != v2.size()) {
        LOG_DEFAULT(LogLevel::WARNING, "Cosine Similarity: Vektör boyutları uyuşmuyor!");
        return 0.0f;
    }
    float dot_product = v1.dot(v2);
    float norm_v1 = v1.norm();
    float norm_v2 = v2.norm();

    if (norm_v1 == 0.0f || norm_v2 == 0.0f) return 0.0f; // Sıfıra bölme hatasını önle
    return dot_product / (norm_v1 * norm_v2);
}

void SparseQTable::update_q_value(const CryptofigVector& cv, int action, float reward) {
    std::lock_guard<std::mutex> lock(mutex_); // Thread güvenliği
    if (action < 0 || action >= num_actions_) {
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "SparseQTable: Geçersiz aksiyon indisi: " << action);
        return;
    }

    int state_idx = get_state_index(cv.embedding);
    if (state_idx < 0 || state_idx >= num_states_) {
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "SparseQTable: Geçersiz durum indisi: " << state_idx);
        return;
    }

    // Q-tablosunda hücreye erişim ve güncelleme
    // Bu, Q-learning'in basit bir adaptasyonudur.
    // Gerçek bir RL algoritmasında learning_rate, discount_factor vb. olacaktır.
    float learning_rate = 0.1f; // Örnek learning rate
    float current_q = q_table_.coeff(state_idx, action);
    float new_q = current_q + learning_rate * (reward - current_q); // Basit güncelleme
    
    q_table_.coeffRef(state_idx, action) = new_q; // q_table_.row(state_idx).col(action)
    
    // Gelişmiş: Vektör benzerliğine göre Q-değeri güncelle
    // cv.embedding ile mevcut durum vektörlerinin benzerliğini hesaplayıp güncelleyebiliriz.
    // Ancak Eigen::SparseMatrix için bu biraz daha karmaşık bir erişim gerektirir.
    // Şimdilik doğrudan state_idx'e güncelleme yapıldı.
    
    LOG_DEFAULT(LogLevel::TRACE, "SparseQTable: Q-değeri güncellendi. State: " << state_idx << ", Action: " << action << ", Reward: " << reward << ", Yeni Q: " << new_q);
}

float SparseQTable::get_q_value(const CryptofigVector& cv, int action) const {
    std::lock_guard<std::mutex> lock(mutex_); // Thread güvenliği
    if (action < 0 || action >= num_actions_) {
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "SparseQTable: Geçersiz aksiyon indisi: " << action);
        return 0.0f;
    }

    int state_idx = get_state_index(cv.embedding);
    if (state_idx < 0 || state_idx >= num_states_) {
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "SparseQTable: Geçersiz durum indisi: " << state_idx);
        return 0.0f;
    }
    return q_table_.coeff(state_idx, action);
}


} // namespace SwarmVectorDB
} // namespace CerebrumLux