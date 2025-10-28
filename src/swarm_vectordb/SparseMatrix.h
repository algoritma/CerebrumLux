#ifndef SWARM_VECTORDB_SPARSE_MATRIX_H
#define SWARM_VECTORDB_SPARSE_MATRIX_H

#include <Eigen/Sparse>
#include <Eigen/Dense> // Eigen::VectorXf için
#include <string>
#include <mutex> // Thread güvenliği için

#include "DataModels.h" // CryptofigVector için
#include "../core/logger.h" // CerebrumLux Logger için

namespace CerebrumLux {
namespace SwarmVectorDB {

// Sparse Q-Tablosu
// Tanım: RL Q-tablosu, Eigen::SparseMatrix ile optimize edilir.
// Sürüden gelen vektörler, Q-değerlerini günceller.
class SparseQTable {
public:
    SparseQTable(int num_states, int num_actions);

    // Q-tablosunu başlatır
    void initialize();
    // CryptofigVector ve reward ile Q-değerini günceller
    void update_q_value(const CryptofigVector& cv, int action, float reward);
    // Verilen bir durum ve aksiyon için Q-değerini döndürür
    float get_q_value(const CryptofigVector& cv, int action) const;

private:
    int num_states_;
    int num_actions_;
    Eigen::SparseMatrix<float> q_table_;
    mutable std::mutex mutex_; // Thread güvenliği için (const metotlarda kilitlenebilmesi için mutable eklendi)

    // CryptofigVector embedding'inden bir durum indeksi türetir (basit hash)
    int get_state_index(const Eigen::VectorXf& embedding) const;
};

} // namespace SwarmVectorDB
} // namespace CerebrumLux

#endif // SWARM_VECTORDB_SPARSE_MATRIX_H