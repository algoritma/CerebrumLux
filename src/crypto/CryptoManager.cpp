// src/crypto/CryptoManager.cpp
#include "CryptoManager.h"
#include "../../core/logger.h"
#include <openssl/rand.h>
#include <openssl/err.h> // ERR_print_errors_fp için eklendi
#include <openssl/pem.h>
#include <openssl/x509.h>
#include <openssl/kdf.h> // HKDF için

namespace CerebrumLux {
namespace Crypto {

CryptoManager::CryptoManager()
    : my_ed25519_private_key(nullptr, &EVP_PKEY_free),
      my_ed25519_public_key(nullptr, &EVP_PKEY_free) {

    LOG_DEFAULT(LogLevel::INFO, "CryptoManager: Initializing. Attempting to load or generate identity keys.");
    try {
        generate_or_load_identity_keys("my_ed25519_private.pem", "my_ed25519_public.pem");
    } catch (const std::exception& e) {
        LOG_DEFAULT(LogLevel::ERR_CRITICAL, "CryptoManager: Identity key initialization failed: " << e.what());
    }
}

CryptoManager::~CryptoManager() {
    LOG_DEFAULT(LogLevel::INFO, "CryptoManager: Destructor called. Clearing sensitive keys.");
}

void CryptoManager::generate_or_load_identity_keys(const std::string& private_key_path, const std::string& public_key_path) {
    if (!private_key_path.empty() && !public_key_path.empty()) {
        std::ifstream priv_file(private_key_path);
        std::ifstream pub_file(public_key_path);
        if (priv_file.is_open() && pub_file.is_open()) {
            std::string priv_pem((std::istreambuf_iterator<char>(priv_file)), std::istreambuf_iterator<char>());
            std::string pub_pem((std::istreambuf_iterator<char>(pub_file)), std::istreambuf_iterator<char>());

            my_ed25519_private_key.reset(load_private_key_from_pem(priv_pem));
            my_ed25519_public_key.reset(load_public_key_from_pem(pub_pem));

            if (my_ed25519_private_key && my_ed25519_public_key) {
                LOG_DEFAULT(LogLevel::INFO, "CryptoManager: Identity keys loaded from files.");
                return;
            } else {
                LOG_DEFAULT(LogLevel::WARNING, "CryptoManager: Failed to load identity keys from files, generating new ones.");
            }
        }
    }

    my_ed25519_private_key.reset(generate_ed25519_keypair().release());
    if (my_ed25519_private_key) {
        my_ed25519_public_key.reset(EVP_PKEY_dup(my_ed25519_private_key.get()));
        if (!my_ed25519_public_key) {
            ERR_print_errors_fp(stderr);
            my_ed25519_private_key.reset();
            throw std::runtime_error("EVP_PKEY_dup failed for public key");
        }
        LOG_DEFAULT(LogLevel::INFO, "CryptoManager: New Ed25519 identity keypair generated.");

        if (!private_key_path.empty() && !public_key_path.empty()) {
            std::ofstream priv_file(private_key_path);
            if (priv_file.is_open()) {
                priv_file << pkey_to_pem(my_ed25519_private_key.get(), true);
                LOG_DEFAULT(LogLevel::INFO, "CryptoManager: Private key saved to " << private_key_path);
            }
            std::ofstream pub_file(public_key_path);
            if (pub_file.is_open()) {
                pub_file << pkey_to_pem(my_ed25519_public_key.get(), false);
                LOG_DEFAULT(LogLevel::INFO, "CryptoManager: Public key saved to " << public_key_path);
            }
        }
    } else {
        throw std::runtime_error("Failed to generate Ed25519 private key.");
    }
}

std::unique_ptr<EVP_PKEY, decltype(&EVP_PKEY_free)> CryptoManager::generate_x25519_keypair() const {
    EVP_PKEY_CTX* kctx = EVP_PKEY_CTX_new_id(EVP_PKEY_X25519, nullptr);
    if (!kctx) throw std::runtime_error("EVP_PKEY_CTX_new_id failed for X25519");
    OPENSSL_CHECK(EVP_PKEY_keygen_init(kctx));
    EVP_PKEY* pkey = nullptr;
    OPENSSL_CHECK(EVP_PKEY_keygen(kctx, &pkey));
    EVP_PKEY_CTX_free(kctx);
    LOG_DEFAULT(LogLevel::DEBUG, "CryptoManager: X25519 keypair generated.");
    return std::unique_ptr<EVP_PKEY, decltype(&EVP_PKEY_free)>(pkey, &EVP_PKEY_free);
}

std::unique_ptr<EVP_PKEY, decltype(&EVP_PKEY_free)> CryptoManager::generate_ed25519_keypair() const {
    EVP_PKEY_CTX* kctx = EVP_PKEY_CTX_new_id(EVP_PKEY_ED25519, nullptr);
    if (!kctx) throw std::runtime_error("EVP_PKEY_CTX_new_id failed for Ed25519");
    OPENSSL_CHECK(EVP_PKEY_keygen_init(kctx));
    EVP_PKEY* pkey = nullptr;
    OPENSSL_CHECK(EVP_PKEY_keygen(kctx, &pkey));
    EVP_PKEY_CTX_free(kctx);
    LOG_DEFAULT(LogLevel::DEBUG, "CryptoManager: Ed25519 keypair generated.");
    return std::unique_ptr<EVP_PKEY, decltype(&EVP_PKEY_free)>(pkey, &EVP_PKEY_free);
}

std::string CryptoManager::get_my_private_key_pem() const {
    if (!my_ed25519_private_key) {
        LOG_DEFAULT(LogLevel::ERR_CRITICAL, "CryptoManager: Kendi özel anahtarım mevcut değil.");
        return "";
    }
    return pkey_to_pem(my_ed25519_private_key.get(), true);
}

std::string CryptoManager::get_my_public_key_pem() const {
    if (!my_ed25519_public_key) {
        LOG_DEFAULT(LogLevel::ERR_CRITICAL, "CryptoManager: Kendi açık anahtarım mevcut değil.");
        return "";
    }
    return pkey_to_pem(my_ed25519_public_key.get(), false);
}

void CryptoManager::register_peer_public_key(const std::string& peer_id, const std::string& public_key_pem) {
    if (public_key_pem.empty()) {
        LOG_DEFAULT(LogLevel::WARNING, "CryptoManager: Kaydedilecek eş açık anahtarı boş.");
        return;
    }
    peer_public_keys[peer_id] = public_key_pem;
    LOG_DEFAULT(LogLevel::INFO, "CryptoManager: Peer '" << peer_id << "' için açık anahtar kaydedildi.");
}

std::string CryptoManager::get_peer_public_key_pem(const std::string& peer_id) const {
    auto it = peer_public_keys.find(peer_id);
    if (it != peer_public_keys.end()) {
        return it->second;
    }
    LOG_DEFAULT(LogLevel::WARNING, "CryptoManager: Peer '" << peer_id << "' için açık anahtar bulunamadı. Dummy döndürülüyor.");
    // Geçici olarak bir dummy anahtar döndürüyoruz, gerçek implementasyonda hata veya exception olmalı.
    return "-----BEGIN PUBLIC KEY-----\nMFkwEwYHKoZIzj0CAQYIKoZIzj0DAQcDQgAE(DUMMY_PUBKEY_FOR_PEER)\n-----END PUBLIC KEY-----\n";
}

// Ed25519 İmzalama
std::string CryptoManager::ed25519_sign(const std::string& message, const std::string& private_key_pem) const {
    std::unique_ptr<EVP_PKEY, decltype(&EVP_PKEY_free)> pkey(load_private_key_from_pem(private_key_pem), &EVP_PKEY_free);
    if (!pkey) return "";
    return base64_encode(vec_to_str(ed25519_sign(str_to_vec(message), pkey.get())));
}

std::vector<unsigned char> CryptoManager::ed25519_sign(const std::vector<unsigned char>& message, EVP_PKEY* private_key) const {
    if (EVP_PKEY_id(private_key) != EVP_PKEY_ED25519) {
        LOG_DEFAULT(LogLevel::ERR_CRITICAL, "Ed25519 sign: Yüklenen anahtar Ed25519 değil.");
        return {};
    }

    std::unique_ptr<EVP_PKEY_CTX, decltype(&EVP_PKEY_CTX_free)> ctx(EVP_PKEY_CTX_new(private_key, nullptr), &EVP_PKEY_CTX_free);
    if (!ctx) throw std::runtime_error("EVP_PKEY_CTX_new failed for Ed25519 sign");

    OPENSSL_CHECK(EVP_PKEY_sign_init(ctx.get()));

    size_t sig_len = 0;
    OPENSSL_CHECK(EVP_PKEY_sign(ctx.get(), nullptr, &sig_len, message.data(), message.size()));

    std::vector<unsigned char> signature(sig_len);
    OPENSSL_CHECK(EVP_PKEY_sign(ctx.get(), signature.data(), &sig_len, message.data(), message.size()));
    signature.resize(sig_len);

    LOG_DEFAULT(LogLevel::DEBUG, "CryptoManager: Ed25519 imzalama başarılı.");
    return signature;
}

// Ed25519 Doğrulama
bool CryptoManager::ed25519_verify(const std::string& message, const std::string& signature_base64, const std::string& public_key_pem) const {
    std::unique_ptr<EVP_PKEY, decltype(&EVP_PKEY_free)> pkey(load_public_key_from_pem(public_key_pem), &EVP_PKEY_free);
    if (!pkey) return false;
    return ed25519_verify(str_to_vec(message), str_to_vec(base64_decode(signature_base64)), pkey.get());
}

bool CryptoManager::ed25519_verify(const std::vector<unsigned char>& message, const std::vector<unsigned char>& signature, EVP_PKEY* public_key) const {
    if (EVP_PKEY_id(public_key) != EVP_PKEY_ED25519) {
        LOG_DEFAULT(LogLevel::ERR_CRITICAL, "Ed25519 verify: Yüklenen anahtar Ed25519 değil.");
        return false;
    }

    std::unique_ptr<EVP_PKEY_CTX, decltype(&EVP_PKEY_CTX_free)> ctx(EVP_PKEY_CTX_new(public_key, nullptr), &EVP_PKEY_CTX_free);
    if (!ctx) throw std::runtime_error("EVP_PKEY_CTX_new failed for Ed25519 verify");

    OPENSSL_CHECK(EVP_PKEY_verify_init(ctx.get()));

    int rc = EVP_PKEY_verify(ctx.get(), signature.data(), signature.size(), message.data(), message.size());

    if (rc == 1) {
        LOG_DEFAULT(LogLevel::DEBUG, "CryptoManager: Ed25519 doğrulama başarılı.");
        return true;
    } else if (rc == 0) {
        LOG_DEFAULT(LogLevel::WARNING, "CryptoManager: Ed25519 doğrulama başarısız (geçersiz imza).");
        return false;
    } else {
        ERR_print_errors_fp(stderr);
        LOG_DEFAULT(LogLevel::ERR_CRITICAL, "CryptoManager: Ed25519 doğrulama sırasında hata oluştu.");
        return false;
    }
}

// AES-256-GCM Şifreleme
AESGCMCiphertext CryptoManager::aes256_gcm_encrypt(const std::string& plaintext, const std::string& key_base64, const std::string& aad) const {
    return aes256_gcm_encrypt(str_to_vec(plaintext), str_to_vec(base64_decode(key_base64)), str_to_vec(aad));
}

AESGCMCiphertext CryptoManager::aes256_gcm_encrypt(const std::vector<unsigned char>& plaintext, const std::vector<unsigned char>& key, const std::vector<unsigned char>& aad) const {
    AESGCMCiphertext out;
    std::vector<unsigned char> iv_vec(12);
    OPENSSL_CHECK(RAND_bytes(iv_vec.data(), (int)iv_vec.size()));
    out.iv_base64 = base64_encode(vec_to_str(iv_vec));

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) throw std::runtime_error("EVP_CIPHER_CTX_new failed");

    OPENSSL_CHECK(EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, nullptr, nullptr));
    OPENSSL_CHECK(EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, (int)iv_vec.size(), nullptr));
    OPENSSL_CHECK(EVP_EncryptInit_ex(ctx, nullptr, nullptr, key.data(), iv_vec.data()));

    int len = 0;
    if (!aad.empty()) {
        OPENSSL_CHECK(EVP_EncryptUpdate(ctx, nullptr, &len, aad.data(), (int)aad.size()));
    }

    std::vector<unsigned char> ciphertext_vec(plaintext.size() + EVP_CIPHER_block_size(EVP_aes_256_gcm()));
    OPENSSL_CHECK(EVP_EncryptUpdate(ctx, ciphertext_vec.data(), &len, plaintext.data(), (int)plaintext.size()));
    int outlen = len;

    std::vector<unsigned char> tag_vec(16); // 16 bytes for GCM tag
    OPENSSL_CHECK(EVP_EncryptFinal_ex(ctx, ciphertext_vec.data() + len, &len));
    outlen += len;
    ciphertext_vec.resize(outlen);

    OPENSSL_CHECK(EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, 16, tag_vec.data()));

    EVP_CIPHER_CTX_free(ctx);

    out.ciphertext_base64 = base64_encode(vec_to_str(ciphertext_vec));
    out.tag_base64 = base64_encode(vec_to_str(tag_vec));
    LOG_DEFAULT(LogLevel::DEBUG, "CryptoManager: AES-256-GCM şifreleme başarılı.");
    return out;
}

// AES-256-GCM Şifre Çözme
std::string CryptoManager::aes256_gcm_decrypt(const AESGCMCiphertext& ct, const std::string& key_base64, const std::string& aad) const {
    std::vector<unsigned char> ciphertext = str_to_vec(base64_decode(ct.ciphertext_base64));
    std::vector<unsigned char> tag = str_to_vec(base64_decode(ct.tag_base64));
    std::vector<unsigned char> iv = str_to_vec(base64_decode(ct.iv_base64));
    std::vector<unsigned char> key = str_to_vec(base64_decode(key_base64));
    std::vector<unsigned char> aad_vec = str_to_vec(aad);

    std::vector<unsigned char> decrypted_data = aes256_gcm_decrypt(ciphertext, tag, iv, key, aad_vec);
    return vec_to_str(decrypted_data);
}

std::vector<unsigned char> CryptoManager::aes256_gcm_decrypt(const std::vector<unsigned char>& ciphertext, const std::vector<unsigned char>& tag, const std::vector<unsigned char>& iv, const std::vector<unsigned char>& key, const std::vector<unsigned char>& aad) const {
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) throw std::runtime_error("EVP_CIPHER_CTX_new failed");

    OPENSSL_CHECK(EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, nullptr, nullptr));
    OPENSSL_CHECK(EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, (int)iv.size(), nullptr));
    OPENSSL_CHECK(EVP_DecryptInit_ex(ctx, nullptr, nullptr, key.data(), iv.data()));

    int len = 0;
    if (!aad.empty()) {
        OPENSSL_CHECK(EVP_DecryptUpdate(ctx, nullptr, &len, aad.data(), (int)aad.size()));
    }

    std::vector<unsigned char> plaintext_vec(ciphertext.size()); // Max possible size
    OPENSSL_CHECK(EVP_DecryptUpdate(ctx, plaintext_vec.data(), &len, ciphertext.data(), (int)ciphertext.size()));
    int outlen = len;

    OPENSSL_CHECK(EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, (int)tag.size(), (void*)tag.data()));

    if (EVP_DecryptFinal_ex(ctx, plaintext_vec.data() + outlen, &len) <= 0) {
        EVP_CIPHER_CTX_free(ctx);
        LOG_DEFAULT(LogLevel::WARNING, "CryptoManager: AES-256-GCM şifre çözme: Kimlik doğrulama başarısız.");
        throw std::runtime_error("Authentication failed");
    }
    outlen += len;
    plaintext_vec.resize(outlen);

    EVP_CIPHER_CTX_free(ctx);
    LOG_DEFAULT(LogLevel::DEBUG, "CryptoManager: AES-256-GCM şifre çözme başarılı.");
    return plaintext_vec;
}

// Rastgele bayt üretimi
std::string CryptoManager::generate_random_bytes_str(size_t length) const {
    std::vector<unsigned char> random_vec = generate_random_bytes_vec(length);
    return vec_to_str(random_vec);
}

std::vector<unsigned char> CryptoManager::generate_random_bytes_vec(size_t length) const {
    std::vector<unsigned char> buffer(length);
    OPENSSL_CHECK(RAND_bytes(buffer.data(), (int)length));
    return buffer;
}

// X25519 ile paylaşılan gizli anahtar türetir
std::vector<unsigned char> CryptoManager::derive_x25519_shared_secret(EVP_PKEY* my_privkey, EVP_PKEY* peer_pubkey) const {
    std::unique_ptr<EVP_PKEY_CTX, decltype(&EVP_PKEY_CTX_free)> ctx(EVP_PKEY_CTX_new(my_privkey, nullptr), &EVP_PKEY_CTX_free);
    if (!ctx) throw std::runtime_error("EVP_PKEY_CTX_new failed for shared secret derivation");

    OPENSSL_CHECK(EVP_PKEY_derive_init(ctx.get()));
    OPENSSL_CHECK(EVP_PKEY_derive_set_peer(ctx.get(), peer_pubkey));

    size_t secret_len = 0;
    OPENSSL_CHECK(EVP_PKEY_derive(ctx.get(), nullptr, &secret_len));

    std::vector<unsigned char> secret(secret_len);
    OPENSSL_CHECK(EVP_PKEY_derive(ctx.get(), secret.data(), &secret_len));
    secret.resize(secret_len);

    LOG_DEFAULT(LogLevel::DEBUG, "CryptoManager: X25519 shared secret türetildi. Uzunluk: " << secret.size() << " bayt.");
    return secret;
}

// HKDF-SHA256 kullanarak anahtar malzemesi türetir
std::vector<unsigned char> CryptoManager::hkdf_sha256(const std::vector<unsigned char>& ikm,
                                       const std::vector<unsigned char>& salt,
                                       const std::vector<unsigned char>& info,
                                       size_t out_len) const {
    std::unique_ptr<EVP_PKEY_CTX, decltype(&EVP_PKEY_CTX_free)> pctx(EVP_PKEY_CTX_new_id(EVP_PKEY_HKDF, nullptr), &EVP_PKEY_CTX_free);
    if (!pctx) throw std::runtime_error("HKDF ctx new failed");

    OPENSSL_CHECK(EVP_PKEY_derive_init(pctx.get()));
    OPENSSL_CHECK(EVP_PKEY_CTX_set_hkdf_md(pctx.get(), EVP_sha256()));

    if (!salt.empty()) {
        OPENSSL_CHECK(EVP_PKEY_CTX_set1_hkdf_salt(pctx.get(), salt.data(), (int)salt.size()));
    }
    OPENSSL_CHECK(EVP_PKEY_CTX_set1_hkdf_key(pctx.get(), ikm.data(), (int)ikm.size()));

    if (!info.empty()) {
        OPENSSL_CHECK(EVP_PKEY_CTX_add1_hkdf_info(pctx.get(), info.data(), (int)info.size()));
    }

    std::vector<unsigned char> out(out_len);
    size_t olen = out_len;
    OPENSSL_CHECK(EVP_PKEY_derive(pctx.get(), out.data(), &olen));
    out.resize(olen);

    LOG_DEFAULT(LogLevel::DEBUG, "CryptoManager: HKDF-SHA256 ile anahtar malzemesi türetildi. Uzunluk: " << out.size() << " bayt.");
    return out;
}

// ============================
// Yardımcı Fonksiyonlar (public yapıldı)
// ============================

std::string CryptoManager::vec_to_str(const std::vector<unsigned char>& vec) const {
    return std::string(reinterpret_cast<const char*>(vec.data()), vec.size());
}

std::vector<unsigned char> CryptoManager::str_to_vec(const std::string& str) const {
    return std::vector<unsigned char>(reinterpret_cast<const unsigned char*>(str.data()),
                                      reinterpret_cast<const unsigned char*>(str.data() + str.size()));
}

} // namespace Crypto
} // namespace CerebrumLux