// tutor_broker_adapter.h
// Adapted TutorBroker that uses LlamaAdapter & TeacherEvaluator and supports async student/teacher calls.
// Provides a simple API for GUI bridge.

#ifndef TUTOR_BROKER_ADAPTER_H
#define TUTOR_BROKER_ADAPTER_H

#include "curriculum.h"
#include "teacher_evaluator.h"
#include <functional>
#include <string>
#include <atomic>
#include <thread> // Required for std::thread
#include <mutex> // Required for std::mutex
#include <deque> // Required for std::deque

struct BrokerMsg {
    std::string from;
    std::string type; // user_input/lesson_request/...
    std::string user;
    std::string text;
};

class TutorBrokerAdapter {
public:
    TutorBrokerAdapter();
    ~TutorBrokerAdapter();

    // set optional callbacks
    void set_gui_callback(std::function<void(const std::string& from, const std::string& type, const std::string& payload)> cb);

    // push user input (from GUI)
    void push_user_input(const std::string &user, const std::string &text);

    // start/stop loop
    void start();
    void stop();

private:
    void main_loop();
    void handle_user(const BrokerMsg &m);
    std::string decide_route(const std::string &text); // simple heuristics (or use FastText externally)

    std::function<void(const std::string& from, const std::string& type, const std::string& payload)> gui_cb;
    std::atomic<bool> running;
    std::thread worker;
    std::mutex q_mtx;
    std::deque<BrokerMsg> queue;
};

#endif // TUTOR_BROKER_ADAPTER_H
