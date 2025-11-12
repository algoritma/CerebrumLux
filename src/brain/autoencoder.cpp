#include "autoencoder.h" // Kendi başlık dosyasını dahil et
#include "../core/logger.h" // LOG makrosu için
#include "../core/utils.h" // SafeRNG ve diğer yardımcı fonksiyonlar için
#include <cmath>     // std::exp, std::sqrt için
#include <algorithm> // std::min/max için
#include <cstdio>    // fopen, fwrite, fread, fclose için
#include <limits>    // std::numeric_limits için
#include <iostream>  // std::cerr için
#include <sstream>   // std::stringstream için


namespace CerebrumLux { // TÜM İMPLEMENTASYON BU NAMESPACE İÇİNDE OLACAK

// Statik const int üyelerinin dış tanımları
const int CryptofigAutoencoder::INPUT_DIM;
const int CryptofigAutoencoder::LATENT_DIM;

// Default constructor calls the initialization method
CryptofigAutoencoder::CryptofigAutoencoder() {
    initialize_weights(); // Call the unified initialization method
}

void CryptofigAutoencoder::initialize_weights() { // Unified initialization method
    std::uniform_real_distribution<float> dist(-0.5f, 0.5f);

    // Resize and initialize member vectors
    encoder_weights.resize(INPUT_DIM * LATENT_DIM);
    encoder_bias.resize(LATENT_DIM);
    decoder_weights.resize(LATENT_DIM * INPUT_DIM);
    decoder_bias.resize(INPUT_DIM);

    // Use the declared member variables, not _1 suffixed ones
    for (size_t i = 0; i < encoder_weights.size(); ++i) encoder_weights[i] = dist(SafeRNG::getInstance().get_generator());
    for (size_t i = 0; i < encoder_bias.size(); ++i) encoder_bias[i] = dist(SafeRNG::getInstance().get_generator());
    for (size_t i = 0; i < decoder_weights.size(); ++i) decoder_weights[i] = dist(SafeRNG::getInstance().get_generator());
    for (size_t i = 0; i < decoder_bias.size(); ++i) decoder_bias[i] = dist(SafeRNG::getInstance().get_generator());

    LOG_DEFAULT(LogLevel::INFO, "CryptofigAutoencoder: Agirliklar rastgele baslatildi.");
}


float CryptofigAutoencoder::sigmoid(float x) const {
    return 1.0f / (1.0f + std::exp(-x));
}

float CryptofigAutoencoder::sigmoid_derivative(float x) const {
    float s = sigmoid(x);
    return s * (1.0f - s);
}

std::vector<float> CryptofigAutoencoder::encode(const std::vector<float>& input_features) const {
    if (input_features.size() != INPUT_DIM) {
        LOG_DEFAULT(LogLevel::ERR_CRITICAL, "CryptofigAutoencoder::encode: Girdi boyutu uyuşmuyor! Beklenen: " << INPUT_DIM << ", Gelen: " << input_features.size() << "\n");
        return std::vector<float>(LATENT_DIM, 0.0f); // Hata durumunda sıfır vektör döndür
    }

    std::vector<float> latent_features(LATENT_DIM, 0.0f);
    for (int j = 0; j < LATENT_DIM; ++j) {
        float sum = 0.0f;
        for (int i = 0; i < INPUT_DIM; ++i) { // Corrected loop variable from `i` to `i`
            sum += input_features[i] * encoder_weights[i * LATENT_DIM + j]; // Use correct member name
        }
        latent_features[j] = sigmoid(sum + encoder_bias[j]);
    }
    return latent_features;
}

std::vector<float> CryptofigAutoencoder::decode(const std::vector<float>& latent_features) const {
    if (latent_features.size() != LATENT_DIM) {
        LOG_DEFAULT(LogLevel::ERR_CRITICAL, "CryptofigAutoencoder::decode: Gizil boyut uyuşmuyor! Beklenen: " << LATENT_DIM << ", Gelen: " << latent_features.size() << "\n");
        return std::vector<float>(INPUT_DIM, 0.0f);
    }

    std::vector<float> reconstructed_features(INPUT_DIM, 0.0f);
    for (int j = 0; j < INPUT_DIM; ++j) {
        float sum = 0.0f;
        for (int i = 0; i < LATENT_DIM; ++i) { // Corrected loop variable from `i` to `i`
            sum += latent_features[i] * decoder_weights[i * INPUT_DIM + j]; // Use correct member name
        }
        reconstructed_features[j] = sigmoid(sum + decoder_bias[j]); // Use correct member name
    }
    return reconstructed_features;
}

std::vector<float> CryptofigAutoencoder::reconstruct(const std::vector<float>& input_features) const {
    return decode(encode(input_features));
}

void CryptofigAutoencoder::adjust_weights_on_error(const std::vector<float>& input_features, float learning_rate_ae) {
    if (input_features.size() != INPUT_DIM) {
        LOG_DEFAULT(LogLevel::ERR_CRITICAL, "CryptofigAutoencoder::adjust_weights_on_error: Girdi boyutu uyuşmuyor! Ayarlama yapılamıyor.\n");
        return;
    }

    std::vector<float> latent_features = encode(input_features);
    std::vector<float> reconstructed_features = decode(latent_features);

    float total_error = 0.0f;
    for (int i = 0; i < INPUT_DIM; ++i) {
        float error = input_features[i] - reconstructed_features[i];
        total_error += error * error;
    }
    total_error = std::sqrt(total_error / INPUT_DIM); // RMSE

    if (total_error > 0.1f) { 
        float adjustment_magnitude = learning_rate_ae * total_error; 

        for (int j = 0; j < INPUT_DIM; ++j) {
            float output_error = input_features[j] - reconstructed_features[j];
            decoder_bias[j] += adjustment_magnitude * output_error; // Use correct member name
            decoder_bias[j] = std::min(1.0f, std::max(-1.0f, decoder_bias[j])); // Use correct member name

            for (int i = 0; i < LATENT_DIM; ++i) {
                decoder_weights[i * INPUT_DIM + j] += adjustment_magnitude * output_error * latent_features[i]; // Use correct member name
                decoder_weights[i * INPUT_DIM + j] = std::min(1.0f, std::max(-1.0f, decoder_weights[i * INPUT_DIM + j])); // Use correct member name
            }
        }

        for (int j = 0; j < LATENT_DIM; ++j) {
            float latent_error_signal = 0.0f;
            for (int k = 0; k < INPUT_DIM; ++k) {
                latent_error_signal += (input_features[k] - reconstructed_features[k]) * decoder_weights[j * INPUT_DIM + k]; // Use correct member name
            }
            encoder_bias[j] += adjustment_magnitude * latent_error_signal; // Use correct member name
            encoder_bias[j] = std::min(1.0f, std::max(-1.0f, encoder_bias[j])); // Use correct member name

            for (int i = 0; i < INPUT_DIM; ++i) {
                encoder_weights[i * LATENT_DIM + j] += adjustment_magnitude * latent_error_signal * input_features[i]; // Use correct member name
                encoder_weights[i * LATENT_DIM + j] = std::min(1.0f, std::max(-1.0f, encoder_weights[i * LATENT_DIM + j])); // Use correct member name
            }
        }
        LOG_DEFAULT(LogLevel::DEBUG, "CryptofigAutoencoder: Agirliklar hataya gore ayarlandi. Hata: " << total_error << "\n");
    }
}

void CryptofigAutoencoder::save_weights(const std::string& filename) const {
    FILE* fp = fopen(filename.c_str(), "wb");
    if (!fp) {
        LOG_DEFAULT(LogLevel::ERR_CRITICAL, "Hata: Autoencoder agirlik dosyasi yazilamadi: " << filename << " (errno: " << errno << ")\n");
        return;
    }

    fwrite(&INPUT_DIM, sizeof(int), 1, fp);
    fwrite(&LATENT_DIM, sizeof(int), 1, fp);

    size_t size; // Use correct member names
    size = encoder_weights.size(); fwrite(&size, sizeof(size_t), 1, fp); fwrite(encoder_weights.data(), sizeof(float), size, fp);
    size = encoder_bias.size();    fwrite(&size, sizeof(size_t), 1, fp); fwrite(encoder_bias.data(), sizeof(float), size, fp);
    size = decoder_weights.size(); fwrite(&size, sizeof(size_t), 1, fp); fwrite(decoder_weights.data(), sizeof(float), size, fp);
    size = decoder_bias.size();    fwrite(&size, sizeof(size_t), 1, fp); fwrite(decoder_bias.data(), sizeof(float), size, fp);

    fclose(fp);
    LOG_DEFAULT(LogLevel::INFO, "CryptofigAutoencoder agirliklari kaydedildi: " << filename << "\n");
}

void CryptofigAutoencoder::load_weights(const std::string& filename) {
    FILE* fp = fopen(filename.c_str(), "rb");
    if (!fp) {
        LOG_DEFAULT(LogLevel::WARNING, "Uyari: Autoencoder agirlik dosyasi bulunamadi, rastgele agirliklar kullaniliyor: " << filename << " (errno: " << errno << ")\n");
        initialize_weights(); // initialize_random_weights() yerine initialize_weights() çağrıldı
        return;
    }

    int loaded_input_dim, loaded_latent_dim;
    fread(&loaded_input_dim, sizeof(int), 1, fp);
    fread(&loaded_latent_dim, sizeof(int), 1, fp);

    if (loaded_input_dim != INPUT_DIM || loaded_latent_dim != LATENT_DIM) {
        LOG_DEFAULT(LogLevel::ERR_CRITICAL, "Hata: Autoencoder agirlik dosyasi boyutlari uyuşmuyor! Dosya: " << loaded_input_dim << "x" << loaded_latent_dim << ", Beklenen: " << INPUT_DIM << "x" << LATENT_DIM << ". Rastgele agirliklar kullaniliyor.\n");
        fclose(fp);
        initialize_weights(); // initialize_random_weights() yerine initialize_weights() çağrıldı
        return;
    }

    size_t size;
    fread(&size, sizeof(size_t), 1, fp); encoder_weights.resize(size); fread(encoder_weights.data(), sizeof(float), size, fp); // Use correct member names
    fread(&size, sizeof(size_t), 1, fp); encoder_bias.resize(size);    fread(encoder_bias.data(), sizeof(float), size, fp); // Use correct member names
    fread(&size, sizeof(size_t), 1, fp); decoder_weights.resize(size); fread(decoder_weights.data(), sizeof(float), size, fp); // Use correct member names
    fread(&size, sizeof(size_t), 1, fp); decoder_bias.resize(size);    fread(decoder_bias.data(), sizeof(float), size, fp); // Use correct member names

    fclose(fp);
    LOG_DEFAULT(LogLevel::INFO, "CryptofigAutoencoder agirliklari yuklendi: " << filename << "\n");
}

float CryptofigAutoencoder::calculate_reconstruction_error(const std::vector<float>& original, const std::vector<float>& reconstructed) const {
    if (original.size() != reconstructed.size() || original.empty()) {
        return std::numeric_limits<float>::max();
    }
    float error_sum_sq = 0.0f;
    for (size_t i = 0; i < original.size(); ++i) {
        float diff = original[i] - reconstructed[i];
        error_sum_sq += diff * diff;
    }
    return std::sqrt(error_sum_sq / original.size());
}

} // namespace CerebrumLux
