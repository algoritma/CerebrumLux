// src/ai_tutor/tutor_broker_router.h
#ifndef TUTOR_BROKER_ROUTER_H
#define TUTOR_BROKER_ROUTER_H

#include <string>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <atomic>
#include <thread>
#include <memory>
#include <optional>

#include "external/nlohmann/json.hpp"
#include "ai_tutor/curriculum.h"
#include "ai_tutor/teacher_ai.h"
#include "ai_tutor/student_ai.h"
#include <QObject>
#include <QMetaType>

using json = nlohmann::json;

// ---------------- Message struct ----------------
struct Msg {
    std::string id;
    std::string from;   // "gui"|"broker"|"teacher"|"student"|"router"|"intent"
    std::string to;     // target
    std::string type;   // "user_input"|"lesson_request"|"lesson"|"answer"|"eval"|"intent"
    json payload;       // freeform
    json meta;
};
Q_DECLARE_METATYPE(Msg)


// ---------------- Thread-safe queue ----------------
template<typename T>
class TSQueue {
    std::queue<T> q;
    std::mutex m;
    std::condition_variable cv;
public:
    void push(const T &v){ std::lock_guard lk(m); q.push(v); cv.notify_one(); }
    
    T wait_pop() {
        std::unique_lock lk(m);
        cv.wait(lk, [&]{ return !q.empty(); });
        T v = q.front(); q.pop(); return v;
    }
    
    bool try_pop(T &out) {
        std::lock_guard lk(m);
        if (q.empty()) return false;
        out = q.front(); q.pop(); return true;
    }
    
    bool empty(){ std::lock_guard lk(m); return q.empty(); }
};

// ---------------- IntentRouter ----------------
class IntentRouter {
public:
    void set_fasttext_callable(std::function<std::string(const std::string&)> cb) {
        fasttext_cb = cb;
    }
    void set_llm_classifier(std::function<std::string(const std::string&)> cb) {
        llm_classify_cb = cb;
    }

    std::string route(const std::string &user_text);

private:
    std::function<std::string(const std::string&)> fasttext_cb;
    std::function<std::string(const std::string&)> llm_classify_cb;

    static bool contains_any(const std::string &s, const std::initializer_list<std::string>& keys);
    static std::string lower_copy(const std::string &s);
};

// ---------------- TutorBroker ----------------
class TutorBroker : public QObject {
    Q_OBJECT
public:
    TutorBroker();
    ~TutorBroker();

    void set_external_llm(std::function<std::string(const std::string&)> llm_cb);
    void set_fasttext_classifier(std::function<std::string(const std::string&)> fcb);
    void set_llm_classifier(std::function<std::string(const std::string&)> cb);

    void push_user_input(const std::string &userid, const std::string &text);
    
    void start();
    void stop();

signals:
    void new_message_for_gui(const Msg& message);

private:
    std::unique_ptr<CerebrumLux::TeacherAI> teacher;
    std::unique_ptr<CerebrumLux::StudentAI> student;
    IntentRouter intent_router;
    TSQueue<Msg> inbox;
    std::atomic<bool> running;
    std::thread worker;
    std::function<std::string(const std::string&)> external_llm_cb;

    void loop();
    void handle_user_input(const Msg &m);
    void process_student_request(const Msg &req);
    void process_teacher_request(const Msg &req);
    void handle_internal(const Msg &m);
    void emit_to_gui(const Msg &m);

    static std::string now_ts();
    static std::string gen_id();
};

#endif // TUTOR_BROKER_ROUTER_H
