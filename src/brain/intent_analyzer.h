#ifndef CEREBRUM_LUX_INTENT_ANALYZER_H
#define CEREBRUM_LUX_INTENT_ANALYZER_H

#include <vector>  // For std::vector
#include <map>     // For std::map
#include <string>  // For std::wstring
#include <limits>  // For std::numeric_limits
#include "../core/enums.h"         // Enumlar için
#include "../data_models/dynamic_sequence.h" // DynamicSequence için ileri bildirim
#include "intent_template.h"       // IntentTemplate için
#include "autoencoder.h"           // CryptofigAutoencoder::LATENT_DIM için (sadece boyut için)

// İleri bildirimler
struct DynamicSequence;
struct IntentTemplate;

// *** IntentAnalyzer: Kullanici niyetlerini analiz eder ***
class IntentAnalyzer {
public:
    IntentAnalyzer(); 
    UserIntent analyze_intent(const DynamicSequence& sequence); // latent_cryptofig_vector kullanacak
    
    std::vector<IntentTemplate> intent_templates; 

    void update_template_weights(UserIntent intent_id, const std::vector<float>& new_weights); 
    void update_action_success_score(UserIntent intent_id, AIAction action, float score_change); 
    std::vector<float> get_intent_weights(UserIntent intent_id) const; 

    void save_memory(const std::wstring& filename) const;
    void load_memory(const std::wstring& filename);

    void report_learning_performance(UserIntent intent_id, float implicit_feedback_avg, float explicit_feedback_avg);

    AbstractState analyze_abstract_state(const DynamicSequence& sequence, UserIntent current_intent) const; // latent_cryptofig_vector kullanacak

    float confidence_threshold_for_known_intent; 
    void set_confidence_threshold(float threshold); 
};


#endif // CEREBRUM_LUX_INTENT_ANALYZER_H