#include "LearningStore.h"
#include <fstream>
#include <filesystem> // std::filesystem::create_directories için

namespace CerebrumLux {

LearningStore::LearningStore(const std::string& learning_log_path, const std::string& behavior_profile_path)
    : learningLogPath(learning_log_path), behaviorProfilePath(behavior_profile_path)
{
    // knowledge dizininin var olduğundan emin ol
    std::filesystem::path dir = std::filesystem::path(learningLogPath).parent_path();
    if (!dir.empty() && !std::filesystem::exists(dir)) {
        std::filesystem::create_directories(dir);
    }
    dir = std::filesystem::path(behaviorProfilePath).parent_path();
    if (!dir.empty() && !std::filesystem::exists(dir)) {
        std::filesystem::create_directories(dir);
    }

    // Dosyaların var olduğundan emin ol, yoksa oluştur
    ensureFileExists(learningLogPath);
    ensureFileExists(behaviorProfilePath);

    LOG_DEFAULT(LogLevel::INFO, "LearningStore: Initialized with learning_log_path=" << learningLogPath << " and behavior_profile_path=" << behaviorProfilePath);
}

void LearningStore::ensureFileExists(const std::string& filePath) {
    if (!std::filesystem::exists(filePath)) {
        std::ofstream{filePath}.close(); // Dosyayı oluştur ve kapat
        LOG_DEFAULT(LogLevel::DEBUG, "LearningStore: Dosya oluşturuldu: " << filePath);
    }
}

void LearningStore::saveLearningState(const LearningState& state) {
    std::lock_guard<std::mutex> lock(learningLogMutex);
    std::ofstream ofs(learningLogPath, std::ios::app); // append mode
    if (ofs.is_open()) {
        nlohmann::json j;
        to_json(j, state);
        ofs << j.dump() << "\n";
        ofs.close();
        LOG_DEFAULT(LogLevel::DEBUG, "LearningStore: LearningState kaydedildi (domain: " << state.domain << ", skill: " << state.skill << ").");
    } else {
        LOG_DEFAULT(LogLevel::ERR_CRITICAL, "LearningStore: LearningState dosyaya yazılamadı: " << learningLogPath);
    }
}

void LearningStore::saveBehaviorProfile(const BehaviorProfile& profile) {
    std::lock_guard<std::mutex> lock(behaviorProfileMutex);
    std::ofstream ofs(behaviorProfilePath); // overwrite mode
    if (ofs.is_open()) {
        nlohmann::json j;
        to_json(j, profile);
        ofs << j.dump(2) << "\n"; // Pretty print
        ofs.close();
        LOG_DEFAULT(LogLevel::DEBUG, "LearningStore: BehaviorProfile kaydedildi.");
    } else {
        LOG_DEFAULT(LogLevel::ERR_CRITICAL, "LearningStore: BehaviorProfile dosyaya yazılamadı: " << behaviorProfilePath);
    }
}

LearningState LearningStore::loadLatestLearningState(const std::string& domain, const std::string& skill) {
    std::lock_guard<std::mutex> lock(learningLogMutex);
    std::ifstream ifs(learningLogPath);
    LearningState latestState;
    std::string line;
    nlohmann::json j;

    if (ifs.is_open()) {
        while (std::getline(ifs, line)) {
            try {
                j = nlohmann::json::parse(line);
                LearningState currentState = j.get<LearningState>();
                if (currentState.domain == domain && currentState.skill == skill) {
                    latestState = currentState; // Şimdilik sadece sonuncuyu al
                }
            } catch (const nlohmann::json::parse_error& e) {
                LOG_DEFAULT(LogLevel::ERR_CRITICAL, "LearningStore: Learning log parse hatası: " << e.what() << " (Line: " << line << ")");
            }
        }
        ifs.close();
        LOG_DEFAULT(LogLevel::DEBUG, "LearningStore: En son LearningState yüklendi (domain: " << domain << ", skill: " << skill << ").");
    } else {
        LOG_DEFAULT(LogLevel::WARNING, "LearningStore: Learning log dosyası bulunamadı veya açılamadı: " << learningLogPath);
    }
    return latestState;
}

BehaviorProfile LearningStore::loadBehaviorProfile() {
    std::lock_guard<std::mutex> lock(behaviorProfileMutex);
    std::ifstream ifs(behaviorProfilePath);
    BehaviorProfile profile;
    nlohmann::json j;

    if (ifs.is_open()) {
        try {
            ifs >> j;
            profile = j.get<BehaviorProfile>();
            ifs.close();
            LOG_DEFAULT(LogLevel::DEBUG, "LearningStore: BehaviorProfile yüklendi.");
        } catch (const nlohmann::json::parse_error& e) {
            LOG_DEFAULT(LogLevel::ERR_CRITICAL, "LearningStore: BehaviorProfile parse hatası: " << e.what());
        }
    } else {
        LOG_DEFAULT(LogLevel::WARNING, "LearningStore: BehaviorProfile dosyası bulunamadı veya açılamadı: " << behaviorProfilePath);
    }
    return profile;
}

} // namespace CerebrumLux
