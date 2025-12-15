// src/ai_tutor/tutor_broker_router.cpp
#include "tutor_broker_router.h"
#include <iostream>
#include <chrono>
#include <algorithm>
#include <cctype>

using namespace std::chrono_literals;

// ---------------- IntentRouter Implementation ----------------

bool IntentRouter::contains_any(const std::string &s, const std::initializer_list<std::string>& keys){
    for (const auto &k: keys) if (s.find(k) != std::string::npos) return true;
    return false;
}

std::string IntentRouter::lower_copy(const std::string &s) {
    std::string r = s;
    std::transform(r.begin(), r.end(), r.begin(),
                   [](unsigned char c){ return std::tolower(c); });
    return r;
}

std::string IntentRouter::route(const std::string &user_text) {
    // 1) try fasttext if available (very fast)
    if (fasttext_cb) {
        std::string label = fasttext_cb(user_text);
        if (label == "intent_code" || label == "intent_ask_code") return "student";
        if (label == "intent_teach" || label == "intent_curriculum") return "teacher";
        if (label == "intent_chat") return "external_llm";
        if (label == "intent_gui_action") return "hand_off_to_gui";
    }
    // 2) fallback lightweight heuristics
    std::string lower = lower_copy(user_text);
    if (contains_any(lower, {"compile","build","segmentation","memory","pointer","template"})) return "student";
    if (contains_any(lower, {"teach","curriculum","lesson","explain","how to"} )) return "teacher";
    if (contains_any(lower, {"hi","hello","merhaba","how are you","selam"})) return "external_llm";
    // 3) if llm classifier available, use it
    if (llm_classify_cb) {
        std::string label = llm_classify_cb(user_text);
        if (!label.empty()) {
            if (label=="student") return "student";
            if (label=="teacher") return "teacher";
            if (label=="chat") return "external_llm";
        }
    }
    // default: student for technical queries, otherwise external_llm
    bool hasTechnical = contains_any(lower, {"class ","int ","std::","template","->","::","#include"});
    return hasTechnical ? "student" : "external_llm";
}


// ---------------- TutorBroker Implementation ----------------

TutorBroker::TutorBroker()
    : QObject(nullptr), running(false)
{
    teacher = std::make_unique<CerebrumLux::TeacherAI>();
    student = std::make_unique<CerebrumLux::StudentAI>();
}

TutorBroker::~TutorBroker() {
    stop();
}

void TutorBroker::set_external_llm(std::function<std::string(const std::string&)> llm_cb) {
    external_llm_cb = llm_cb;
}

void TutorBroker::set_fasttext_classifier(std::function<std::string(const std::string&)> fcb) {
    intent_router.set_fasttext_callable(fcb);
}

void TutorBroker::set_llm_classifier(std::function<std::string(const std::string&)> cb) {
    intent_router.set_llm_classifier(cb);
}

void TutorBroker::push_user_input(const std::string &userid, const std::string &text) {
    Msg m;
    m.id = gen_id();
    m.from = "gui";
    m.to = "router";
    m.type = "user_input";
    m.payload["user"] = userid;
    m.payload["text"] = text;
    m.meta["ts"] = now_ts();
    inbox.push(m);
}

void TutorBroker::start() {
    if(running) return;
    running = true;
    worker = std::thread([this](){ this->loop(); });
}

void TutorBroker::stop() {
    if(!running) return;
    running = false;
    inbox.push(Msg()); // Push an empty message to wake up the worker thread
    if (worker.joinable()) {
        worker.join();
    }
}

void TutorBroker::loop() {
    while (running) {
        Msg m = inbox.wait_pop();
        if (!running) break;
        try {
            if (m.to == "router" && m.type == "user_input") {
                handle_user_input(m);
            } else {
                handle_internal(m);
            }
        } catch (const std::exception &ex) {
            std::cerr << "Broker error: " << ex.what() << "\n";
            Msg err_msg;
            err_msg.id = gen_id();
            err_msg.from = "broker";
            err_msg.to = "gui";
            err_msg.type = "error";
            err_msg.payload["text"] = std::string("Broker error: ") + ex.what();
            emit_to_gui(err_msg);
        }
    }
}

void TutorBroker::handle_user_input(const Msg &m) {
    std::string text = m.payload.value("text", "");
    std::string user = m.payload.value("user", "user0");
    std::string route_target = intent_router.route(text);

    if (route_target == "student") {
        Msg req; req.id = gen_id(); req.from = "broker"; req.to = "student"; req.type = "ask_student";
        req.payload["text"] = text; req.meta["user"] = user;
        process_student_request(req);
    } else if (route_target == "teacher") {
        Msg req; req.id = gen_id(); req.from = "broker"; req.to = "teacher"; req.type = "ask_teacher";
        req.payload["text"] = text; req.meta["user"] = user;
        process_teacher_request(req);
    } else if (route_target == "external_llm") {
        if (external_llm_cb) {
            std::string resp = external_llm_cb(text);
            Msg out; out.id=gen_id(); out.from="external_llm"; out.to="gui"; out.type="user_response";
            out.payload["text"] = resp; out.meta["user"]=user;
            emit_to_gui(out);
        } else {
            process_teacher_request(Msg{gen_id(),"broker","teacher","ask_teacher", json{{"text",text}}, json{{"user",user}}});
        }
    } else { // hand_off_to_gui or unknown
        Msg out; out.id=gen_id(); out.from="broker"; out.to="gui"; out.type="info";
        out.payload["text"] = std::string("Unhandled route: ") + route_target; out.meta["user"]=user;
        emit_to_gui(out);
    }
}

void TutorBroker::process_student_request(const Msg &req) {
    std::string prompt = req.payload.value("text", "");
    std::string student_resp = student->respond(prompt);

    Msg out; out.id = gen_id(); out.from="student"; out.to="gui"; out.type="user_response";
    out.payload["text"] = student_resp; out.meta = req.meta;
    emit_to_gui(out);

    std::string eval = teacher->evaluate(student_resp);
    Msg ev; ev.id=gen_id(); ev.from="teacher"; ev.to="gui"; ev.type="evaluation";
    ev.payload["text"] = eval; ev.meta = req.meta;
    emit_to_gui(ev);
}

void TutorBroker::process_teacher_request(const Msg &req) {
    std::string text = req.payload.value("text", "");
    std::string lesson = teacher->teach(text); // 'text' might be a section title or a question
    Msg out; out.id=gen_id(); out.from="teacher"; out.to="gui"; out.type="lesson";
    out.payload["text"] = lesson; out.meta = req.meta;
    emit_to_gui(out);
}

void TutorBroker::handle_internal(const Msg &m) {
    (void)m;
}

void TutorBroker::emit_to_gui(const Msg &m) {
    emit new_message_for_gui(m);
}

std::string TutorBroker::now_ts() {
    using namespace std::chrono;
    auto t = system_clock::now();
    auto s = duration_cast<milliseconds>(t.time_since_epoch()).count();
    return std::to_string(s);
}

std::string TutorBroker::gen_id() {
    static std::atomic<uint64_t> counter(1);
    uint64_t v = counter.fetch_add(1);
    return "m" + std::to_string(v);
}

