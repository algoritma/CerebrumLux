#ifndef INTENT_SIGNAL_H
#define INTENT_SIGNAL_H

#include <string>
#include <vector> // Sadece vector için

namespace CerebrumLux {

struct IntentSignal {
    std::string intent;
    float confidence;
    std::string source; // "fasttext", "llama", "rule"

    IntentSignal() = default;
    IntentSignal(std::string i, float c) : intent(std::move(i)), confidence(c), source("rule") {}
    IntentSignal(std::string i, float c, std::string s) : intent(std::move(i)), confidence(c), source(std::move(s)) {}
};


} // namespace CerebrumLux

#endif // INTENT_SIGNAL_H
    