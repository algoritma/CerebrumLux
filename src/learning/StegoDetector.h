#ifndef STEGO_DETECTOR_H
#define STEGO_DETECTOR_H

#include <string>
#include <vector>

namespace CerebrumLux { // Yeni eklendi

class StegoDetector {
public:
    bool detectSteganography(const std::string& data) const;

private:
    // Örneğin, belirli imza kalıpları veya anormallikler
    bool checkEntropy(const std::string& data) const;
    bool checkMetadata(const std::string& data) const; // Örneğin, dosya formatları için
    bool checkKnownSignatures(const std::string& data) const;
};

} // namespace CerebrumLux // Yeni eklendi

#endif // STEGO_DETECTOR_H