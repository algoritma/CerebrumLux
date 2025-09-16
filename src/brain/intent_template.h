#ifndef CEREBRUM_LUX_INTENT_TEMPLATE_H
#define CEREBRUM_LUX_INTENT_TEMPLATE_H

#include <vector>  // For std::vector
#include <map>     // For std::map
#include <string>  // For std::string
#include "../core/enums.h" // UserIntent, AIAction enum'ları için
#include "../core/utils.h" // For convert_wstring_to_string (if needed elsewhere)

// İleri bildirim: Eğer IntentTemplate içinde CryptofigAutoencoder'dan bir boyut kullanılıyorsa, burada bildirilebilir.
// Ancak IntentTemplate'ın kendisi CryptofigAutoencoder'ı doğrudan kullanmadığı için şimdilik gerek yok.

// *** IntentTemplate: Dinamik niyet sablonlarini temsil eden yapi ***
struct IntentTemplate { 
    UserIntent id;
    std::vector<float> weights; // Bu ağırlıklar artık latent_cryptofig_vector boyutunda olacak
    std::map<AIAction, float> action_success_scores; 
    
    IntentTemplate(UserIntent intent_id, const std::vector<float>& initial_weights); 
};

#endif // CEREBRUM_LUX_INTENT_TEMPLATE_H