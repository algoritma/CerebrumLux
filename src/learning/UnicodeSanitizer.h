#ifndef UNICODESANITIZER_H
#define UNICODESANITIZER_H

#include <string>

class UnicodeSanitizer {
public:
    UnicodeSanitizer() = default;

    // Metindeki potansiyel olarak sorunlu veya gizli Unicode karakterlerini temizler.
    std::string sanitize(const std::string& text) const;

private:
    // Yardımcı metodlar (gelecekte eklenebilir)
    // bool isControlChar(char32_t c) const;
    // bool isConfusableChar(char32_t c) const;
};

#endif // UNICODESANITIZER_H