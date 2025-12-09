// src/learning/ai_tutor_loop.cpp

#include "ai_tutor_loop.h"
#include <iostream>
#include <chrono>
#include <sstream>
#include <random>
#include <vector>

// Gerekli başlıklar `ai_tutor_loop.h` içinde zaten mevcut.
using json = nlohmann::json;
using namespace std::chrono_literals;

// ---------- Yardımcı Fonksiyonlar ----------
namespace { // İsimsiz namespace, yardımcıların bu dosyaya özel kalmasını sağlar.
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
        for (int i=0; i<4; i++) ss << dis(gen);
        return ss.str();
    }

    void logi(const std::string &s) {
        // TODO: Projenin kendi loglama sistemiyle (src/core/logger.h) değiştirin.
        std::cout << "[" << now_ts() << "] [TUTOR_LOOP] " << s << std::endl;
    }
}

#include "../communication/natural_language_processor.h" // Gerçek sınıflar için
#include "../learning/KnowledgeBase.h"

// ---------- TeacherInvoker Gerçekleştirimi ----------
TeacherInvoker::TeacherInvoker(CerebrumLux::NaturalLanguageProcessor* nlp) : nlp_(nlp) {
    logi("TeacherInvoker, gerçek NLP motoruyla başlatıldı.");
}

json TeacherInvoker::ask_teacher(const json &request) {
    logi("Öğretmen, hedef için ders hazırlıyor: " + request.value("goal", ""));
    std::string goal = request.value("goal", "Implement sum in C++");

    // LLaMA'ya gönderilecek prompt'u, net bir şekilde JSON formatı istediğimizi belirterek güncelliyoruz.
    std::string prompt = R"(
Bir AI öğrencisine ders vermek için JSON formatında bir ders materyali hazırla.
Konu: ")" + goal + R"(".
JSON çıktısı MUTLAKA şu alanları içermelidir: "content" (Markdown formatında açıklama), "code" (çözülecek kod iskeleti), ve "tests" (öğrenci kodunu test edecek bir JSON dizisi).
"tests" dizisindeki her test objesi "id", "type", "fn", "args" ve "expected" alanlarını içermelidir.

Örnek JSON:
{
  "content": "Bir C++ fonksiyonu yazarak bir tamsayı vektörünün toplamını hesaplayın. Aralık tabanlı bir for döngüsü kullanın.",
  "code": "int sum(const std::vector<int>& a) {\n  // TODO: Implement here\n}",
  "tests": [
    {
      "id": "t1",
      "type": "call",
      "fn": "sum",
      "args": [[1, 2, 3]],
      "expected": 6
    }
  ]
}
)";

    json out;
    out["lesson_id"] = request.value("lesson_id", "L-" + uuid4());
    out["step"] = request.value("step", 1);

    if (nlp_) {
        std::string llama_response_str = nlp_->generate_simple_response(prompt);
        try {
            json llama_response = json::parse(llama_response_str);
            out["content"] = llama_response.value("content", "Açıklama üretilemedi.");
            out["code"] = llama_response.value("code", "// Kod iskeleti üretilemedi.");
            out["tests"] = llama_response.value("tests", json::array());
            logi("LLaMA'dan ders başarıyla üretildi.");
        } catch (const json::parse_error& e) {
            logi("LLaMA yanıtı JSON olarak ayrıştırılamadı: " + std::string(e.what()));
            logi("Ham yanıt: " + llama_response_str);
            // Fallback: Yanıtı olduğu gibi content'e koy
            out["content"] = "LLaMA'dan gelen yanıt işlenemedi. Ham yanıt: \n" + llama_response_str;
            out["code"] = "// Hata";
            out["tests"] = json::array();
        }
    } else {
        // NLP motoru yoksa, eski usul fallback
        logi("NLP motoru mevcut değil, fallback ders kullanılıyor.");
        out["content"] = "Bir C++ fonksiyonu `int sum(const std::vector<int>& a)` gerçekleştirin.";
        out["code"] = "```cpp\nint sum(const std::vector<int>& a) {\n    // TODO: Mantığı buraya uygulayın.\n}\n```";
        out["tests"] = json::array({
            {{"id", "t1"}, {"type", "call"}, {"fn", "sum"}, {"args", {{1, 2, 3}}}, {"expected", 6}}
        });
    }
    
    out["teacher_generated_at"] = now_ts();
    return out;
}

// ---------- StudentAgent Gerçekleştirimi ----------
StudentAgent::StudentAgent(CerebrumLux::NaturalLanguageProcessor* nlp, CerebrumLux::KnowledgeBase* kb) 
    : level(0), nlp_(nlp), kb_(kb) {
    logi("StudentAgent, gerçek NLP motoru ve Bilgi Tabanı ile başlatıldı.");
}

json StudentAgent::produce_answer(const json &lesson) {
    logi("Öğrenci, ders için cevap üretiyor: " + lesson.value("lesson_id", "?"));
    json ans;
    std::string content = lesson.value("content", "");
    std::string code_scaffold = lesson.value("code", "");
    std::string explanation;

    // Akıllı Çözüm Yolu (NLP ve KB kullanarak)
    if (nlp_ && kb_ && !content.empty()) {
        logi("StudentAgent: Akıllı çözüm yolu kullanılıyor (NLP+KB).");

        // 1. Adım: Ders içeriğinden embedding oluştur
        std::vector<float> query_embedding = nlp_->generate_text_embedding_sync(content);
        
        // 2. Adım: Bilgi tabanında anlamsal arama yap
        std::vector<CerebrumLux::Capsule> related_capsules = kb_->semantic_search(query_embedding, 3);
        
        std::stringstream context_stream;
        context_stream << "İlgili bilgilerim:\n";
        if (!related_capsules.empty()) {
            for (const auto& cap : related_capsules) {
                context_stream << "- Konu: " << cap.topic << "\n  Özet: " << cap.plain_text_summary << "\n";
            }
        } else {
            context_stream << "Bu konuda özel bir bilgim yok.\n";
        }
        
        // 3. Adım: LLaMA'dan kodu tamamlamasını iste
        std::string student_prompt = 
            "Ben bir AI öğrencisiyim. Aşağıdaki dersi çözmeme yardım et.\n\n"
            "DERS AÇIKLAMASI:\n" + content + "\n\n"
            "KOD TASLAĞI:\n" + code_scaffold + "\n\n"
            "KONUYLA İLGİLİ BİLDİKLERİM:\n" + context_stream.str() + "\n"
            "Lütfen yukarıdaki bilgilere dayanarak KOD TASLAĞINI tamamla. Sadece tamamlanmış C++ kodunu döndür, başka bir açıklama ekleme.";

        std::string completed_code = nlp_->generate_simple_response(student_prompt);

        // LLaMA'dan gelen kod bloklarını temizle
        if (completed_code.rfind("```cpp", 0) == 0) {
            completed_code = completed_code.substr(6); // ```cpp\n başını kaldır
            size_t end_pos = completed_code.rfind("```");
            if (end_pos != std::string::npos) {
                completed_code = completed_code.substr(0, end_pos);
            }
        }
        
        ans["code"] = completed_code;
        explanation = "Çözümü, '" + content.substr(0, 30) + "...' dersi ve bilgi tabanından bulduğum " 
                    + std::to_string(related_capsules.size()) + " adet ilgili kapsülü kullanarak LLaMA'ya ürettirdim.";

    } else {
        // Fallback: Eski kural tabanlı çözüm
        logi("StudentAgent: Fallback (kural tabanlı) çözüm yolu kullanılıyor.");
        if (content.find("sum(const std::vector<int>&") != std::string::npos) {
            ans["code"] = "int sum(const std::vector<int>& a) {\n    int s = 0;\n    for (int x : a) {\n        s += x;\n    }\n    return s;\n}\n";
            explanation = "Çözüm için kural tabanlı olarak aralık tabanlı for döngüsü kullandım.";
        } else {
            ans["code"] = "// Öğrenci tarafından otomatik oluşturulmuş taslak\n" + code_scaffold;
            explanation = "Ancak bu konu benim için yeni, bu yüzden sadece iskeleti doldurabildim.";
        }
    }
    
    ans["explanation"] = explanation;
    ans["student_generated_at"] = now_ts();
    return ans;
}

void StudentAgent::upgrade_level() {
    level++;
    logi("Öğrenci seviyesi yükseltildi: " + std::to_string(level));
}

int StudentAgent::get_level() const {
    return level;
}

// ---------- Evaluator Gerçekleştirimi ----------
Evaluator::Evaluator() {
    logi("Evaluator başlatıldı.");
}

json Evaluator::evaluate(const json &answer, const json &lesson) {
    // Bu çok basit, geçici bir değerlendiricidir.
    logi("Değerlendirici, ders için cevabı kontrol ediyor: " + lesson.value("lesson_id", "?"));

    json report;
    report["score"] = 0;
    report["passed"] = false;
    report["details"] = json::array();

    auto tests = lesson.value("tests", json::array());
    int ok = 0;
    int total = tests.size();
    std::string code = answer.value("code", "");

    for (const auto &t : tests) {
        std::string tid = t.value("id", "t?");
        std::string type = t.value("type", "");
        bool pass = false;

        if (type == "call" && t.value("fn", "?") == "sum") {
            // `sum` fonksiyonu için ilkel bir yorumlayıcı.
            if (code.find("for") != std::string::npos && code.find("+=") != std::string::npos) {
                 const auto& args = t.at("args");
                 if (!args.empty()) {
                    const auto& arg_vec = args[0];
                    int calculated_sum = 0;
                    if(arg_vec.is_array()) {
                        for(const auto& val : arg_vec) {
                            calculated_sum += val.get<int>();
                        }
                    }
                    if (calculated_sum == t.at("expected").get<int>()) {
                        pass = true;
                    }
                 } else if (t.at("expected").get<int>() == 0) { // Boş vektör durumu
                    pass = true;
                 }
            }
        }
        report["details"].push_back({{"test_id", tid}, {"passed", pass}});
        if (pass) ok++;
    }

    int score = (total > 0) ? (int)((100.0 * ok) / total) : 0;
    report["score"] = score;
    report["passed"] = (score >= 80); // Geçme notu %80
    report["checked_at"] = now_ts();

    logi("Değerlendirme tamamlandı. Puan: " + std::to_string(score));
    return report;
}


// ---------- TutorBroker Gerçekleştirimi ----------
TutorBroker::TutorBroker(CerebrumLux::NaturalLanguageProcessor* nlp, CerebrumLux::KnowledgeBase* kb) 
    : running(false), nlp_(nlp), kb_(kb) {
    logi("TutorBroker oluşturuldu ve bağımlılıklar (NLP, KB) alındı.");
}

TutorBroker::~TutorBroker() {
    if (running) {
        stop();
    }
}

void TutorBroker::start() {
    if (running) return;
    running = true;

    // Bağımlılıkları kullanarak aktörleri oluştur
    teacher = std::make_unique<TeacherInvoker>(nlp_);
    student = std::make_unique<StudentAgent>(nlp_, kb_);
    evaluator = std::make_unique<Evaluator>();

    broker_thread = std::thread(&TutorBroker::broker_loop, this);
    
    // Döngüyü ilk ders isteğiyle başlat.
    push_teacher_request("L0001", "C++'da bir vektörün toplamını hesapla.");
}

void TutorBroker::stop() {
    running = false;
    if (broker_thread.joinable()) {
        broker_thread.join();
    }
    logi("TutorBroker durduruldu.");
}

void TutorBroker::push_teacher_request(const std::string &lesson_id, const std::string &goal) {
    Msg m;
    m.msg_id = uuid4();
    m.from = "broker";
    m.to = "teacher";
    m.type = "lesson_request";
    m.payload = {{"lesson_id", lesson_id}, {"step", 1}, {"goal", goal}};
    m.meta = {{"ts", now_ts()}};
    inq.push(m);
}

void TutorBroker::broker_loop() {
    logi("TutorBroker döngüsü başladı.");
    while (running) {
        auto mo = inq.try_pop();
        if (!mo) {
            std::this_thread::sleep_for(50ms);
            continue;
        }
        handle_msg(*mo);
    }
    logi("TutorBroker döngüsü sona erdi.");
}

void TutorBroker::handle_msg(const Msg &m) {
    logi("Mesaj işleniyor: to=" + m.to + ", type=" + m.type);

    if (m.to == "teacher") {
        json response_payload;
        if (m.type == "lesson_request") {
            response_payload = teacher->ask_teacher(m.payload);
        } else if (m.type == "eval") {
            json report = m.payload["report"];
            bool passed = report.value("passed", false);
            json lesson = m.payload["lesson"];
            if (passed) {
                logi("Ders " + lesson.value("lesson_id","?") + " BAŞARILI. Puan=" + std::to_string(report.value("score",0)));
                student->upgrade_level();
                static int lid = 2;
                std::string nextid = "L" + std::to_string(1000 + (lid++));
                push_teacher_request(nextid, "Sıradaki konu: vektörlerle ilgili kavramlar");
            } else {
                logi("Ders BAŞARISIZ. Puan=" + std::to_string(report.value("score",0)) + ". Öğretmen ipucu gönderecek.");
                Msg hint_msg;
                hint_msg.msg_id = uuid4();
                hint_msg.from = "teacher";
                hint_msg.to = "student";
                hint_msg.type = "hint";
                hint_msg.payload = {{"hint_text", "Döngünüzü ve toplama mantığınızı kontrol edin. Örneğin, aralık tabanlı for döngüsü kullanın."}};
                inq.push(hint_msg);
            }
            return;
        }
        
        Msg response_msg;
        response_msg.msg_id = uuid4();
        response_msg.from = "teacher";
        response_msg.to = "student";
        response_msg.type = "lesson";
        response_msg.payload = response_payload;
        inq.push(response_msg);

    } else if (m.to == "student") {
        if (m.type == "lesson" || m.type == "hint") {
            json answer_payload = student->produce_answer(m.payload);
            Msg answer_msg;
            answer_msg.msg_id = uuid4();
            answer_msg.from = "student";
            answer_msg.to = "evaluator";
            answer_msg.type = "answer";
            // Değerlendirme için orijinal ders bilgisi gereklidir.
            answer_msg.payload = {{"lesson", m.payload}, {"answer", answer_payload}};
            inq.push(answer_msg);
        }

    } else if (m.to == "evaluator") {
        if (m.type == "answer") {
            json lesson = m.payload["lesson"];
            json answer = m.payload["answer"];
            json report = evaluator->evaluate(answer, lesson);
            
            Msg eval_msg;
            eval_msg.msg_id = uuid4();
            eval_msg.from = "evaluator";
            eval_msg.to = "teacher";
            eval_msg.type = "eval";
            eval_msg.payload = {{"lesson", lesson}, {"answer", answer}, {"report", report}};
            inq.push(eval_msg);
        }
    } else {
        logi("İşlenemeyen mesaj: to=" + m.to + ", type=" + m.type);
    }
}