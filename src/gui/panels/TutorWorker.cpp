#include "TutorWorker.h"
#include <QThread>
#include "../../learning/LearningModule.h" // EKLENDİ: learnFromText metodunu görebilmek için şart

TutorWorker::TutorWorker(CerebrumLux::LLMEngine* teacher, 
                         CerebrumLux::LLMEngine* student, 
                         CerebrumLux::LearningModule* learningModule, // YENİ: LearningModule
                         const Curriculum& curriculum, 
                         QObject* parent)
    : QObject(parent), m_teacher(teacher), m_student(student), 
      m_learningModule(learningModule), m_curriculum(curriculum), m_running(false)
{}

void TutorWorker::run() {
    m_running = true;
    emit update("Sistem başlatılıyor...");
    
    if (!m_teacher || !m_student) {
        emit update("HATA: Model pointerları geçersiz!");
        emit finished();
        return;
    }

    TeacherStudentFrame currentFrame;
    currentFrame.curriculum = m_curriculum;
    
    int cycle = 1;
    while (m_running) {
        // Döngüyü çalıştır (Bu bloklayıcı bir işlemdir)
        currentFrame = runTutorLoop(m_teacher, m_student, currentFrame);

        // Kullanıcı bu sırada durdur butonuna bastı mı?
        if (!m_running) {
            emit update(">>> Kullanıcı tarafından durduruldu.");
            break;
        }

        // Hata Kontrolü
        if (currentFrame.evaluation.feedback.find("CRITICAL") != std::string::npos ||
            currentFrame.evaluation.feedback.find("ERROR") != std::string::npos) 
        {
            emit update(QString("HATA TESPİT EDİLDİ: %1").arg(QString::fromStdString(currentFrame.evaluation.feedback)));
            emit update("Döngü durduruluyor.");
            break;
        }

        // Loglama
        QString log;
        log += QString("----------------------------------------\n");
        log += QString("DERS #%1\n").arg(cycle);
        log += QString("HEDEF: %1\n").arg(QString::fromStdString(currentFrame.lesson.goal));
        log += QString("GÖREV: %1\n").arg(QString::fromStdString(currentFrame.lesson.task));
        log += QString("CEVAP: %1\n").arg(QString::fromStdString(currentFrame.student_response));
        log += QString("DEĞERLENDİRME: %1\n").arg(QString::fromStdString(currentFrame.evaluation.feedback));
        
        QString scores = "PUANLAR: ";
        for(const auto& [k, v] : currentFrame.evaluation.scores) {
            scores += QString("%1: %2 | ").arg(QString::fromStdString(k)).arg(v);
        }
        log += scores + "\n";
        
        emit update(log);

        // YENİ: Başarılı ise Öğrenme Modülüne Kaydet (Persistence)
        // Feedback içinde "passed" kelimesi geçiyorsa veya ortalama skor yüksekse (basit heuristic)
        // Not: AI_Tutor_Loop JSON yapısında doğrudan "passed" alanı yok, feedback'e bakacağız.
        // Daha sağlam bir yöntem: evaluation.scores ortalamasını almak.
        float total_score = 0;
        for(auto const& [k, v] : currentFrame.evaluation.scores) total_score += v;
        float avg_score = currentFrame.evaluation.scores.empty() ? 0 : total_score / currentFrame.evaluation.scores.size();

        if (avg_score > 0.85f && m_learningModule) {
            std::string content = "Q: " + currentFrame.lesson.task + "\nA: " + currentFrame.student_response;
            std::string topic = "SelfTraining_" + currentFrame.lesson.goal; // Topic'i dersten türet
            
            m_learningModule->learnFromText(content, "AI_Tutor", topic, avg_score);
            emit update(">>> [LEARNING] Başarılı cevap bilgi tabanına kaydedildi.");
        }

        cycle++;
        
        // GUI event loop'un nefes alması için kısa bekleme
        QThread::msleep(2000); 
    }

    emit finished();
}

void TutorWorker::stop() {
    m_running = false;
}