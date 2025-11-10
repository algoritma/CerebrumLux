#ifndef CRYPTO_MANAGER_H
#define CRYPTO_MANAGER_H

#include <string>
#include <vector>
#include <memory> // For std::unique_ptr
#include <openssl/evp.h> // For EVP_PKEY
#include <map>
#include <fstream> // Anahtar kaydetme/yükleme için
#include "CryptoUtils.h" // Kriptografik yardımcı fonksiyonlar için

namespace CerebrumLux {
namespace Crypto {

// AES-GCM şifrelenmiş çıktı yapısı
struct AESGCMCiphertext {
    std::string ciphertext_base64; // Şifrelenmiş veri Base64
    std::string tag_base64;        // Kimlik doğrulama etiketi Base64
    std::string iv_base64;         // Başlatma vektörü (nonce) Base64
};

class CryptoManager {
public:
    CryptoManager();
    ~CryptoManager();

    // ============================
    // Anahtar Yönetimi
    // ============================

    // Ed25519 identity anahtar çiftini oluşturur veya yükler
    void generate_or_load_identity_keys(const std::string& private_key_path = "", const std::string& public_key_path = "");

    // Geçici X25519 anahtar çifti oluşturur (Perfect Forward Secrecy için)
    std::unique_ptr<EVP_PKEY, decltype(&EVP_PKEY_free)> generate_x25519_keypair() const;

    // Ed25519 identity anahtar çifti oluşturur
    std::unique_ptr<EVP_PKEY, decltype(&EVP_PKEY_free)> generate_ed25519_keypair() const;

    // Kendi özel Ed25519 identity anahtarını döndürür (PEM formatında)
    std::string get_my_private_key_pem() const;

    // Kendi açık Ed25519 identity anahtarını döndürür (PEM formatında)
    std::string get_my_public_key_pem() const;

    // Belirli bir eşin açık anahtarını kaydeder/yönetir
    void register_peer_public_key(const std::string& peer_id, const std::string& public_key_pem);

    // Belirli bir eşin açık anahtarını döndürür (PEM formatında)
    std::string get_peer_public_key_pem(const std::string& peer_id) const;

    // ============================
    // Kriptografik Primitifler
    // ============================

    // Ed25519 ile imzalama (PEM formatında özel anahtar ile)
    std::string ed25519_sign(const std::string& message, const std::string& private_key_pem) const;
    // Overload: EVP_PKEY* ile imzalama (daha düşük seviye)
    std::vector<unsigned char> ed25519_sign(const std::vector<unsigned char>& message, EVP_PKEY* private_key) const;

    // Ed25519 ile doğrulama (PEM formatında açık anahtar ile)
    bool ed25519_verify(const std::string& message, const std::string& signature_base64, const std::string& public_key_pem) const;
    // Overload: EVP_PKEY* ile doğrulama (daha düşük seviye)
    bool ed25519_verify(const std::vector<unsigned char>& message, const std::vector<unsigned char>& signature, EVP_PKEY* public_key) const;

    // AES-256-GCM ile şifreleme
    AESGCMCiphertext aes256_gcm_encrypt(const std::string& plaintext, const std::string& key_base64, const std::string& aad = "") const;
    // Overload: Byte vektörleri ile şifreleme
    AESGCMCiphertext aes256_gcm_encrypt(const std::vector<unsigned char>& plaintext, const std::vector<unsigned char>& key, const std::vector<unsigned char>& aad) const;

    // AES-256-GCM ile şifre çözme
    std::string aes256_gcm_decrypt(const AESGCMCiphertext& ct, const std::string& key_base64, const std::string& aad = "") const;
    // Overload: Byte vektörleri ile şifre çözme
    std::vector<unsigned char> aes256_gcm_decrypt(const std::vector<unsigned char>& ciphertext, const std::vector<unsigned char>& tag, const std::vector<unsigned char>& iv, const std::vector<unsigned char>& key, const std::vector<unsigned char>& aad) const;

    // Rastgele bayt üretimi
    std::string generate_random_bytes_str(size_t length) const;
    std::vector<unsigned char> generate_random_bytes_vec(size_t length) const;

    // ============================
    // Anahtar Türetme (ECDH ve HKDF)
    // ============================

    // X25519 ile paylaşılan gizli anahtar türetir
    std::vector<unsigned char> derive_x25519_shared_secret(EVP_PKEY* my_privkey, EVP_PKEY* peer_pubkey) const;

    // HKDF-SHA256 kullanarak anahtar malzemesi türetir
    std::vector<unsigned char> hkdf_sha256(const std::vector<unsigned char>& ikm,
                                           const std::vector<unsigned char>& salt,
                                           const std::vector<unsigned char>& info,
                                           size_t out_len) const;

    // DÜZELTİLDİ: vec_to_str overload'ları doğru sırada deklarasyon edildi.
    std::string vec_to_str(const std::vector<unsigned char>& vec) const; // unsigned char vektörü için overload
    std::string vec_to_str(const std::vector<float>& vec) const;         // float vektörü için overload
    std::vector<unsigned char> str_to_vec(const std::string& str) const;

private:
    std::unique_ptr<EVP_PKEY, decltype(&EVP_PKEY_free)> my_ed25519_private_key;
    std::unique_ptr<EVP_PKEY, decltype(&EVP_PKEY_free)> my_ed25519_public_key;

    // Peer açık anahtarlarını saklamak için (şimdilik, gelecekte daha gelişmiş bir sisteme ihtiyaç duyulacak)
    std::map<std::string, std::string> peer_public_keys; // peer_id -> public_key_pem
};

} // namespace Crypto
} // namespace CerebrumLux

#endif // CRYPTO_MANAGER_H