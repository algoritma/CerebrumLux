#ifndef LEARNING_STORE_H
#define LEARNING_STORE_H

#include <string>
#include <vector>
#include <fstream>
#include <mutex>
#include <nlohmann/json.hpp>

#include "LearningState.h"
#include "BehaviorProfile.h"
#include "../core/logger.h" // LOG_DEFAULT için

namespace CerebrumLux {

class LearningStore {
public:
    LearningStore(const std::string& learning_log_path = "knowledge/learning.log",
                  const std::string& behavior_profile_path = "knowledge/behavior.json");

    // Öğrenme durumunu log dosyasına ekle
    void saveLearningState(const LearningState& state);

    // Davranış profilini dosyaya kaydet (overwrite)
    void saveBehaviorProfile(const BehaviorProfile& profile);

    // Belirli bir öğrenme durumunu log dosyasından yükle (sonuncuyu)
    // Şimdilik sadece son kaydı alıyor, ileride daha sofistike yükleme eklenebilir.
    LearningState loadLatestLearningState(const std::string& domain, const std::string& skill);

    // Davranış profilini dosyadan yükle
    BehaviorProfile loadBehaviorProfile();

private:
    std::string learningLogPath;
    std::string behaviorProfilePath;
    std::mutex learningLogMutex; // Log dosyasına yazarken çakışmaları önlemek için
    std::mutex behaviorProfileMutex; // Profil dosyasına yazarken çakışmaları önlemek için

    // Yardımcı fonksiyon: Dosyanın varlığını kontrol et ve gerekirse oluştur
    void ensureFileExists(const std::string& filePath);
};

} // namespace CerebrumLux

#endif // LEARNING_STORE_H