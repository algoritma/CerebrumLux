#ifndef AI_TUTOR_LOOP_H
#define AI_TUTOR_LOOP_H

#include <string>
#include <vector>
#include <map>

// LLMEngine forward declaration (Header dependency'yi azaltmak için)
namespace CerebrumLux {
    class LLMEngine;
}

// Müfredatın bir bölümü (Örn: "cxx", "conversation")
struct CurriculumSection {
    std::vector<std::string> topics;
    std::string difficulty; // "adaptive", "easy", "hard"
};

// Tüm müfredat yapısı
struct Curriculum {
    std::map<std::string, CurriculumSection> sections;
};

// Değerlendirme sonuçları
struct Evaluation {
    std::map<std::string, float> scores;
    std::string feedback;
};

// Tek bir dersin yapısı
struct Lesson {
    std::string goal;
    std::vector<std::string> examples;
    std::string task;
};

// Öğretmen-Öğrenci döngüsündeki veri paketi (Frame)
struct TeacherStudentFrame {
    std::string teacher_role = "ai_tutor";
    std::string student_role = "cerebrumlux_ai";
    Curriculum curriculum;
    Lesson lesson;
    Evaluation evaluation;
    std::string student_response;
};

// Döngüyü çalıştıran ana fonksiyon
TeacherStudentFrame runTutorLoop(CerebrumLux::LLMEngine* teacherModel,
                                 CerebrumLux::LLMEngine* studentModel,
                                 const TeacherStudentFrame& lastFrame);

#endif // AI_TUTOR_LOOP_H