#ifndef AI_TUTOR_LOOP_H
#define AI_TUTOR_LOOP_H

#include <string>
#include <vector>
#include <map>
#include <memory>

#include "curriculum.h" // Lesson, CurriculumDefinition için

// Forward declarations (MOC-safe)
namespace CerebrumLux { class IntentLearnerQtBridge; }

#include "teacher_ai.h"   // TeacherAI için
#include "student_ai.h"   // StudentAI için
#include "teacher_evaluator.h" // EvaluationResult için
namespace CerebrumLux { class LearningModule; } // Learning metrics'i güncellemek için
#include "../communication/intent_router.h" // IntentRouter için
#include "../swarm_vectordb/VectorDB.h" // VectorDB için
#include "../core/enums.h"
#include "enums.h" // StudentLevel için

#include <QObject>
#include <QString>

namespace CerebrumLux {



struct TeachingDecision {
    UserIntent intent;
    StudentLevel level;
    TeachingStyle style;
    std::string rationale;
};

struct ChatPedagogyReport {
    TeachingStyle strategy;
    LearningOutcome estimatedOutcome;
    float confidence;   // 0.0 – 1.0
    std::string shortExplanation;
};

class AITutorLoop : public QObject { // QObject'ten miras alındı
    Q_OBJECT // Q_OBJECT makrosu eklendi
public:
    explicit AITutorLoop(TeacherAI& teacher, StudentAI& student, LearningModule& learningModule,
                IntentRouter& intentRouter, SwarmVectorDB::SwarmVectorDB& vectorDB, QObject* parent = nullptr); // QObject parent parametresi eklendi

    // Öğretmen ve Öğrenci AI'leri arasında tek bir ders döngüsünü çalıştırır
    void runLesson(const std::string& user_input); // user_input kabul edecek şekilde değiştirildi

private slots:
    void onIntentLearned(int intentId); // Qt Bridge callback

signals:
    void tutorDecisionUpdated(const TeachingDecision& decision); // Yeni sinyal
    void pedagogyReportUpdated(const ChatPedagogyReport& report); // Yeni sinyal

private:
    TeacherAI& teacherAI;
    StudentAI& studentAI;
    LearningModule& learnerMetrics;
    IntentRouter& intentRouter;
    SwarmVectorDB::SwarmVectorDB& vectorDB;

    // Yardımcı fonksiyonlar (isteğe bağlı, gerekirse)
    // std::string buildTeacherPrompt(const Lesson& lesson, const std::string& studentReply);

    // Qt bridge (lifetime QObject tree ile yönetilir)
    IntentLearnerQtBridge* learnerBridge = nullptr;
};

} // namespace CerebrumLux

#endif // AI_TUTOR_LOOP_H