#ifndef STEGODETECTOR_H
#define STEGODETECTOR_H

#include <string>
#include <vector> // Eğer içeriği byte vektörü olarak işleyeceksek

class StegoDetector {
public:
    StegoDetector() = default;

    // Verilen metin veya veri içinde steganografi belirtileri tespit eder.
    // Bu ilk versiyonda basit heuristikler kullanacak.
    bool detectSteganography(const std::string& data) const;

private:
    // Yardımcı metodlar (gelecekte eklenebilir)
    // float analyzeFrequencyDistribution(const std::string& data) const;
    // bool checkForSuspiciousPatterns(const std::string& data) const;
};

#endif // STEGODETECTOR_H