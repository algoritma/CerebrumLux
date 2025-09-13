#include "autoencoder.h" // Kendi başlık dosyasını dahil et
#include "../core/logger.h" // LOG makrosu için
#include "../core/utils.h" // Diğer yardımcı fonksiyonlar için
#include <cmath>     // std::exp, std::sqrt için
#include <algorithm> // std::min/max için
#include <fstream>   // For file operations (fwrite, fread, _wfopen)
#include <limits>    // std::numeric_limits (errno için değil, min/max için)
#include <iostream>  // std::wcerr için


// Statik const int üyelerinin dış tanımları
// Bu satırların yalnızca bir kez (genellikle .cpp dosyasında) tanımlanması gerekir.
const int CryptofigAutoencoder::INPUT_DIM;
const int CryptofigAutoencoder::LATENT_DIM;

CryptofigAutoencoder::CryptofigAutoencoder() 
    : gen(std::random_device()()), dist(-0.5f, 0.5f) { // Ağırlıkları -0.5 ile 0.5 arasında başlat
    initialize_random_weights();
}

void CryptofigAutoencoder::initialize_random_weights() {
    encoder_weights_1.resize(INPUT_DIM * LATENT_DIM);
    encoder_bias_1.resize(LATENT_DIM);
    decoder_weights_1.resize(LATENT_DIM * INPUT_DIM);
    decoder_bias_1.resize(INPUT_DIM);

    for (size_t i = 0; i < encoder_weights_1.size(); ++i) encoder_weights_1[i] = dist(gen);
    for (size_t i = 0; i < encoder_bias_1.size(); ++i) encoder_bias_1[i] = dist(gen);
    for (size_t i = 0; i < decoder_weights_1.size(); ++i) decoder_weights_1[i] = dist(gen);
    for (size_t i = 0; i < decoder_bias_1.size(); ++i) decoder_bias_1[i] = dist(gen);

    LOG(LogLevel::INFO, L"CryptofigAutoencoder: Agirliklar rastgele baslatildi.\n");
}

float CryptofigAutoencoder::sigmoid(float x) const {
    return 1.0f / (1.0f + std::exp(-x));
}

float CryptofigAutoencoder::sigmoid_derivative(float x) const {
    float s = sigmoid(x);
    return s * (1.0f - s);LOG(LogLevel::INFO, L"CryptofigAutoencoder: Agirliklar rastgele baslatildi.\\n");
}

std::vector<float> CryptofigAutoencoder::encode(const std::vector<float>& input_features) const {
    if (input_features.size() != INPUT_DIM) {
        LOG(LogLevel::ERR_CRITICAL, L"CryptofigAutoencoder::encode: Girdi boyutu uyuşmuyor! Beklenen: " << INPUT_DIM << L", Gelen: " << input_features.size() << L"\n");
        return std::vector<float>(LATENT_DIM, 0.0f); // Hata durumunda sıfır vektör döndür
    }

    std::vector<float> latent_features(LATENT_DIM, 0.0f);
    for (int j = 0; j < LATENT_DIM; ++j) {
        float sum = 0.0f;
        for (int i = 0; i < INPUT_DIM; ++i) {
            sum += input_features[i] * encoder_weights_1[i * LATENT_DIM + j];
        }
        latent_features[j] = sigmoid(sum + encoder_bias_1[j]);
    }
    return latent_features;
}

std::vector<float> CryptofigAutoencoder::decode(const std::vector<float>& latent_features) const {
    if (latent_features.size() != LATENT_DIM) {
        LOG(LogLevel::ERR_CRITICAL, L"CryptofigAutoencoder::decode: Gizil boyut uyuşmuyor! Beklenen: " << LATENT_DIM << L", Gelen: " << latent_features.size() << L"\n");
        return std::vector<float>(INPUT_DIM, 0.0f);
    }

    std::vector<float> reconstructed_features(INPUT_DIM, 0.0f);
    for (int j = 0; j < INPUT_DIM; ++j) {
        float sum = 0.0f;
        for (int i = 0; i < LATENT_DIM; ++i) {
            sum += latent_features[i] * decoder_weights_1[i * INPUT_DIM + j];
        }
        reconstructed_features[j] = sigmoid(sum + decoder_bias_1[j]); // Çıkış katmanında da sigmoid
    }
    return reconstructed_features;
}

std::vector<float> CryptofigAutoencoder::reconstruct(const std::vector<float>& input_features) const {
    return decode(encode(input_features));
}

// Heuristik, tek adımlı ağırlık ayarlaması (meta-öğrenme esintili)
void CryptofigAutoencoder::adjust_weights_on_error(const std::vector<float>& input_features, float learning_rate_ae) {
    if (input_features.size() != INPUT_DIM) {
        LOG(LogLevel::ERR_CRITICAL, L"CryptofigAutoencoder::adjust_weights_on_error: Girdi boyutu uyuşmuyor! Ayarlama yapılamıyor.\n");
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

    // Sadece hata belirli bir eşiğin üzerindeyse ayar yap
    if (total_error > 0.1f) { // Örnek eşik değeri
        float adjustment_magnitude = learning_rate_ae * total_error; // Hata ne kadar büyükse, ayar o kadar büyük

        // Decoder ağırlıklarını ve bias'larını ayarla
        for (int j = 0; j < INPUT_DIM; ++j) {
            float output_error = input_features[j] - reconstructed_features[j];
            decoder_bias_1[j] += adjustment_magnitude * output_error;
            decoder_bias_1[j] = std::min(1.0f, std::max(-1.0f, decoder_bias_1[j])); // Ağırlık/bias sınırlandırması

            for (int i = 0; i < LATENT_DIM; ++i) {
                // Basit gradyan esintili ayar (decoder'ı orijinal girişe yaklaştırmaya çalış)
                decoder_weights_1[i * INPUT_DIM + j] += adjustment_magnitude * output_error * latent_features[i];
                decoder_weights_1[i * INPUT_DIM + j] = std::min(1.0f, std::max(-1.0f, decoder_weights_1[i * INPUT_DIM + j]));
            }
        }

        // Encoder ağırlıklarını ve bias'larını ayarla (latent uzayın daha iyi temsil etmesini sağlamak için)
        // Bu kısım daha karmaşık bir geri yayılım gerektirse de, heuristik bir yaklaşım uygulayacağız:
        // Yeniden yapılandırma hatasını azaltacak şekilde latent gösterimi etkileyen encoder ağırlıklarını ayarla.
        // Bu basit bir "error_signal * input_feature" kuralı ile yapılabilir.
        for (int j = 0; j < LATENT_DIM; ++j) {
            float latent_error_signal = 0.0f;
            for (int k = 0; k < INPUT_DIM; ++k) {
                latent_error_signal += (input_features[k] - reconstructed_features[k]) * decoder_weights_1[j * INPUT_DIM + k];
            }
            encoder_bias_1[j] += adjustment_magnitude * latent_error_signal;
            encoder_bias_1[j] = std::min(1.0f, std::max(-1.0f, encoder_bias_1[j]));

            for (int i = 0; i < INPUT_DIM; ++i) {
                encoder_weights_1[i * LATENT_DIM + j] += adjustment_magnitude * latent_error_signal * input_features[i];
                encoder_weights_1[i * LATENT_DIM + j] = std::min(1.0f, std::max(-1.0f, encoder_weights_1[i * LATENT_DIM + j]));
            }
        }
        LOG(LogLevel::DEBUG, L"CryptofigAutoencoder: Agirliklar hataya gore ayarlandi. Hata: " << std::fixed << std::setprecision(4) << total_error << L"\n");
    }
}

void CryptofigAutoencoder::save_weights(const std::wstring& filename) const {
    FILE* fp = _wfopen(filename.c_str(), L"wb"); // Binary modda yaz
    if (!fp) {
        LOG(LogLevel::ERR_CRITICAL, L"Hata: Autoencoder agirlik dosyasi yazilamadi: " << filename << L" (errno: " << errno << L")\n");
        return;
    }

    // Boyutları yaz
    fwrite(&INPUT_DIM, sizeof(int), 1, fp);
    fwrite(&LATENT_DIM, sizeof(int), 1, fp);

    // Vektörleri yaz
    size_t size;
    size = encoder_weights_1.size(); fwrite(&size, sizeof(size_t), 1, fp); fwrite(encoder_weights_1.data(), sizeof(float), size, fp);
    size = encoder_bias_1.size();    fwrite(&size, sizeof(size_t), 1, fp); fwrite(encoder_bias_1.data(), sizeof(float), size, fp);
    size = decoder_weights_1.size(); fwrite(&size, sizeof(size_t), 1, fp); fwrite(decoder_weights_1.data(), sizeof(float), size, fp);
    size = decoder_bias_1.size();    fwrite(&size, sizeof(size_t), 1, fp); fwrite(decoder_bias_1.data(), sizeof(float), size, fp);

    fclose(fp);
    LOG(LogLevel::INFO, L"CryptofigAutoencoder agirliklari kaydedildi: " << filename << L"\n");
}

void CryptofigAutoencoder::load_weights(const std::wstring& filename) {
    FILE* fp = _wfopen(filename.c_str(), L"rb"); // Binary modda oku
    if (!fp) {
        LOG(LogLevel::WARNING, L"Uyari: Autoencoder agirlik dosyasi bulunamadi, rastgele agirliklar kullaniliyor: " << filename << L" (errno: " << errno << L")\n");
        initialize_random_weights(); // Bulunamadıysa rastgele başlat
        return;
    }

    int loaded_input_dim, loaded_latent_dim;
    fread(&loaded_input_dim, sizeof(int), 1, fp);
    fread(&loaded_latent_dim, sizeof(int), 1, fp);

    if (loaded_input_dim != INPUT_DIM || loaded_latent_dim != LATENT_DIM) {
        LOG(LogLevel::ERR_CRITICAL, L"Hata: Autoencoder agirlik dosyasi boyutlari uyuşmuyor! Dosya: " << loaded_input_dim << L"x" << loaded_latent_dim << L", Beklenen: " << INPUT_DIM << L"x" << LATENT_DIM << L". Rastgele agirliklar kullaniliyor.\n");
        fclose(fp);
        initialize_random_weights();
        return;
    }

    size_t size;
    fread(&size, sizeof(size_t), 1, fp); encoder_weights_1.resize(size); fread(encoder_weights_1.data(), sizeof(float), size, fp);
    fread(&size, sizeof(size_t), 1, fp); encoder_bias_1.resize(size);    fread(encoder_bias_1.data(), sizeof(float), size, fp);
    fread(&size, sizeof(size_t), 1, fp); decoder_weights_1.resize(size); fread(decoder_weights_1.data(), sizeof(float), size, fp);
    fread(&size, sizeof(size_t), 1, fp); decoder_bias_1.resize(size);    fread(decoder_bias_1.data(), sizeof(float), size, fp);
    
    fclose(fp);
    LOG(LogLevel::INFO, L"CryptofigAutoencoder agirliklari yuklendi: " << filename << L"\n");
}
