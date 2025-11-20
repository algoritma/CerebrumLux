// src/crypto/CryptoUtils.h
#ifndef CRYPTO_UTILS_H
#define CRYPTO_UTILS_H

#include <string>
#include <vector>
#include <openssl/evp.h> // ERR_print_errors_fp için gerekli
#include <stdexcept>

// OpenSSL errorları için yardımcı bir makro
#define OPENSSL_CHECK(call) \
    if ((call) <= 0) { \
        ERR_print_errors_fp(stderr); \
        throw std::runtime_error(#call " failed"); \
    }

namespace CerebrumLux {
namespace Crypto {

// Base64 kodlama/kod çözme fonksiyonları
std::string base64_encode(const std::string& in);
std::string base64_decode(const std::string& in);

// OpenSSL EVP_PKEY* objelerini PEM formatına dönüştürme
std::string pkey_to_pem(EVP_PKEY* pkey, bool is_private);

// PEM formatındaki özel anahtarı EVP_PKEY* objesine yükleme
EVP_PKEY* load_private_key_from_pem(const std::string& pem_key);

// PEM formatındaki açık anahtarı EVP_PKEY* objesine yükleme
EVP_PKEY* load_public_key_from_pem(const std::string& pem_key);

// OpenSSL hafıza temizleme
void secure_zero_memory(void* v, size_t n);

// YENİ EKLENDİ: SHA-256 hash fonksiyonu
std::string sha256_hash(const std::string& data);

} // namespace Crypto
} // namespace CerebrumLux

#endif // CRYPTO_UTILS_H