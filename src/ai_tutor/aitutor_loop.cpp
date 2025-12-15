#include "aitutor_loop.h"
#include "../learning/LearningModule.h"
#include "../core/logger.h"
#include "student_ai.h"
#include "../brain/IntentLearnerQtBridge.h"
#include <nlohmann/json.hpp> 
#include "student_level_evaluator.h" // StudentLevelEvaluator için
#include "teacher_evaluator.h" // EvaluationParser için

namespace CerebrumLux {


// Static helper function for explain_decision (A1.2)
std::string explain_decision(
    UserIntent intent,
    StudentLevel level,
    TeachingStyle style
) {
    std::ostringstream oss;

    oss << "Niyet: " << CerebrumLux::to_string(intent)
        << ", Öğrenci Seviyesi: " << to_string(level)
        << ". ";

    switch (style) {
        case TeachingStyle::MICRO_STEPS:
            oss << "Küçük adımlarla öğrenme tercih edildi.";
            break;
        case TeachingStyle::ERROR_DRIVEN:
            oss << "Hata üzerinden öğrenme en verimli yöntem.";
            break;
        case TeachingStyle::SOCRATIC:
            oss << "Analitik düşünmeyi teşvik etmek için soru temelli yaklaşım seçildi.";
            break;
        case TeachingStyle::DIRECT:
            oss << "Doğrudan anlatım yeterli görüldü.";
            break;
        case TeachingStyle::EXAMPLE_FIRST:
            oss << "Örneklerle öğrenme yöntemi uygulandı.";
            break;
        case TeachingStyle::UNKNOWN:
        default:
            oss << "Varsayılan öğretim yöntemi uygulandı.";
    }

    return oss.str();
}

AITutorLoop::AITutorLoop(TeacherAI& teacher, StudentAI& student, LearningModule& learningModule,
                       IntentRouter& intentRouter, SwarmVectorDB::SwarmVectorDB& vectorDB, QObject* parent)
    : QObject(parent), teacherAI(teacher), studentAI(student), learnerMetrics(learningModule),
      intentRouter(intentRouter), vectorDB(vectorDB)
{
    LOG_DEFAULT(LogLevel::INFO, "AITutorLoop: Initialized.");

    // IntentLearner Qt Bridge (MOC-safe)
    learnerBridge = new IntentLearnerQtBridge(
        intentRouter.getIntentLearner(),
        this
    );

    connect(
        learnerBridge,
        &IntentLearnerQtBridge::intentLearned,
        this,
        &AITutorLoop::onIntentLearned
    );
}

void AITutorLoop::runLesson(const std::string& user_input) {
    LOG_DEFAULT(LogLevel::INFO, "AITutorLoop: Starting lesson with user input: " << user_input);

    // RLHF: etkileşim state'i aksiyon öncesi kaydet
    learnerMetrics.setLastInteraction(
        std::vector<float>{},
        AIAction::TutorRespond
    );

    // 1. Niyet tipini algıla
    UserIntent intent = intentRouter.detect(user_input);
    LOG_DEFAULT(LogLevel::INFO, "AITutorLoop: Detected IntentType: " << CerebrumLux::to_string(intent));

    // 2. Öğrencinin geçmiş strateji sonuçlarını yükle (StudentLevelEvaluator için)
    std::vector<StrategyOutcome> history_for_level_inference = vectorDB.load_strategy_history(intent, 50); // Daha fazla geçmiş yükle
    
    // 3. Öğrenci seviyesini çıkar
    // Infer_student_level'a EvaluationResult listesi sağlamak gerekiyor.
    // Varsayılan olarak boş liste ile çağırıyoruz, gerçek implementasyonda VectorDB'den
    // EvaluationResult'ları alacak bir mekanizma olmalı.
    StudentLevel student_level = AITutor::StudentLevelEvaluator::infer_student_level(
        // Burada StudentAI veya LearningModule'den geçmiş EvaluationResult'ları almak gerekiyor.
        // Şu an için bu mekanizma yok, bu yüzden geçici olarak boş bir liste ile Unknown dönecek.
        // Bu TODO ileride gerçek geçmiş verileriyle beslenecek.
        std::vector<EvaluationResult>() // TODO: Gerçek EvaluationResult geçmişi sağlanacak
    );
    LOG_DEFAULT(LogLevel::INFO, "AITutorLoop: Inferred StudentLevel: " << to_string(student_level));

    // 4. Strateji geçmişini yükle (Öğretme stili çözümü için)
    std::vector<StrategyOutcome> history_for_style_resolution = vectorDB.load_strategy_history(intent, 20);

    // 5. Öğretme stilini çöz
    TeachingStyle style = teacherAI.resolve_teaching_style(intent, student_level, history_for_style_resolution);
    LOG_DEFAULT(LogLevel::INFO, "AITutorLoop: Resolved TeachingStyle: " << to_string(style));

    // 6. Öğretmenden dersi üretmesini iste
    std::string lesson_content = teacherAI.generate_lesson(style, intent, user_input);
    LOG_DEFAULT(LogLevel::INFO, "AITutorLoop: Teacher generated lesson: " << lesson_content.substr(0, std::min((size_t)100, lesson_content.length())) << "...");

    // 7. Öğrenci yanıtını al
    std::string studentReply = studentAI.respond(lesson_content);
    LOG_DEFAULT(LogLevel::INFO, "AITutorLoop: Student responded: " << studentReply.substr(0, std::min((size_t)100, studentReply.length())) << "...");

    // 8. Öğretmen değerlendirmesi
    std::string evaluation_json = teacherAI.evaluate(studentReply);
    EvaluationResult evaluation = EvaluationParser::parseEvaluationResult(evaluation_json);
    
    LOG_DEFAULT(LogLevel::INFO, "AITutorLoop: Student response evaluated. Overall Score: " << evaluation.score_overall << ", Feedback: " << evaluation.feedback);

    // 9. Öğrenme çıktısını değerlendir (A1.2 için)
    LearningOutcome estimated_learning_outcome = AITutor::StudentLevelEvaluator::evaluate_outcome(evaluation, student_level);
    LOG_DEFAULT(LogLevel::INFO, "AITutorLoop: Estimated Learning Outcome: " << to_string(estimated_learning_outcome));

    // 10. Strateji sonucunu değerlendir ve kaydet
    StrategyOutcome current_outcome;
    current_outcome.style = style;
    current_outcome.delta_correctness = evaluation.score_cxx / 100.0f;
    current_outcome.delta_clarity = evaluation.score_conversation / 100.0f;
    current_outcome.delta_efficiency = (evaluation.score_overall / 100.0f) * 0.8f;
    current_outcome.retention_score = (evaluation.score_overall / 100.0f) * 0.9f;

    vectorDB.store_strategy_outcome(intent, current_outcome);
    LOG_DEFAULT(LogLevel::INFO, "AITutorLoop: StrategyOutcome stored for Intent: " << CerebrumLux::to_string(intent) << ", Style: " << to_string(style));

    // 11. Öğrenme metriklerini güncelle
    // learnerMetrics.update(...); // TODO: LearningModule'ü güncel StrategyOutcome ile entegre et
    
    // 12. Öğrenci AI davranışını ayarla (placeholder)
    if (evaluation.score_overall / 100.0f < 0.7f) {
        // studentAI.adjustBehavior(evaluation); // TODO: StudentAI'de adjustBehavior methodu implemente edilmeli
        LOG_DEFAULT(LogLevel::INFO, "AITutorLoop: StudentAI'nin davranışı ayarlanıyor (düşük performanstan dolayı).");
    }

    // 13. Kararı GUI'ye ilet (A1.2) - TeachingDecision
    TeachingDecision decision = {
        intent,
        student_level,
        style,
        explain_decision(intent, student_level, style)
    };
    emit tutorDecisionUpdated(decision);
    LOG_DEFAULT(LogLevel::INFO, "AITutorLoop: Teaching decision emitted to GUI. Rationale: " << decision.rationale);

    // 14. Pedagojik Raporu GUI'ye ilet (C seçeneği)
    ChatPedagogyReport pedagogy_report = {
        style,
        estimated_learning_outcome,
        (float)evaluation.score_overall / 100.0f, // Güven = overall score
        explain_decision(intent, student_level, style) // Kısa açıklama, şimdilik aynı
    };
    emit pedagogyReportUpdated(pedagogy_report);
    LOG_DEFAULT(LogLevel::INFO, "AITutorLoop: Pedagogy Report emitted to GUI. Outcome: " << to_string(pedagogy_report.estimatedOutcome));
}

void AITutorLoop::onIntentLearned(int intentId) {
    // Bu slot, IntentLearnerQtBridge'den gelen sinyali yakalar.
    // Şimdilik sadece logluyoruz. İleride bu bilgiye göre
    // ders döngüsünü veya stratejiyi ayarlayabiliriz.
    LOG_DEFAULT(LogLevel::INFO, "AITutorLoop: IntentLearned sinyali alındı. Intent ID: " << intentId);
}

} // namespace CerebrumLux
