#include <gtest/gtest.h>



#include "../src/ai_tutor/student_ai.h"
#include "../src/ai_tutor/teacher_ai.h"
#include "../src/learning/LearningStore.h"
#include "../src/learning/LearningState.h"
#include "../src/core/CoreEventBus.h" // To ensure CerebrumLux namespace is defined
#include "../src/ai_tutor/teacher_evaluator.h" // For EvaluationResult traits, EvaluationParser
#include "../src/ai_tutor/tutor_engine.h" // To ensure CerebrumLux namespace is defined
#include "../src/ai_tutor/llama_adapter.h" // Explicitly include LlamaAdapter

// Google Test framework is assumed to be configured by the main CMakeLists.txt
// LlamaAdapter::set_inference_fn will need to be mocked or provided
// CerebrumLux::Logger::getInstance() will need to be initialized if used within AI classes

// Mock LlamaAdapter for testing purposes
// In a real scenario, you might want a more sophisticated mock or a test-specific LLaMA instance
namespace {
    std::string mock_llama_infer(const std::string& prompt) {
        // Simple mock behavior: echo back or provide a canned response
        if (prompt.find("politeness level") != std::string::npos) {
            // Simulate evaluation response for politeness
            return R"({"score_cxx": 50, "score_conversation": 60, "score_overall": 55, "feedback": "Needs improvement in politeness."})";
        }
        if (prompt.find("User says: Hello!") != std::string::npos) {
            // Simulate student response
            return "Hello there! How can I assist you today?";
        }
        return "Mock LLaMA response for: " + prompt;
    }
}

TEST(ConversationLearning, PolitenessImprovesOverTime) {
    // Setup mock LLaMA
    CerebrumLux::LlamaAdapter::set_inference_fn(mock_llama_infer);


    // Ensure learning store is clean or starts from a known state for the test
    // This is a simplification; in real tests, you'd manage test data files.
    CerebrumLux::LearningStore testStore("test_learning.log", "test_behavior.json");
    // Optionally clear previous test files
    // std::filesystem::remove("test_learning.log");
    // std::filesystem::remove("test_behavior.json");
    
    // Initialize StudentAI and TeacherAI with test-specific LearningStore or mocks
    // StudentAI now loads its profile from the LearningStore on construction
    CerebrumLux::StudentAI ai; // Will load from "test_behavior.json"
    CerebrumLux::TeacherAI teacher;

    // Simulate initial response and evaluation
    // Need to correctly interpret the evaluation output (JSON)
    std::string initial_student_response = ai.respond("Hello!");
    std::string initial_evaluation_json = teacher.evaluate(initial_student_response);
    CerebrumLux::EvaluationResult initial_eval = CerebrumLux::EvaluationParser::parseEvaluationResult(initial_evaluation_json);

    float initialPoliteness = initial_eval.score_conversation / 100.0f; // Using conversation score for politeness

    // Simulate learning iterations
    for (int i = 0; i < 10; ++i) {
        std::string student_response_loop = ai.respond("Hello!");
        std::string evaluation_json_loop = teacher.evaluate(student_response_loop);
        CerebrumLux::EvaluationResult eval_loop = CerebrumLux::EvaluationParser::parseEvaluationResult(evaluation_json_loop);
        ai.adjustBehavior(eval_loop); // Adjust behavior based on feedback
    }

    // Simulate final response and evaluation
    std::string later_student_response = ai.respond("Hello!");
    std::string later_evaluation_json = teacher.evaluate(later_student_response);
    CerebrumLux::EvaluationResult later_eval = CerebrumLux::EvaluationParser::parseEvaluationResult(later_evaluation_json);
    
    float laterPoliteness = later_eval.score_conversation / 100.0f;

    // Expect politeness to improve (simple check for now, can be more robust)
    EXPECT_GT(laterPoliteness, initialPoliteness);

    // Clean up test files if they were created
    // std::filesystem::remove("test_learning.log");
    // std::filesystem::remove("test_behavior.json");
}

TEST(LearningRegression, SkillDoesNotDegrade) {
    // Setup mock LLaMA
    CerebrumLux::LlamaAdapter::set_inference_fn(mock_llama_infer);

    // This test would typically load a "mastered" state.
    // For this example, we'll simulate a stabilized state.
    CerebrumLux::LearningStore testStore("test_learning.log", "test_behavior.json");
    
    // Simulate a stabilized LearningState
    CerebrumLux::LearningState stabilizedState = {"conversation", "politeness", 0.85f, 25, true};
    testStore.saveLearningState(stabilizedState); // Save this state to be loaded or checked

    // Simulate a stabilized BehaviorProfile
    CerebrumLux::BehaviorProfile stabilizedProfile;
    stabilizedProfile.politeness = 0.9f;
    testStore.saveBehaviorProfile(stabilizedProfile);

    // StudentAI will load the stabilized profile upon construction
    CerebrumLux::StudentAI ai;
    CerebrumLux::TeacherAI teacher;

    // Ensure the loaded profile reflects the stabilized state
    // (This check assumes StudentAI loads the behaviorProfile in its constructor from the testStore path)
    
    std::string student_response = ai.respond("Hello!");
    std::string evaluation_json = teacher.evaluate(student_response);
    CerebrumLux::EvaluationResult eval = CerebrumLux::EvaluationParser::parseEvaluationResult(evaluation_json);

    float politenessScore = eval.score_conversation / 100.0f;

    // Expect the score not to degrade significantly from the "mastered" level
    // (e.g., within 0.05 tolerance of the stabilized politeness score)
    EXPECT_GE(politenessScore, stabilizedProfile.politeness - 0.05f);

    // Clean up test files if they were created
    // std::filesystem::remove("test_learning.log");
    // std::filesystem::remove("test_behavior.json");
}

