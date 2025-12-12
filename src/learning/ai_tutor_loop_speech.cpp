// ai_tutor_loop_speech.cpp
// Single-file: speech curriculum + teacher-student loop for CerebrumLux
// Requires: nlohmann/json.hpp (https://github.com/nlohmann/json)
// Build: add to your CMake and link header-only nlohmann_json.

#include <iostream>
#include <thread>
#include <mutex>
#include <queue>
#include <unordered_map>
#include <functional>
#include <atomic>
#include <chrono>
#include <fstream>
#include <optional>
#include <sstream>
#include <random>
#include <algorithm>
#include "nlohmann/json.hpp"

using json = nlohmann::json;
using namespace std::chrono_literals;

// ---------------- Utilities ----------------
static std::string now_ts() {
    using namespace std::chrono;
    auto t = system_clock::now();
    auto s = duration_cast<milliseconds>(t.time_since_epoch()).count();
    return std::to_string(s);
}
static std::string uuid4() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<uint32_t> dis;
    std::ostringstream ss;
    ss << std::hex;
    for (int i=0;i<4;i++) ss<<dis(gen);
    return ss.str();
}
static std::string lower_copy(const std::string &s) {
    std::string r = s; std::transform(r.begin(), r.end(), r.begin(), ::tolower); return r;
}

void logi(const std::string &s){ std::cout<<"["<<now_ts()<<"] "<<s<<"\n"; }

// ---------------- Message struct (JSON/struct format) ----------------
struct Msg {
    std::string msg_id;
    std::string from;
    std::string to;
    std::string type; // lesson|prompt|answer|eval|hint|meta
    json payload;
    json meta;
    json to_json() const {
        json j;
        j["msg_id"]=msg_id; j["from"]=from; j["to"]=to; j["type"]=type; j["payload"]=payload; j["meta"]=meta;
        return j;
    }
    static Msg from_json(const json &j) {
        Msg m;
        m.msg_id = j.value("msg_id",""); m.from=j.value("from",""); m.to=j.value("to","");
        m.type = j.value("type",""); m.payload = j.value("payload", json::object()); m.meta = j.value("meta", json::object());
        return m;
    }
};

// ---------------- Thread-safe queue ----------------
template<typename T>
class TSQueue {
    std::queue<T> q; std::mutex m;
public:
    void push(const T &v){ std::lock_guard lk(m); q.push(v); }
    std::optional<T> try_pop() {
        std::lock_guard lk(m);
        if (q.empty()) return std::nullopt;
        T v=q.front(); q.pop(); return v;
    }
    bool empty(){
        std::lock_guard lk(m); return q.empty(); }
};

// ---------------- Speech Curriculum (simple table) ----------------
struct SpeechLessonTemplate {
    std::string lesson_id;
    std::string title;
    std::string prompt;         // teacher prompt to student
    std::vector<std::string> expectations; // keywords/behaviors expected
    int difficulty; // 1..5
    int level; // A1..C2 mapping numeric
};

static std::vector<SpeechLessonTemplate> build_default_speech_curriculum() {
    return {
        {"S001", "Greeting - friendly", "Greet the user in a friendly short sentence.", {"hello","hi","merhaba"}, 1, 1},
        {"S002", "Greeting - formal", "Greet user formally (resmi).", {"merhaba","sayın","hoşgeldiniz"}, 1, 1},
        {"S010", "Short concise answer", "Answer the question in one short sentence.", {"one sentence","concise"}, 1, 1},
        {"S020", "Ask clarifying question", "If user's intent is ambiguous, ask a clarifying question.", {"nasıl","hangi","hangi amaçla"}, 2, 2},
        {"S030", "Empathetic response", "Provide empathetic reply to user frustration.", {"üzgünüm","anlıyorum","yardımcı olabilirim"}, 3, 3},
        {"S040", "Technical explanation", "Explain a simple technical concept in 3-5 sentences.", {"örnek","adım adım","açıkla"}, 3, 3},
        {"S100", "Plan generation", "Given a user task, produce a 3-step actionable plan.", {"adım","ilk","son"}, 4, 4},
        {"S200", "Assistant behavior", "Summarize user's request and propose next action.", {"özet","sonra ne yapılacak","öneri"}, 5, 5}
    };
}

// ---------------- TeacherInvoker ----------------
// Produces lesson JSON for speech curriculum. In production, replace scaffold with LLaMA-generated content.
class TeacherInvoker {
public:
    TeacherInvoker() {
        curriculum = build_default_speech_curriculum();
    }

    // Produce a lesson JSON for a given lesson_id (or pick next by level)
    json ask_teacher_for_lesson(const std::string &lesson_id = "", int level_hint=1) {
        // find by id, else pick by level_hint
        SpeechLessonTemplate tmpl;
        bool found=false;
        if (!lesson_id.empty()) {
            for (auto &t: curriculum) if (t.lesson_id==lesson_id) { tmpl=t; found=true; break; }
        }
        if (!found) {
            // pick first with level >= level_hint
            for (auto &t: curriculum) if (t.level >= level_hint) { tmpl=t; found=true; break; }
        }
        if (!found && !curriculum.empty()) tmpl=curriculum[0];

        json out;
        out["lesson_id"] = tmpl.lesson_id;
        out["title"] = tmpl.title;
        out["prompt"] = tmpl.prompt;
        out["expectations"] = tmpl.expectations;
        out["difficulty"] = tmpl.difficulty;
        out["level"] = tmpl.level;
        out["timestamp"] = now_ts();

        // Provide an "ideal" sample response to score against (teacher's reference)
        out["ideal"] = generate_ideal_response(tmpl);
        return out;
    }

    // Simplified: teacher can also generate hints (text) based on eval report
    json ask_teacher_for_hint(const json &lesson, const json &eval_report) {
        json hint;
        hint["lesson_id"] = lesson.value("lesson_id", "unknown");
        hint["hint_text"] = generate_hint(lesson, eval_report);
        hint["timestamp"] = now_ts();
        return hint;
    }

private:
    std::vector<SpeechLessonTemplate> curriculum;

    std::string generate_ideal_response(const SpeechLessonTemplate &t) {
        // For demo, create canned ideal responses per lesson id (in production: LLaMA)
        if (t.lesson_id=="S001") return "Merhaba! Sana nasıl yardımcı olabilirim?";
        if (t.lesson_id=="S002") return "Merhaba, hoş geldiniz. Size nasıl yardımcı olabilirim?";
        if (t.lesson_id=="S010") return "Evet, toplantı 10:00'da.";
        if (t.lesson_id=="S020") return "Bu isteği hangi amaçla kullanmak istiyorsunuz?";
        if (t.lesson_id=="S030") return "Üzgünüm böyle hissettiğin için. Konuyu birlikte çözebiliriz.";
        if (t.lesson_id=="S040") return "Bu, bellek yönetiminden dolayı olur; öncelikle kaynakları serbest bırakmalısınız. Adım 1: ...";
        if (t.lesson_id=="S100") return "1) İhtiyacı tanımla. 2) Gereken araçları hazırla. 3) Uygula ve test et.";
        if (t.lesson_id=="S200") return "İsteğin: rapor özeti çıkarma. Öneri: önce metni analiz et, sonra 3 maddede özetle.";
        return "Bu konu hakkında kısa ve net bilgi veriniz.";
    }

    std::string generate_hint(const json &lesson, const json &eval_report) {
        // Look at report and return short hint
        int score = eval_report.value("score", 0);
        if (score > 80) return "Gayet iyi, sadece daha doğal bir geçiş cümlesi ekleyebilirsin.";
        if (score > 60) return "Cümle akışı iyi; biraz daha empati tonu ekle.";
        return "Kısa tutmaya çalış; ayrıca doğrudan kullanıcıya soru sorun ve nazik bir giriş ekleyin.";
    }
};

// ---------------- StudentAgent ----------------
// In production: this should call out to LLaMA / your existing llm invocation.
// Here: we provide a simple templated answer and a hook 'user_callback' to plug real LLM.
class StudentAgent {
public:
    StudentAgent() : student_level(1) {}

    // Hook: set function to call real LLM (prompt -> response)
    void set_llm_call(std::function<std::string(const std::string&)> cb) { llm_call = cb; }

    json produce_answer(const json &lesson) {
        json ans;
        std::string prompt = lesson.value("prompt", "");
        // If llm_call is provided, delegate
        if (llm_call) {
            std::string llm_input = build_llm_prompt(lesson);
            std::string resp = "";
            try {
                resp = llm_call(llm_input); // synchronous call expected
            } catch (const std::exception& e) {
                logi("StudentAgent LLM call failed: " + std::string(e.what()));
            }
            ans["text"] = resp;
            ans["generated_by"] = "llm";
        } else {
            // fallback canned generation (deterministic simple)
            ans["text"] = canned_answer_for(lesson.value("lesson_id", ""));
            ans["generated_by"] = "template";
        }
        ans["student_level"] = student_level;
        ans["generated_at"] = now_ts();
        return ans;
    }

    void apply_feedback_and_improve(const json &eval_report) {
        int score = eval_report.value("score",0);
        // naive improvement: if passed, increase level
        if (score >= 70) student_level = std::min(5, student_level + 1);
    }

    int current_level() const { return student_level; }

private:
    int student_level;
    std::function<std::string(const std::string&)> llm_call;

    std::string build_llm_prompt(const json &lesson) {
        std::ostringstream ss;
        ss << "Lesson: " << lesson.value("title","") << "\n";
        ss << "Instruction: " << lesson.value("prompt","") << "\n";
        ss << "Expectations: ";
        for (auto &e : lesson["expectations"]) ss << e.get<std::string>() << "; ";
        ss << "\nReply with a natural-sounding short response.";
        return ss.str();
    }

    std::string canned_answer_for(const std::string &id) {
        if (id=="S001") return "Merhaba! Nasılsınız? Size nasıl yardımcı olabilirim?";
        if (id=="S002") return "Merhaba, hoş geldiniz. Size nasıl yardımcı olabilirim?";
        if (id=="S010") return "Evet, toplantı bugün 10:00'da.";
        if (id=="S020") return "Bu isteği hangi amaçla kullanmak istiyorsunuz?";
        if (id=="S030") return "Üzgünüm böyle hissettiğiniz için; birlikte çözebiliriz.";
        if (id=="S040") return "Bellek yönetimi açısından kaynakları serbest bırakmayı deneyin; adım adım açıklayayım.";
        if (id=="S100") return "1) İhtiyacı belirleyin. 2) Araçları hazırlayın. 3) Uygulayın ve test edin.";
        if (id=="S200") return "Anladım: rapor özeti isteniyor. Önerim: önce metni analiz edip üç madde ile özetleyin.";
        return "Kısa ve net cevap: elbette, yardımcı olabilirim.";
    }
};

// ---------------- Evaluator (speech scoring) ----------------
class Evaluator {
public:
    Evaluator() {}

    // Returns a structured eval report with component scores and final 0..100 score
    json evaluate_speech(const json &lesson, const json &answer) {
        json report;
        std::string text = answer.value("text", "");
        std::string ideal = lesson.value("ideal", "");
        std::vector<std::string> expectations;
        if (lesson.contains("expectations") && lesson["expectations"].is_array()) {
            for (auto &e: lesson["expectations"]) expectations.push_back(e.get<std::string>());
        }

        int nat = score_naturalness(text);
        int ctx = score_context_preservation(text, lesson);
        int tone = score_tone(text, lesson);
        int lenc = score_length_control(text, lesson);
        int expl = score_explanation_quality(text);
        int corr = score_correction_applicability(text, answer);
        int intent = score_intent_understanding(text, expectations);

        // weight components per earlier spec (sum weights to 100)
        // Doğallık(20), Bağlam(10), Ton(10), Uzunluk(10), Açıklama(10), HataDüzeltme(20), Niyet(20)
        int final_score =
            (int)(nat * 0.20f) +
            (int)(ctx * 0.10f) +
            (int)(tone * 0.10f) +
            (int)(lenc * 0.10f) +
            (int)(expl * 0.10f) +
            (int)(corr * 0.20f) +
            (int)(intent * 0.20f);

        report["component_scores"] = {
            {"naturalness", nat},
            {"context", ctx},
            {"tone", tone},
            {"length_control", lenc},
            {"explanation", expl},
            {"correction", corr},
            {"intent", intent}
        };
        report["score"] = final_score;
        report["passed"] = (final_score >= 70);
        report["evaluated_at"] = now_ts();
        return report;
    }

private:
    // Heuristics / placeholders. Replace with ML-based or LLM-based evaluator for production.

    int score_naturalness(const std::string &t) {
        if (t.empty()) return 0;
        // heuristic: average sentence length and punctuation presence
        int sentences = std::count(t.begin(), t.end(), '.') + std::count(t.begin(), t.end(), '!') + std::count(t.begin(), t.end(), '?');
        if (sentences == 0) sentences = 1;
        int words = 0;
        std::istringstream iss(t);
        std::string w;
        while (iss >> w) words++;
        if (words == 0) return 0;
        int avg = words / sentences;
        // good avg between 6 and 20
        if (avg < 4) return 60;
        if (avg <= 20) return 100;
        return 80;
    }

    int score_context_preservation(const std::string &t, const json &lesson) {
        // check presence of keywords from lesson prompt or ideal
        std::string ideal = lesson.value("ideal", "");
        int match = 0;
        auto tokens = split_words(lower_copy(ideal));
        auto txtokens = split_words(lower_copy(t));
        if (tokens.empty()) return 100;
        int relevant_tokens = 0;
        for (auto &tk : tokens) {
            if (tk.size() < 3) continue; // ignore very short tokens
            relevant_tokens++;
            if (std::find(txtokens.begin(), txtokens.end(), tk) != txtokens.end()) match++;
        }
        if (relevant_tokens == 0) return 100;
        return std::min(100, (match * 100) / relevant_tokens);
    }

    int score_tone(const std::string &t, const json &lesson) {
        // crude tone check: presence of polite words
        std::string lower = lower_copy(t);
        int polite = 0;
        std::vector<std::string> pol = {"lütfen","rica","teşekkür","memnun","özür","özür dilerim","üzgünüm"};
        for (auto &p: pol) if (lower.find(p) != std::string::npos) polite++;
        if (polite >= 2) return 100;
        if (polite == 1) return 70;
        return 50;
    }

    int score_length_control(const std::string &t, const json &lesson) {
        // if instruction says short -> prefer <=1 sentence
        std::string instr = lesson.value("prompt", "");
        bool want_short = instr.find("short") != std::string::npos || instr.find("kısa") != std::string::npos;
        int sentences = std::count(t.begin(), t.end(), '.') + std::count(t.begin(), t.end(), '!') + std::count(t.begin(), t.end(), '?');
        if(sentences == 0 && !t.empty()) sentences = 1;

        if (want_short) {
            if (sentences <= 1) return 100;
            if (sentences == 2) return 60;
            return 20;
        } else {
            if (sentences >= 2 && sentences <= 6) return 100;
            if (sentences == 1) return 60;
            return 40;
        }
    }

    int score_explanation_quality(const std::string &t) {
        // simple heuristic: presence of connective words and examples
        std::string lower = lower_copy(t);
        int conn = 0;
        std::vector<std::string> con = {"çünkü","örneğin","mesela","bu yüzden","ayrıca","öncelikle","sonuç olarak","adım"};
        for (auto &c: con) if (lower.find(c) != std::string::npos) conn++;
        return std::min(100, conn * 40); // 0, 40, 80, 100...
    }

    int score_correction_applicability(const std::string &t, const json &answer) {
        // crude: if student indicates attempt to fix (contains 'düzelt' or 'yeniden')
        std::string lower = lower_copy(answer.value("text",""));
        if (lower.find("düzelt") != std::string::npos || lower.find("yeniden") != std::string::npos) return 90;
        // otherwise modest
        return 50;
    }

    int score_intent_understanding(const std::string &t, const std::vector<std::string> &expectations) {
        // match expected keywords
        int found = 0;
        std::string lower = lower_copy(t);
        for (auto &e : expectations) {
            if (lower.find(lower_copy(e)) != std::string::npos) found++;
        }
        // scale to 0..100
        int total = expectations.size() ? (int)expectations.size() : 1;
        int sc = (int)((100.0f * found) / total);
        return sc;
    }

    // utility split
    std::vector<std::string> split_words(const std::string &s) {
        std::vector<std::string> out; std::istringstream iss(s); std::string w;
        while (iss >> w) {
            // remove punctuation
            w.erase(std::remove_if(w.begin(), w.end(), ::ispunct), w.end());
            out.push_back(w);
        }
        return out;
    }
};


// ---------------- TutorBroker (orchestrator) ----------------
class TutorBroker {
public:
    TutorBroker() : running(false) {
        teacher = std::make_unique<TeacherInvoker>();
        student = std::make_unique<StudentAgent>();
        evaluator = std::make_unique<Evaluator>();
    }
    ~TutorBroker(){ stop(); }

    // Optionally provide a real LLM callable function to the student agent
    void set_llm_callable(std::function<std::string(const std::string&)> cb) {
        student->set_llm_call(cb);
    }

    void start(bool auto_seed=true) {
        running = true;
        broker_thread = std::thread([this]() { this->broker_loop(); });
        if (auto_seed) seed_first_lesson();
    }
    void stop() {
        running = false;
        if (broker_thread.joinable()) broker_thread.join();
    }

private:
    std::unique_ptr<TeacherInvoker> teacher;
    std::unique_ptr<StudentAgent> student;
    std::unique_ptr<Evaluator> evaluator;
    TSQueue<Msg> q;
    std::thread broker_thread;
    std::atomic<bool> running;

    void seed_first_lesson() {
        Msg m; m.msg_id = uuid4(); m.from="broker"; m.to="teacher"; m.type="lesson_request";
        m.payload = { {"lesson_id","S001"}, {"level_hint", student->current_level()} };
        m.meta = { {"ts", now_ts()} };
        q.push(m);
    }

    void broker_loop() {
        logi("TutorBroker started (speech curriculum).");
        while (running) {
            auto mo = q.try_pop();
            if (!mo) { std::this_thread::sleep_for(50ms); continue; }
            handle_msg(*mo);
        }
        logi("TutorBroker stopped.");
    }

    void handle_msg(const Msg &m) {
        if (m.to == "teacher" && m.type == "lesson_request") {
            json lesson = teacher->ask_teacher_for_lesson(m.payload.value("lesson_id",""), m.payload.value("level_hint",1));
            Msg mt; mt.msg_id = uuid4(); mt.from="teacher"; mt.to="student"; mt.type="lesson"; mt.payload=lesson; mt.meta={{"ts",now_ts()}};
            q.push(mt);
            logi("Teacher sent lesson: " + lesson.value("title", "?"));
            return;
        }
        if (m.to == "student" && m.type == "lesson") {
            // ask student to produce answer (async)
            std::thread([this, m]() {
                json ans = student->produce_answer(m.payload);
                Msg ma; ma.msg_id=uuid4(); ma.from="student"; ma.to="evaluator"; ma.type="answer"; ma.payload={{"lesson", m.payload},{"answer", ans}}; ma.meta={{"ts",now_ts()}};
                q.push(ma);
                logi("Student sent answer for: " + m.payload.value("title", "?"));
            }).detach();
            return;
        }
        if (m.to == "evaluator" && m.type == "answer") {
            json lesson = m.payload["lesson"];
            json answer = m.payload["answer"];
            json report = evaluator->evaluate_speech(lesson, answer);

            if (answer.value("text", "").empty()) {
                logi("Student produced an empty answer. Skipping evaluation and asking for a hint.");
                report["passed"] = false;
            }

            Msg me; me.msg_id=uuid4(); me.from="evaluator"; me.to="broker"; me.type="eval"; me.payload={{"lesson",lesson},{"answer",answer},{"report",report}}; me.meta={{"ts",now_ts()}};
            q.push(me);
            logi("Evaluator sent report for: " + lesson.value("title", "?"));
            return;
        }
        if (m.to == "broker" && m.type == "eval") {
            json lesson = m.payload["lesson"];
            json answer = m.payload["answer"];
            json report = m.payload["report"];
            logi("Eval result: lesson="+lesson.value("lesson_id","?") + " score=" + std::to_string(report.value("score",0)));
            
            // Also log the detailed scores
            if(report.contains("component_scores")) {
                logi("Detailed scores: " + report["component_scores"].dump());
            }

            if (report.value("passed", false)) {
                // student improves
                student->apply_feedback_and_improve(report);
                logi("Student passed. New level: " + std::to_string(student->current_level()));
                // seed next lesson by level
                int new_level = student->current_level();
                // find next lesson of that level
                Msg mnext; mnext.msg_id=uuid4(); mnext.from="broker"; mnext.to="teacher"; mnext.type="lesson_request"; mnext.payload={{"lesson_id",""},{ "level_hint", new_level }};
                q.push(mnext);
            } else {
                // ask teacher for a hint and re-run
                json hint = teacher->ask_teacher_for_hint(lesson, report);
                // FIX: Orijinal dersi payload'a eklemezsek öğrenci neye cevap vereceğini unutur.
                hint["lesson"] = lesson; 
                Msg mh; mh.msg_id=uuid4(); mh.from="teacher"; mh.to="student"; mh.type="hint"; mh.payload = hint; mh.meta={{"ts",now_ts()}};
                q.push(mh);
                logi("Student failed. Teacher sent hint: " + hint.value("hint_text", ""));
            }
            return;
        }
        if (m.to == "student" && m.type == "hint") {
            // student receives hint and creates improved answer
            std::thread([this, m]() {
                // In a real design, we would pass hint to LLM as additional context
                 logi("Student received hint. Trying again.");
                
                // FIX: Hint payload'ından orijinal dersi çıkarıyoruz.
                // Yukarıdaki fix ile 'hint' objesi içine 'lesson' gömüldü.
                json original_lesson;
                if(m.payload.contains("lesson")) {
                    original_lesson = m.payload["lesson"];
                } else {
                    logi("CRITICAL ERROR: Lost lesson context in hint loop!"); 
                    return;
                }

                // Hint metnini de derse ek context olarak verebiliriz (Opsiyonel implementasyon)
                // std::string hint_text = m.payload.value("hint_text", "");
                
                json improved = student->produce_answer(original_lesson);
                Msg ma; ma.msg_id=uuid4(); ma.from="student"; ma.to="evaluator"; ma.type="answer"; ma.payload={{"lesson", m.payload},{"answer", improved}}; ma.meta={{"ts",now_ts()}};
                q.push(ma);
                logi("Student sent answer for: " + m.payload.value("title", "?"));
                logi("Student sent improved answer.");
            }).detach();
            return;
        }

        logi("Unhandled message: to="+m.to + " type="+m.type);
    }
};

// ---------------- Main (demo) ----------------
#ifdef AI_TUTOR_LOOP_SPEECH_DEMO
int main() {
    logi("Starting AI Tutor Loop Speech Demo");
    TutorBroker tb;
    // Example: bind real LLM call (synchronous) if available:
    // tb.set_llm_callable([](const std::string &prompt)->std::string {
    //    // call your llama/llm here and return text
    //    // return your_llama_infer(prompt);
    //    logi("LLM called with prompt:\n---\n" + prompt + "\n---");
    //    return "This is a placeholder response from the LLM.";
    // });
    tb.start(true);
    // let it run for some cycles
    logi("Running for 30 seconds...");
    std::this_thread::sleep_for(std::chrono::seconds(30));
    logi("Stopping demo.");
    tb.stop();
    return 0;
}
#endif

// If not demo compile, provide functions to be called from the main application.
// e.g. create global broker object and start/stop from CerebrumLux lifecycle.
