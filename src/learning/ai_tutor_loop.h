// src/learning/ai_tutor_loop.h

#ifndef AI_TUTOR_LOOP_H
#define AI_TUTOR_LOOP_H

#include <string>
#include <thread>
#include <mutex>
#include <queue>
#include <atomic>
#include <optional>
#include <memory>

// nlohmann/json.hpp başlık dosyasının projenizin include yollarında
// erişilebilir olduğu varsayılmaktadır. Genellikle vcpkg veya doğrudan
// proje dizini üzerinden eklenir.
#include "nlohmann/json.hpp"

using json = nlohmann::json;

// ---------- Mesaj Yapısı ----------
// Farklı bileşenler (Teacher, Student, Broker) arasındaki iletişimi sağlar.
struct Msg {
    std::string msg_id;
    std::string from;
    std::string to;
    std::string type;
    json payload;
    json meta;
};

// ---------- Thread-Safe Kuyruk ----------
// Eşzamanlı çalışan bileşenler arasında mesajların güvenli bir şekilde
// iletilmesi için kullanılır.
template<typename T>
class TSQueue {
public:
    void push(const T& v) {
        std::lock_guard<std::mutex> lk(m);
        q.push(v);
    }
    std::optional<T> try_pop() {
        std::lock_guard<std::mutex> lk(m);
        if (q.empty()) {
            return std::nullopt;
        }
        T v = q.front();
        q.pop();
        return v;
    }
    bool empty() {
        std::lock_guard<std::mutex> lk(m);
        return q.empty();
    }
private:
    std::queue<T> q;
    std::mutex m;
};

namespace CerebrumLux {
    class NaturalLanguageProcessor; // Forward declaration
    class KnowledgeBase;            // Forward declaration
}

// ---------- Aktörler (Bileşenler) ----------

// TeacherInvoker: LLaMA modelini sarmalayarak ders ve ipuçları üretir.
class TeacherInvoker {
public:
    TeacherInvoker(CerebrumLux::NaturalLanguageProcessor* nlp);
    json ask_teacher(const json &request);
private:
    CerebrumLux::NaturalLanguageProcessor* nlp_; // Gerçek NLP motoruna işaretçi
};

// StudentAgent: CerebrumLux'ın öğrenen iç ajanıdır, derslere cevap üretir.
class StudentAgent {
public:
    StudentAgent(CerebrumLux::NaturalLanguageProcessor* nlp, CerebrumLux::KnowledgeBase* kb);
    json produce_answer(const json &lesson);
    void upgrade_level();
    int get_level() const;
private:
    int level;
    CerebrumLux::NaturalLanguageProcessor* nlp_; // Gerçek NLP motoruna işaretçi
    CerebrumLux::KnowledgeBase* kb_; // Gerçek bilgi tabanına işaretçi
};

// Evaluator: Öğrencinin cevaplarını statik olarak veya sanal ortamda değerlendirir.
class Evaluator {
public:
    Evaluator();
    json evaluate(const json &answer, const json &lesson);
};

// ---------- TutorBroker ----------
// Tüm öğretim döngüsünü yöneten merkezi orkestratör.
class TutorBroker {
public:
    TutorBroker(CerebrumLux::NaturalLanguageProcessor* nlp, CerebrumLux::KnowledgeBase* kb);
    ~TutorBroker();
    void start();
    void stop();

private:
    void broker_loop();
    void handle_msg(const Msg &m);
    void push_teacher_request(const std::string &lesson_id, const std::string &goal);

    CerebrumLux::NaturalLanguageProcessor* nlp_;
    CerebrumLux::KnowledgeBase* kb_;
    std::unique_ptr<TeacherInvoker> teacher;
    std::unique_ptr<StudentAgent> student;
    std::unique_ptr<Evaluator> evaluator;
    TSQueue<Msg> inq;
    std::thread broker_thread;
    std::atomic<bool> running;
};

#endif // AI_TUTOR_LOOP_H