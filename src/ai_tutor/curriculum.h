#ifndef CURRICULUM_H
#define CURRICULUM_H

#include <string>
#include <vector>
#include <map>
#include <iostream>
#include <fstream>
#include <mutex>
#include <stdexcept>

#include <nlohmann/json.hpp> // YENİ: JSON serileştirme/deserileştirme için

namespace CerebrumLux { // Namespace eklendi

// YENİ: AI Rol Tanımı için Struct'lar
struct AIActor {
    std::string role;
    std::vector<std::string> abilities;

    // JSON'dan dönüştürme (nlohmann/json desteği)
    friend void from_json(const nlohmann::json& j, AIActor& p) {
        j.at("role").get_to(p.role);
        j.at("abilities").get_to(p.abilities);
    }
};

struct AIRoles {
    AIActor teacher_ai;
    AIActor student_ai;

    // JSON'dan dönüştürme
    friend void from_json(const nlohmann::json& j, AIRoles& p) {
        j.at("teacher_ai").get_to(p.teacher_ai);
        j.at("student_ai").get_to(p.student_ai);
    }
};

// YENİ: Dersin beklenen özelliklerini tanımlayan Struct
struct LessonTrait {
    float politeness = 0.0f;
    float clarity = 0.0f;
    float conciseness = 0.0f;
    float correctness = 0.0f;
    float relevance = 0.0f;
    float tone = 0.0f;

    // JSON'dan dönüştürme
    friend void from_json(const nlohmann::json& j, LessonTrait& p) {
        p.politeness = j.value("politeness", 0.0f);
        p.clarity = j.value("clarity", 0.0f);
        p.conciseness = j.value("conciseness", 0.0f);
        p.correctness = j.value("correctness", 0.0f);
        p.relevance = j.value("relevance", 0.0f);
        p.tone = j.value("tone", 0.0f);
    }
};

// YENİ: Tek bir dersi tanımlayan Struct
struct Lesson {
    std::string id;
    std::string prompt;
    std::map<std::string, float> success_criteria; // Eski JSON formatından uyarlama
    LessonTrait expected_traits; // Yeni JSON formatından

    // JSON'dan dönüştürme
    friend void from_json(const nlohmann::json& j, Lesson& p) {
        j.at("id").get_to(p.id);
        j.at("prompt").get_to(p.prompt);
        if (j.count("success_criteria")) { // Eski format uyumluluğu
            j.at("success_criteria").get_to(p.success_criteria);
        }
        if (j.count("expected_traits")) { // Yeni format
            j.at("expected_traits").get_to(p.expected_traits);
        }
    }
};

// YENİ: Tüm müfredat yapısını tanımlayan Struct
struct CurriculumDefinition {
    std::string domain;
    std::string level;
    std::vector<std::string> objectives;
    std::vector<Lesson> lessons;

    // JSON'dan dönüştürme
    friend void from_json(const nlohmann::json& j, CurriculumDefinition& p) {
        j.at("domain").get_to(p.domain);
        j.at("level").get_to(p.level);
        j.at("objectives").get_to(p.objectives);
        j.at("lessons").get_to(p.lessons);
    }
};

// Mevcut CurriculumSection ve Curriculum sınıflarını koru veya kaldır
// Şimdilik mevcut yapıyı koruyarak yeni tanımları ekliyorum.
// İleride bu iki yapı birleştirilebilir veya yenisiyle değiştirilebilir.

// Müfredatın bir bölümü (Örn: "cxx", "conversation")
struct CurriculumSection {
    std::string title;
    std::vector<std::string> objectives;
    std::vector<std::string> sample_tasks;
    std::string difficulty; // "adaptive", "easy", "hard"
};

// Tüm müfredat yapısı
class Curriculum {
public:
    void addSection(const std::string& title, const CurriculumSection& section) {
        std::lock_guard<std::mutex> lock(mtx);
        sections[title] = section;
    }

    CurriculumSection* getSection(const std::string& title) {
        std::lock_guard<std::mutex> lock(mtx);
        auto it = sections.find(title);
        if (it != sections.end()) {
            return &it->second;
        }
        return nullptr;
    }
    
    std::map<std::string, CurriculumSection> sections;
private:
    std::mutex mtx;
};

// Global singleton instance
extern Curriculum GLOBAL_CURRICULUM;

} // namespace CerebrumLux

#endif // CURRICULUM_H
