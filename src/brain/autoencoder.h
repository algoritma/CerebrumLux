#ifndef CEREBRUM_LUX_AUTOENCODER_H
#define CEREBRUM_LUX_AUTOENCODER_H

#include <vector>
#include <string>  // For std::string
// #include "../core/enums.h" // LogLevel için (burada gerek yok, autoencoder.cpp'ye taşındı)
// #include "../core/utils.h" // SafeRNG ve diğerleri için (burada gerek yok, autoencoder.cpp'ye taşındı)
#include <cmath>   // std::exp için (burada gerek yok, autoencoder.cpp'ye taşındı)


namespace CerebrumLux { // CryptofigAutoencoder sınıfı bu namespace içine alınmalı

class CryptofigAutoencoder {
public:
    // Yapılandırma parametreleri
    static const int INPUT_DIM = 18;  // statistical_features_vector boyutu (10'dan 18'e güncellendi)
    static const int LATENT_DIM = 3;  // latent_cryptofig_vector boyutu

    CryptofigAutoencoder();

    // Encoder: statistical_features -> latent_cryptofig
    virtual std::vector<float> encode(const std::vector<float>& input_features) const;

    // Decoder: latent_cryptofig -> reconstructed_statistical_features
    virtual std::vector<float> decode(const std::vector<float>& latent_features) const;
    virtual float calculate_reconstruction_error(const std::vector<float>& original, const std::vector<float>& reconstructed) const;

    // Encoder ve Decoder'ı birleştirerek yeniden yapılandırma
    std::vector<float> reconstruct(const std::vector<float>& input_features) const;

    // Meta-öğrenme esintili, tek adımlı ağırlık ayarlaması
    void adjust_weights_on_error(const std::vector<float>& input_features, float learning_rate_ae);

    // Ağırlıkları dosyaya kaydetme/yükleme
    void save_weights(const std::string& filename) const;
    void load_weights(const std::string& filename);

private:
    // Ağırlıklar ve bias'lar
    std::vector<float> encoder_weights_1; // INPUT_DIM * LATENT_DIM
    std::vector<float> encoder_bias_1;    // LATENT_DIM

    std::vector<float> decoder_weights_1; // LATENT_DIM * INPUT_DIM
    std::vector<float> decoder_bias_1;    // INPUT_DIM

    // Yardımcı fonksiyonlar
    float sigmoid(float x) const;
    float sigmoid_derivative(float x) const;
    void initialize_random_weights();
};

} // namespace CerebrumLux

#endif // CEREBRUM_LUX_AUTOENCODER_H