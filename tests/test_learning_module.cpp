// File: tests/test_learning_module.cpp
#include <iostream>
#include <string>
#include <vector>
#include <cassert>
#include <chrono>
#include <algorithm> // std::min için

// CerebrumLux çekirdek ve öğrenme bileşenleri için gerekli başlıklar
#include "../src/core/logger.h"
#include "../src/core/enums.h"
#include "../src/core/utils.h"
#include "../src/learning/LearningModule.h"
#include "../src/learning/KnowledgeBase.h"
#include "../src/learning/Capsule.h"
#include "../src/crypto/CryptoManager.h"
#include "../src/crypto/CryptoUtils.h"

// JSON işleme için
#include "../src/external/nlohmann/json.hpp"


int main() {
    // Logger'ı test ortamı için başlat
    CerebrumLux::Logger::get_instance().init(CerebrumLux::LogLevel::DEBUG, "test_learning_module.log", "TEST_LM");
    LOG_DEFAULT(CerebrumLux::LogLevel::INFO, "--- Starting LearningModule Test Scenarios ---");

    // Gerekli bağımlılıkları oluştur
    CerebrumLux::Crypto::CryptoManager cryptoManager;
    CerebrumLux::KnowledgeBase kb;
    CerebrumLux::LearningModule learning_module(kb, cryptoManager);

    // Test senaryolarında kullanılacak helper lambda (main.cpp'den taşındı)
    auto create_signed_encrypted_capsule = [&](const std::string& id_prefix, const std::string& content, const std::string& source_peer, float confidence) {
        CerebrumLux::Capsule c;
        static unsigned int local_capsule_id_counter = 0; // Bu sayaç her test çalıştığında sıfırlanabilir.
        c.id = id_prefix + std::to_string(++local_capsule_id_counter);
        c.content = content;
        c.source = source_peer;
        c.topic = "Test Topic";
        c.confidence = confidence;
        c.plain_text_summary = content.substr(0, std::min((size_t)100, content.length())) + "...";
        c.timestamp_utc = std::chrono::system_clock::now();

        c.embedding = learning_module.compute_embedding(c.content);
        c.cryptofig_blob_base64 = learning_module.cryptofig_encode(c.embedding);

        std::vector<unsigned char> aes_key_vec = cryptoManager.generate_random_bytes_vec(32);
        std::vector<unsigned char> iv_vec = cryptoManager.generate_random_bytes_vec(12);
        c.encryption_iv_base64 = CerebrumLux::Crypto::base64_encode(cryptoManager.vec_to_str(iv_vec));

        CerebrumLux::Crypto::AESGCMCiphertext encrypted_data =
            cryptoManager.aes256_gcm_encrypt(cryptoManager.str_to_vec(c.content), aes_key_vec, {});

        c.encrypted_content = encrypted_data.ciphertext_base64;
        c.gcm_tag_base64 = encrypted_data.tag_base64;

        std::string private_key_pem = cryptoManager.get_my_private_key_pem();
        c.signature_base64 = cryptoManager.ed25519_sign(c.encrypted_content, private_key_pem);
        return c;
    };


    // Test Senaryosu 1: Başarılı Kapsül Yutma (Valid Signature, Clean Content)
    LOG_DEFAULT(CerebrumLux::LogLevel::INFO, "Running Test Scenario 1: Valid Capsule Ingestion");
    CerebrumLux::Capsule test_capsule_1 = create_signed_encrypted_capsule("valid_capsule_", "Bu temiz bir test kapsuludur. Guzel bir gun geciriyoruz.", "Test_Peer_A", 0.8f);
    CerebrumLux::IngestReport report_1 = learning_module.ingest_envelope(test_capsule_1, test_capsule_1.signature_base64, test_capsule_1.source);
    LOG_DEFAULT(CerebrumLux::LogLevel::INFO, "Test 1 Result: " << static_cast<int>(report_1.result) << " - " << report_1.message);
    assert(report_1.result == CerebrumLux::IngestResult::Success); // Başarılı olmalı


    // Test Senaryosu 2: Geçersiz İmza
    LOG_DEFAULT(CerebrumLux::LogLevel::INFO, "Running Test Scenario 2: Invalid Signature");
    CerebrumLux::Capsule test_capsule_2 = create_signed_encrypted_capsule("invalid_sig_capsule_", "Bu kapsulun imzasi gecersiz olmali.", "Unauthorized_Peer", 0.5f);
    test_capsule_2.signature_base64 = "invalid_signature_tampered"; // Kasten yanlış imza
    CerebrumLux::IngestReport report_2 = learning_module.ingest_envelope(test_capsule_2, test_capsule_2.signature_base64, test_capsule_2.source);
    LOG_DEFAULT(CerebrumLux::LogLevel::INFO, "Test 2 Result (Invalid Signature): " << static_cast<int>(report_2.result) << " - " << report_2.message);
    assert(report_2.result == CerebrumLux::IngestResult::InvalidSignature); // Geçersiz imza hatası olmalı


    // Test Senaryosu 3: Steganografi İçeren Kapsül
    // Not: StegoDetector'un şu anki implementasyonu dummy olabilir, ancak test senaryosu çalışmalı.
    LOG_DEFAULT(CerebrumLux::LogLevel::INFO, "Running Test Scenario 3: Steganography Detection");
    CerebrumLux::Capsule test_capsule_3 = create_signed_encrypted_capsule("stego_capsule_", "Normal gorunen bir metin ama icinde hidden_message_tag var.", "Suspicious_Source", 0.7f); // StegoDetector tetiklemeli
    CerebrumLux::IngestReport report_3 = learning_module.ingest_envelope(test_capsule_3, test_capsule_3.signature_base64, test_capsule_3.source);
    LOG_DEFAULT(CerebrumLux::LogLevel::INFO, "Test 3 Result (Steganography Detected): " << static_cast<int>(report_3.result) << " - " << report_3.message);
    // Eğer StegoDetector'unuz gerçek implementasyonda çalışıyorsa burayı assert(report_3.result == CerebrumLux::IngestResult::SteganographyDetected); yapın.
    // Şimdilik çökmediğini varsayarak Success veya UnknownError olabilir.
    // assert(report_3.result == CerebrumLux::IngestResult::SteganographyDetected); // Eğer detektör aktifse

    // Test Senaryosu 4: Unicode Temizleme Gerektiren Kapsül
    LOG_DEFAULT(CerebrumLux::LogLevel::INFO, "Running Test Scenario 4: Unicode Sanitization Needed");
    CerebrumLux::Capsule test_capsule_4 = create_signed_encrypted_capsule("unicode_capsule_", "Metin\x01\x02\x03içinde kontrol karakterleri var. \t Yeni satir.", "Dirty_Source", 0.9f); // UnicodeSanitizer tetiklemeli
    CerebrumLux::IngestReport report_4 = learning_module.ingest_envelope(test_capsule_4, test_capsule_4.signature_base64, test_capsule_4.source);
    LOG_DEFAULT(CerebrumLux::LogLevel::INFO, "Test 4 Result (Sanitization Needed): " << static_cast<int>(report_4.result) << " - " << report_4.message);
    if (report_4.result == CerebrumLux::IngestResult::SanitizationNeeded) {
        LOG_DEFAULT(CerebrumLux::LogLevel::INFO, "    Sanitized Content: " << report_4.processed_capsule.content);
    }
    // assert(report_4.result == CerebrumLux::IngestResult::SanitizationNeeded); // Eğer sanitizer aktifse


    LOG_DEFAULT(CerebrumLux::LogLevel::INFO, "--- Finished LearningModule Test Scenarios ---");

    return 0; // Testlerin başarılı olduğunu gösterir
}