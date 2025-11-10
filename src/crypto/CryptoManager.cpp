#include "CryptoManager.h"
#include "CryptoUtils.h"
#include "../core/logger.h" // LOG_DEFAULT için
#include "../core/enums.h" // LogLevel için
#include <iostream>
#include <fstream>
#include <vector>
#include <stdexcept>
#include <openssl/rand.h> // RAND_bytes için
#include <openssl/err.h>  // ERR_get_error, ERR_error_string_n için
#include <openssl/kdf.h>  // HKDF için
#include <openssl/param_build.h> // OSSL_PARAM_BLD için
#include <openssl/params.h> // OSSL_PARAM için
#include <filesystem> // For std::filesystem::exists

namespace CerebrumLux {
namespace Crypto {

// AES-GCM şifrelenmiş çıktı yapısı (struct tanımı CryptoManager.h'de olmalı)

CryptoManager::CryptoManager() :
    my_ed25519_private_key(nullptr, EVP_PKEY_free),
    my_ed25519_public_key(nullptr, EVP_PKEY_free)
{
    LOG_DEFAULT(LogLevel::INFO, "CryptoManager: Initializing. Attempting to load or generate identity keys.");
    try {
        generate_or_load_identity_keys("my_ed25519_private.pem", "my_ed25519_public.pem");
    } catch (const std::exception& e) {
        // DÜZELTİLDİ: LogLevel::CRITICAL yerine LogLevel::ERR_CRITICAL kullanıldı.
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "CryptoManager: Anahtar olusturma/yukleme sirasinda kritik hata: " << e.what());
        // Bu noktada uygulamanın devam etmesini istemeyebiliriz, veya daha iyi bir kurtarma mekanizması olabilir.
        // Şimdilik sadece logluyoruz ve uygulama diğer hatalarla karşılaşsa bile GUI'yi açmaya çalışıyoruz.
    }
}

CryptoManager::~CryptoManager() {
    // Unique_ptr'ler otomatik olarak bellek yönetimini yapacaktır.
}

void CryptoManager::generate_or_load_identity_keys(const std::string& private_key_filename, const std::string& public_key_filename) {
    bool loaded_successfully = false;
    std::string private_key_pem_content;
    std::string public_key_pem_content;

    // Anahtar dosyalarının varlığını kontrol et ve içeriklerini oku
    std::filesystem::path priv_path(private_key_filename);
    std::filesystem::path pub_path(public_key_filename);

    if (std::filesystem::exists(priv_path) && std::filesystem::exists(pub_path)) {
        try {
            std::ifstream priv_file(priv_path);
            if (priv_file.is_open()) {
                private_key_pem_content.assign((std::istreambuf_iterator<char>(priv_file)), std::istreambuf_iterator<char>());
                priv_file.close();
            } else {
                LOG_DEFAULT(LogLevel::WARNING, "CryptoManager: Private key file '" << private_key_filename << "' exists but could not be opened. Generating new keys.");
            }

            std::ifstream pub_file(pub_path);
            if (pub_file.is_open()) {
                public_key_pem_content.assign((std::istreambuf_iterator<char>(pub_file)), std::istreambuf_iterator<char>());
                pub_file.close();
            } else {
                LOG_DEFAULT(LogLevel::WARNING, "CryptoManager: Public key file '" << public_key_filename << "' exists but could not be opened. Generating new keys.");
            }

            // PEM içeriğinden anahtarları yüklemeyi dene
            if (!private_key_pem_content.empty() && !public_key_pem_content.empty()) {
                // Önceki OpenSSL hatalarını temizle
                ERR_clear_error();
                
                std::unique_ptr<EVP_PKEY, decltype(&EVP_PKEY_free)> temp_private_key(load_private_key_from_pem(private_key_pem_content), EVP_PKEY_free);
                std::unique_ptr<EVP_PKEY, decltype(&EVP_PKEY_free)> temp_public_key(load_public_key_from_pem(public_key_pem_content), EVP_PKEY_free);
                
                if (temp_private_key && temp_public_key) {
                    my_ed25519_private_key = std::move(temp_private_key);
                    my_ed25519_public_key = std::move(temp_public_key);
                    LOG_DEFAULT(LogLevel::INFO, "CryptoManager: Identity keys loaded from files.");
                    loaded_successfully = true;
                } else {
                    // Yükleme başarısız olursa ancak istisna fırlatılmazsa, OpenSSL hata kuyruğunu kontrol et
                    unsigned long err_code = ERR_get_error();
                    if (err_code != 0) {
                        char err_buf[256];
                        ERR_error_string_n(err_code, err_buf, sizeof(err_buf));
                        // DÜZELTİLDİ: LogLevel::CRITICAL yerine LogLevel::ERR_CRITICAL kullanıldı.
                        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "CryptoManager: Failed to load keys from PEM content. OpenSSL Error: " << err_buf << ". Generating new keys.");
                    } else {
                        LOG_DEFAULT(LogLevel::WARNING, "CryptoManager: Key files found but invalid PEM content. Generating new keys.");
                    }
                }
            } else {
                LOG_DEFAULT(LogLevel::WARNING, "CryptoManager: Key files are empty. Generating new keys.");
            }

        } catch (const std::exception& e) {
            LOG_ERROR_CERR(LogLevel::WARNING, "CryptoManager: Error reading or loading existing key files: " << e.what() << ". Generating new keys.");
        }
    } else {
        LOG_DEFAULT(LogLevel::INFO, "CryptoManager: Identity key files not found. Generating new ones.");
    }

    if (loaded_successfully) {
        return;
    }

    // Yüklenemezse veya geçersizse yeni anahtarlar oluştur
    my_ed25519_private_key.reset(generate_ed25519_keypair().release());
    if (!my_ed25519_private_key) {
        // DÜZELTİLDİ: LogLevel::CRITICAL yerine LogLevel::ERR_CRITICAL kullanıldı.
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "CryptoManager: Failed to generate Ed25519 keypair.");
        throw std::runtime_error("Failed to generate Ed25519 keypair.");
    }
    LOG_DEFAULT(LogLevel::DEBUG, "CryptoManager: Ed25519 keypair generated.");

    // Açık anahtarı özel anahtardan türetme (EVP_PKEY_public_copy yerine alternatif)
    size_t pub_key_len = 0;
    if (EVP_PKEY_get_raw_public_key(my_ed25519_private_key.get(), NULL, &pub_key_len) <= 0) {
        unsigned long err_code = ERR_get_error();
        // DÜZELTİLDİ: LogLevel::CRITICAL yerine LogLevel::ERR_CRITICAL kullanıldı.
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "CryptoManager: Failed to get Ed25519 public key length. OpenSSL Error: " << ERR_error_string(err_code, NULL));
        throw std::runtime_error("Failed to get Ed25519 public key length.");
    }
    std::vector<unsigned char> raw_pub_key(pub_key_len);
    if (EVP_PKEY_get_raw_public_key(my_ed25519_private_key.get(), raw_pub_key.data(), &pub_key_len) <= 0) {
        unsigned long err_code = ERR_get_error();
        // DÜZELTİLDİ: LogLevel::CRITICAL yerine LogLevel::ERR_CRITICAL kullanıldı.
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "CryptoManager: Failed to get Ed25519 raw public key. OpenSSL Error: " << ERR_error_string(err_code, NULL));
        throw std::runtime_error("Failed to get Ed25519 raw public key.");
    }

    EVP_PKEY *pubkey_obj = EVP_PKEY_new_raw_public_key(EVP_PKEY_ED25519, NULL, raw_pub_key.data(), pub_key_len);
    if (!pubkey_obj) {
        unsigned long err_code = ERR_get_error();
        // DÜZELTİLDİ: LogLevel::CRITICAL yerine LogLevel::ERR_CRITICAL kullanıldı.
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "CryptoManager: Failed to create public key from raw bytes. OpenSSL Error: " << ERR_error_string(err_code, NULL));
        throw std::runtime_error("Failed to create public key from raw bytes.");
    }
    my_ed25519_public_key.reset(pubkey_obj);
    
    LOG_DEFAULT(LogLevel::INFO, "CryptoManager: New Ed25519 identity keypair generated.");
    
    // Yeni anahtarları dosyalara kaydet
    std::ofstream priv_file_out(priv_path);
    if (priv_file_out.is_open()) {
        priv_file_out << pkey_to_pem(my_ed25519_private_key.get(), true);
        LOG_DEFAULT(LogLevel::INFO, "CryptoManager: Private key saved to " << private_key_filename);
    } else {
        // DÜZELTİLDİ: LogLevel::CRITICAL yerine LogLevel::ERR_CRITICAL kullanıldı.
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "CryptoManager: Failed to save private key to " << private_key_filename);
    }

    std::ofstream pub_file_out(pub_path);
    if (pub_file_out.is_open()) {
        pub_file_out << pkey_to_pem(my_ed25519_public_key.get(), false);
        LOG_DEFAULT(LogLevel::INFO, "CryptoManager: Public key saved to " << public_key_filename);
    } else {
        // DÜZELTİLDİ: LogLevel::CRITICAL yerine LogLevel::ERR_CRITICAL kullanıldı.
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "CryptoManager: Failed to save public key to " << public_key_filename);
    }
}

std::unique_ptr<EVP_PKEY, decltype(&EVP_PKEY_free)> CryptoManager::generate_x25519_keypair() const {
    EVP_PKEY_CTX *pctx = EVP_PKEY_CTX_new_id(EVP_PKEY_X25519, NULL);
    if (!pctx) throw std::runtime_error("EVP_PKEY_CTX_new_id failed for X25519.");

    std::unique_ptr<EVP_PKEY_CTX, decltype(&EVP_PKEY_CTX_free)> ctx(pctx, EVP_PKEY_CTX_free);

    OPENSSL_CHECK(EVP_PKEY_keygen_init(ctx.get()));
    EVP_PKEY *pkey = NULL;
    OPENSSL_CHECK(EVP_PKEY_keygen(ctx.get(), &pkey));

    return std::unique_ptr<EVP_PKEY, decltype(&EVP_PKEY_free)>(pkey, EVP_PKEY_free);
}

std::unique_ptr<EVP_PKEY, decltype(&EVP_PKEY_free)> CryptoManager::generate_ed25519_keypair() const {
    EVP_PKEY_CTX *pctx = EVP_PKEY_CTX_new_id(EVP_PKEY_ED25519, NULL);
    if (!pctx) throw std::runtime_error("EVP_PKEY_CTX_new_id failed for Ed25519.");

    std::unique_ptr<EVP_PKEY_CTX, decltype(&EVP_PKEY_CTX_free)> ctx(pctx, EVP_PKEY_CTX_free);

    OPENSSL_CHECK(EVP_PKEY_keygen_init(ctx.get()));
    EVP_PKEY *pkey = NULL;
    OPENSSL_CHECK(EVP_PKEY_keygen(ctx.get(), &pkey));

    return std::unique_ptr<EVP_PKEY, decltype(&EVP_PKEY_free)>(pkey, EVP_PKEY_free);
}

std::string CryptoManager::get_my_private_key_pem() const {
    if (!my_ed25519_private_key) {
        // DÜZELTİLDİ: LogLevel::CRITICAL yerine LogLevel::ERR_CRITICAL kullanıldı.
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "CryptoManager: get_my_private_key_pem called but private key is not initialized.");
        throw std::runtime_error("Private key not initialized.");
    }
    return pkey_to_pem(my_ed25519_private_key.get(), true);
}

std::string CryptoManager::get_my_public_key_pem() const {
    if (!my_ed25519_public_key) {
        // DÜZELTİLDİ: LogLevel::CRITICAL yerine LogLevel::ERR_CRITICAL kullanıldı.
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "CryptoManager: get_my_public_key_pem called but public key is not initialized.");
        throw std::runtime_error("Public key not initialized.");
    }
    return pkey_to_pem(my_ed25519_public_key.get(), false);
}

void CryptoManager::register_peer_public_key(const std::string& peer_id, const std::string& public_key_pem) {
    peer_public_keys[peer_id] = public_key_pem;
    LOG_DEFAULT(LogLevel::DEBUG, "CryptoManager: Public key registered for peer: " << peer_id);
}

std::string CryptoManager::get_peer_public_key_pem(const std::string& peer_id) const {
    auto it = peer_public_keys.find(peer_id);
    if (it != peer_public_keys.end()) {
        return it->second;
    }
    LOG_DEFAULT(LogLevel::WARNING, "CryptoManager: Public key not found for peer: " << peer_id);
    return ""; // Veya istisna fırlat
}

std::string CryptoManager::ed25519_sign(const std::string& message, const std::string& private_key_pem) const {
    std::unique_ptr<EVP_PKEY, decltype(&EVP_PKEY_free)> private_key(load_private_key_from_pem(private_key_pem), EVP_PKEY_free);
    if (!private_key) {
        // DÜZELTİLDİ: LogLevel::CRITICAL yerine LogLevel::ERR_CRITICAL kullanıldı.
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "CryptoManager: ed25519_sign: Failed to load private key from PEM.");
        throw std::runtime_error("Failed to load private key for signing.");
    }
    return base64_encode(vec_to_str(ed25519_sign(str_to_vec(message), private_key.get())));
}

std::vector<unsigned char> CryptoManager::ed25519_sign(const std::vector<unsigned char>& message, EVP_PKEY* private_key) const {
    if (!private_key) {
        // DÜZELTİLDİ: LogLevel::CRITICAL yerine LogLevel::ERR_CRITICAL kullanıldı.
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "CryptoManager: ed25519_sign: private_key is NULL.");
        throw std::runtime_error("Private key is NULL for signing.");
    }

    EVP_MD_CTX *mdctx = EVP_MD_CTX_new();
    if (!mdctx) throw std::runtime_error("EVP_MD_CTX_new failed.");
    std::unique_ptr<EVP_MD_CTX, decltype(&EVP_MD_CTX_free)> ctx(mdctx, EVP_MD_CTX_free);

    OPENSSL_CHECK(EVP_DigestSignInit(ctx.get(), NULL, NULL, NULL, private_key));

    size_t sig_len = 0;
    OPENSSL_CHECK(EVP_DigestSign(ctx.get(), NULL, &sig_len, message.data(), message.size()));

    std::vector<unsigned char> signature(sig_len);
    OPENSSL_CHECK(EVP_DigestSign(ctx.get(), signature.data(), &sig_len, message.data(), message.size()));

    LOG_DEFAULT(LogLevel::DEBUG, "CryptoManager: Ed25519 imzalama başarılı.");
    return signature;
}


bool CryptoManager::ed25519_verify(const std::string& message, const std::string& signature_base64, const std::string& public_key_pem) const {
    std::unique_ptr<EVP_PKEY, decltype(&EVP_PKEY_free)> public_key(load_public_key_from_pem(public_key_pem), EVP_PKEY_free);
    if (!public_key) {
        // DÜZELTİLDİ: LogLevel::CRITICAL yerine LogLevel::ERR_CRITICAL kullanıldı.
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "CryptoManager: ed25519_verify: Failed to load public key from PEM.");
        return false;
    }
    return ed25519_verify(str_to_vec(message), str_to_vec(base64_decode(signature_base64)), public_key.get());
}

bool CryptoManager::ed25519_verify(const std::vector<unsigned char>& message, const std::vector<unsigned char>& signature, EVP_PKEY* public_key) const {
    if (!public_key) {
        // DÜZELTİLDİ: LogLevel::CRITICAL yerine LogLevel::ERR_CRITICAL kullanıldı.
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "CryptoManager: ed25519_verify: public_key is NULL.");
        return false;
    }

    EVP_MD_CTX *mdctx = EVP_MD_CTX_new();
    if (!mdctx) throw std::runtime_error("EVP_MD_CTX_new failed.");
    std::unique_ptr<EVP_MD_CTX, decltype(&EVP_MD_CTX_free)> ctx(mdctx, EVP_MD_CTX_free);

    if (EVP_DigestVerifyInit(ctx.get(), NULL, NULL, NULL, public_key) <= 0) {
        unsigned long err_code = ERR_get_error();
        // DÜZELTİLDİ: LogLevel::CRITICAL yerine LogLevel::ERR_CRITICAL kullanıldı.
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "CryptoManager: EVP_DigestVerifyInit failed. OpenSSL Error: " << ERR_error_string(err_code, NULL));
        return false;
    }
    if (EVP_DigestVerify(ctx.get(), signature.data(), signature.size(), message.data(), message.size()) <= 0) {
        return false;
    }

    LOG_DEFAULT(LogLevel::DEBUG, "CryptoManager: Ed25519 dogrulama başarılı.");
    return true;
}

AESGCMCiphertext CryptoManager::aes256_gcm_encrypt(const std::string& plaintext, const std::string& key_base64, const std::string& aad) const {
    return aes256_gcm_encrypt(str_to_vec(plaintext), str_to_vec(base64_decode(key_base64)), str_to_vec(aad));
}

AESGCMCiphertext CryptoManager::aes256_gcm_encrypt(const std::vector<unsigned char>& plaintext, const std::vector<unsigned char>& key, const std::vector<unsigned char>& aad) const {
    if (key.size() != 32) {
        // DÜZELTİLDİ: LogLevel::CRITICAL yerine LogLevel::ERR_CRITICAL kullanıldı.
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "CryptoManager: AES key size must be 32 bytes.");
        throw std::invalid_argument("AES key size must be 32 bytes.");
    }

    std::vector<unsigned char> iv(12);
    OPENSSL_CHECK(RAND_bytes(iv.data(), iv.size()));

    EVP_CIPHER_CTX *pctx = EVP_CIPHER_CTX_new();
    if (!pctx) throw std::runtime_error("EVP_CIPHER_CTX_new failed.");
    std::unique_ptr<EVP_CIPHER_CTX, decltype(&EVP_CIPHER_CTX_free)> ctx(pctx, EVP_CIPHER_CTX_free);

    OPENSSL_CHECK(EVP_EncryptInit_ex(ctx.get(), EVP_aes_256_gcm(), NULL, NULL, NULL));
    OPENSSL_CHECK(EVP_CIPHER_CTX_ctrl(ctx.get(), EVP_CTRL_GCM_SET_IVLEN, iv.size(), NULL));
    OPENSSL_CHECK(EVP_EncryptInit_ex(ctx.get(), NULL, NULL, key.data(), iv.data()));

    int len;
    std::vector<unsigned char> ciphertext;
    ciphertext.resize(plaintext.size() + EVP_MAX_BLOCK_LENGTH);

    if (!aad.empty()) {
        OPENSSL_CHECK(EVP_EncryptUpdate(ctx.get(), NULL, &len, aad.data(), aad.size()));
    }

    OPENSSL_CHECK(EVP_EncryptUpdate(ctx.get(), ciphertext.data(), &len, plaintext.data(), plaintext.size()));
    size_t ciphertext_len = len;

    OPENSSL_CHECK(EVP_EncryptFinal_ex(ctx.get(), ciphertext.data() + len, &len));
    ciphertext_len += len;
    ciphertext.resize(ciphertext_len);

    std::vector<unsigned char> tag(EVP_GCM_TLS_TAG_LEN);
    OPENSSL_CHECK(EVP_CIPHER_CTX_ctrl(ctx.get(), EVP_CTRL_GCM_GET_TAG, tag.size(), tag.data()));

    LOG_DEFAULT(LogLevel::DEBUG, "CryptoManager: AES-256-GCM şifreleme başarılı.");
    return {base64_encode(vec_to_str(ciphertext)), base64_encode(vec_to_str(tag)), base64_encode(vec_to_str(iv))};
}

std::string CryptoManager::aes256_gcm_decrypt(const AESGCMCiphertext& ct, const std::string& key_base64, const std::string& aad_str) const {
    std::vector<unsigned char> decoded_ciphertext = str_to_vec(base64_decode(ct.ciphertext_base64));
    std::vector<unsigned char> decoded_tag = str_to_vec(base64_decode(ct.tag_base64));
    std::vector<unsigned char> decoded_iv = str_to_vec(base64_decode(ct.iv_base64));
    std::vector<unsigned char> decoded_key = str_to_vec(base64_decode(key_base64));
    std::vector<unsigned char> decoded_aad = str_to_vec(aad_str);

    std::vector<unsigned char> decrypted_data = aes256_gcm_decrypt(
        decoded_ciphertext,
        decoded_tag,
        decoded_iv,
        decoded_key,
        decoded_aad
    );

    return vec_to_str(decrypted_data);
}

std::vector<unsigned char> CryptoManager::aes256_gcm_decrypt(const std::vector<unsigned char>& ciphertext, const std::vector<unsigned char>& tag, const std::vector<unsigned char>& iv, const std::vector<unsigned char>& key, const std::vector<unsigned char>& aad) const {
    if (key.size() != 32) {
        // DÜZELTİLDİ: LogLevel::CRITICAL yerine LogLevel::ERR_CRITICAL kullanıldı.
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "CryptoManager: AES key size must be 32 bytes for decryption.");
        throw std::invalid_argument("AES key size must be 32 bytes for decryption.");
    }
    if (iv.size() != 12) {
        // DÜZELTİLDİ: LogLevel::CRITICAL yerine LogLevel::ERR_CRITICAL kullanıldı.
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "CryptoManager: IV size must be 12 bytes for GCM decryption.");
        throw std::invalid_argument("IV size must be 12 bytes for GCM decryption.");
    }

    EVP_CIPHER_CTX *pctx = EVP_CIPHER_CTX_new();
    if (!pctx) throw std::runtime_error("EVP_CIPHER_CTX_new failed.");
    std::unique_ptr<EVP_CIPHER_CTX, decltype(&EVP_CIPHER_CTX_free)> ctx(pctx, EVP_CIPHER_CTX_free);

    OPENSSL_CHECK(EVP_DecryptInit_ex(ctx.get(), EVP_aes_256_gcm(), NULL, NULL, NULL));
    OPENSSL_CHECK(EVP_CIPHER_CTX_ctrl(ctx.get(), EVP_CTRL_GCM_SET_IVLEN, iv.size(), NULL));
    OPENSSL_CHECK(EVP_DecryptInit_ex(ctx.get(), NULL, NULL, key.data(), iv.data()));

    int len;
    std::vector<unsigned char> plaintext;
    plaintext.resize(ciphertext.size() + EVP_MAX_BLOCK_LENGTH);

    if (!aad.empty()) {
        OPENSSL_CHECK(EVP_DecryptUpdate(ctx.get(), NULL, &len, aad.data(), aad.size()));
    }

    OPENSSL_CHECK(EVP_DecryptUpdate(ctx.get(), plaintext.data(), &len, ciphertext.data(), ciphertext.size()));
    size_t plaintext_len = len;

    // GCM etiketi ayarla - const_cast kullanıldı (düzeltildi)
    OPENSSL_CHECK(EVP_CIPHER_CTX_ctrl(ctx.get(), EVP_CTRL_GCM_SET_TAG, tag.size(), const_cast<unsigned char*>(tag.data())));

    if (EVP_DecryptFinal_ex(ctx.get(), plaintext.data() + len, &len) <= 0) {
        LOG_ERROR_CERR(LogLevel::WARNING, "CryptoManager: AES-256-GCM etiket dogrulama basarisiz. OpenSSL Error: " << ERR_error_string(ERR_get_error(), NULL));
        throw std::runtime_error("AES-256-GCM tag verification failed.");
    }
    plaintext_len += len;
    plaintext.resize(plaintext_len);

    LOG_DEFAULT(LogLevel::DEBUG, "CryptoManager: AES-256-GCM şifre çözme başarılı.");
    return plaintext;
}


std::string CryptoManager::generate_random_bytes_str(size_t length) const {
    std::vector<unsigned char> bytes(length);
    OPENSSL_CHECK(RAND_bytes(bytes.data(), bytes.size()));
    return vec_to_str(bytes);
}

std::vector<unsigned char> CryptoManager::generate_random_bytes_vec(size_t length) const {
    std::vector<unsigned char> bytes(length);
    OPENSSL_CHECK(RAND_bytes(bytes.data(), bytes.size()));
    return bytes;
}

std::vector<unsigned char> CryptoManager::derive_x25519_shared_secret(EVP_PKEY* my_privkey, EVP_PKEY* peer_pubkey) const {
    if (!my_privkey || !peer_pubkey) {
        // DÜZELTİLDİ: LogLevel::CRITICAL yerine LogLevel::ERR_CRITICAL kullanıldı.
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "CryptoManager: derive_x25519_shared_secret: NULL key provided.");
        throw std::invalid_argument("NULL key provided for ECDH shared secret derivation.");
    }

    EVP_PKEY_CTX *pctx = EVP_PKEY_CTX_new(my_privkey, NULL);
    if (!pctx) throw std::runtime_error("EVP_PKEY_CTX_new failed for ECDH.");
    std::unique_ptr<EVP_PKEY_CTX, decltype(&EVP_PKEY_CTX_free)> ctx(pctx, EVP_PKEY_CTX_free);

    OPENSSL_CHECK(EVP_PKEY_derive_init(ctx.get()));
    OPENSSL_CHECK(EVP_PKEY_derive_set_peer(ctx.get(), peer_pubkey));

    size_t secret_len;
    OPENSSL_CHECK(EVP_PKEY_derive(ctx.get(), NULL, &secret_len));

    std::vector<unsigned char> shared_secret(secret_len);
    OPENSSL_CHECK(EVP_PKEY_derive(ctx.get(), shared_secret.data(), &secret_len));

    return shared_secret;
}

std::vector<unsigned char> CryptoManager::hkdf_sha256(const std::vector<unsigned char>& ikm,
                                                       const std::vector<unsigned char>& salt,
                                                       const std::vector<unsigned char>& info,
                                                       size_t out_len) const {
    std::vector<unsigned char> out_key(out_len);
    if (PKCS5_PBKDF2_HMAC((const char*)ikm.data(), ikm.size(),
                          salt.empty() ? NULL : salt.data(), salt.size(),
                          1, EVP_sha256(), out_len, out_key.data()) <= 0) {
        // DÜZELTİLDİ: LogLevel::CRITICAL yerine LogLevel::ERR_CRITICAL kullanıldı.
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "CryptoManager: PKCS5_PBKDF2_HMAC (as HKDF placeholder) failed.");
        throw std::runtime_error("HKDF derivation failed.");
    }
    LOG_DEFAULT(LogLevel::DEBUG, "CryptoManager: HKDF-SHA256 türetme başarılı.");
    return out_key;
}

std::string CryptoManager::vec_to_str(const std::vector<unsigned char>& vec) const {
    return std::string(reinterpret_cast<const char*>(vec.data()), vec.size());
}

std::vector<unsigned char> CryptoManager::str_to_vec(const std::string& str) const {
    return std::vector<unsigned char>(str.begin(), str.end());
}

} // namespace Crypto
} // namespace CerebrumLux