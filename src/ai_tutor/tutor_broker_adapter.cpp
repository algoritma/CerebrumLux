// tutor_broker_adapter.cpp
#include "tutor_broker_adapter.h"
#include "teacher_ai.h"
#include "student_ai.h"
#include "llama_adapter.h"
#include <iostream>
#include <condition_variable>
#include <chrono> // Required for std::chrono
#include <algorithm> // Required for std::transform
#include <sstream> // Required for std::ostringstream

// Ensure CerebrumLux types are available
#include "teacher_evaluator.h" // For CerebrumLux::TeacherEvaluator and CerebrumLux::EvaluationResult
#include "student_ai.h" // For CerebrumLux::StudentAI
#include "teacher_ai.h" // For CerebrumLux::TeacherAI
#include "llama_adapter.h"  // For CerebrumLux::LlamaAdapter

TutorBrokerAdapter::TutorBrokerAdapter(): running(false) {}

TutorBrokerAdapter::~TutorBrokerAdapter(){ stop(); }

void TutorBrokerAdapter::set_gui_callback(std::function<void(const std::string&, const std::string&, const std::string&)> cb) {
    gui_cb = cb;
}

void TutorBrokerAdapter::push_user_input(const std::string &user, const std::string &text) {
    {
        std::lock_guard lk(q_mtx);
        queue.push_back({ "gui", "user_input", user, text });
    }
    // notify worker by starting thread if not running
    if (!running) start();
}

void TutorBrokerAdapter::start() {
    if (running) return;
    running = true;
    worker = std::thread(&TutorBrokerAdapter::main_loop, this);
}

void TutorBrokerAdapter::stop() {
    if (!running) return;
    running = false;
    if (worker.joinable()) worker.join();
}

void TutorBrokerAdapter::main_loop() {
    CerebrumLux::TeacherAI teacher; 
    CerebrumLux::StudentAI student; 
    while (running) {
        BrokerMsg m;
        {
            std::lock_guard lk(q_mtx);
            if (queue.empty()) {
                // idle sleep
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
                continue;
            }
            m = queue.front();
            queue.pop_front();
        }
        try {
            if (m.type == "user_input") handle_user(m);
            else {
                // handle other types...
            }
        } catch (const std::exception &ex) {
            std::cerr<<"Broker main loop exception: "<<ex.what()<<"\n";
        }
    }
}

std::string TutorBrokerAdapter::decide_route(const std::string &text) {
    // lightweight: check for technical tokens
    std::string s = text;
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);
    if (s.find("class ") != std::string::npos || s.find("std::") != std::string::npos || s.find("#include") != std::string::npos) return "student";
    if (s.find("teach") != std::string::npos || s.find("lesson") != std::string::npos) return "teacher";
    if (s.find("hello") != std::string::npos || s.find("merhaba") != std::string::npos) return "chat";
    return "student";
}

void TutorBrokerAdapter::handle_user(const BrokerMsg &m) {
    std::string route = decide_route(m.text);
    if (route == "student") {
        CerebrumLux::StudentAI student; 
        std::string resp = student.respond(m.text);

        if (gui_cb) gui_cb("student", "response", resp);

        std::thread([m, resp, this]() {
            std::string context = "Auto lesson context (student route)";
            auto opt = CerebrumLux::TeacherEvaluator::evaluate_response(context, resp);
            if (opt) {
                std::ostringstream out;
                out << "Score overall: " << opt->score_overall << " - feedback: " << opt->feedback;
                if (gui_cb) gui_cb("teacher", "evaluation", out.str());
            } else {
                if (gui_cb) gui_cb("teacher", "evaluation", std::string("Evaluation failed"));
            }
        }).detach();

    } else if (route == "teacher") {
        CerebrumLux::TeacherAI teacher; 
        std::string lesson = teacher.teach(m.text);
        if (gui_cb) gui_cb("teacher", "lesson", lesson);
    } else { // chat/external
        try {
            std::string chatresp = CerebrumLux::LlamaAdapter::infer_sync(m.text);
            if (gui_cb) gui_cb("external", "chat", chatresp);
        } catch (...) {
            if (gui_cb) gui_cb("external", "chat", std::string("No LLM bound"));
        }
    }
}
