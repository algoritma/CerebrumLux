// src/crypto/CryptoUtils.cpp
#include "CryptoUtils.h"
#include "../core/logger.h" // LOG_DEFAULT için
#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <openssl/pem.h>
#include <openssl/sha.h> // SHA-256 için gerekli
#include <openssl/err.h> // ERR_print_errors_fp için eklendi
#include <cstring> // for memset

namespace CerebrumLux {
namespace Crypto {

std::string base64_encode(const std::string& in) {
    BIO *b64 = BIO_new(BIO_f_base64());
    BIO *bmem = BIO_new(BIO_s_mem());
    b64 = BIO_push(b64, bmem);
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    BIO_write(b64, in.data(), static_cast<int>(in.size()));
    BIO_flush(b64);

    char *data;
    long len = BIO_get_mem_data(bmem, &data);
    std::string out(data, len);

    BIO_free_all(b64);
    return out;
}

std::string base64_decode(const std::string& in) {
    BIO *b64 = BIO_new(BIO_f_base64());
    BIO *bmem = BIO_new_mem_buf(in.data(), static_cast<int>(in.size()));
    b64 = BIO_push(b64, bmem);
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);

    std::vector<char> buffer(in.length() + 1);

    int decoded_len = BIO_read(b64, buffer.data(), static_cast<int>(in.length()));
    if (decoded_len < 0) {
        LOG_DEFAULT(LogLevel::ERR_CRITICAL, "Base64 decode hatası.");
        BIO_free_all(b64);
        return "";
    }
    buffer[decoded_len] = '\0';

    std::string out(buffer.data(), decoded_len);
    BIO_free_all(b64);
    return out;
}

std::string pkey_to_pem(EVP_PKEY* pkey, bool is_private) {
    if (!pkey) return "";
    BIO *bio = BIO_new(BIO_s_mem());
    if (!bio) {
        LOG_DEFAULT(LogLevel::ERR_CRITICAL, "PEM dönüştürme: BIO oluşturulamadı.");
        return "";
    }

    if (is_private) {
        if (1 != PEM_write_bio_PrivateKey(bio, pkey, nullptr, nullptr, 0, nullptr, nullptr)) {
            ERR_print_errors_fp(stderr);
            LOG_DEFAULT(LogLevel::ERR_CRITICAL, "PEM dönüştürme: Özel anahtar yazılamadı.");
            BIO_free_all(bio);
            return "";
        }
    } else {
        if (1 != PEM_write_bio_PUBKEY(bio, pkey)) {
            ERR_print_errors_fp(stderr);
            LOG_DEFAULT(LogLevel::ERR_CRITICAL, "PEM dönüştürme: Açık anahtar yazılamadı.");
            BIO_free_all(bio);
            return "";
        }
    }

    BUF_MEM *bptr;
    BIO_get_mem_ptr(bio, &bptr);
    if (!bptr || !bptr->data || bptr->length == 0) {
        LOG_DEFAULT(LogLevel::ERR_CRITICAL, "PEM dönüştürme: BIO belleği boş veya hatalı.");
        BIO_free_all(bio);
        return "";
    }
    std::string pem_str(bptr->data, static_cast<size_t>(bptr->length));

    BIO_free_all(bio);
    return pem_str;
}

EVP_PKEY* load_private_key_from_pem(const std::string& pem_key) {
    BIO* mem_bio = BIO_new_mem_buf(pem_key.data(), static_cast<int>(pem_key.size()));
    if (!mem_bio) {
        LOG_DEFAULT(LogLevel::ERR_CRITICAL, "Özel anahtar yükleme: BIO oluşturulamadı.");
        return nullptr;
    }
    EVP_PKEY* pkey = PEM_read_bio_PrivateKey(mem_bio, nullptr, nullptr, nullptr);
    if (!pkey) {
        ERR_print_errors_fp(stderr);
        LOG_DEFAULT(LogLevel::ERR_CRITICAL, "Özel anahtar yükleme: PEM'den anahtar yüklenemedi.");
    }
    BIO_free_all(mem_bio);
    return pkey;
}

EVP_PKEY* load_public_key_from_pem(const std::string& pem_key) {
    BIO* mem_bio = BIO_new_mem_buf(pem_key.data(), static_cast<int>(pem_key.size()));
    if (!mem_bio) {
        LOG_DEFAULT(LogLevel::ERR_CRITICAL, "Açık anahtar yükleme: BIO oluşturulamadı.");
        return nullptr;
    }
    EVP_PKEY* pkey = PEM_read_bio_PUBKEY(mem_bio, nullptr, nullptr, nullptr);
    if (!pkey) {
        ERR_print_errors_fp(stderr);
        LOG_DEFAULT(LogLevel::ERR_CRITICAL, "Açık anahtar yükleme: PEM'den anahtar yüklenemedi.");
    }
    BIO_free_all(mem_bio);
    return pkey;
}

void secure_zero_memory(void* v, size_t n) {
    if (v && n > 0) {
        OPENSSL_cleanse(v, n); // OpenSSL'in güvenli bellek temizleme fonksiyonu
    }
}

// YENİ EKLENDİ: SHA-256 hash fonksiyonu implementasyonu
std::string sha256_hash(const std::string& data) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, data.c_str(), data.length());
    SHA256_Final(hash, &sha256); // Doğru SHA256_Final kullanımı

    std::stringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    return ss.str();
}

} // namespace Crypto
} // namespace CerebrumLux