#ifndef UNICODE_SANITIZER_H
#define UNICODE_SANITIZER_H

#include <string>

namespace CerebrumLux { // Yeni eklendi

class UnicodeSanitizer {
public:
    std::string sanitize(const std::string& input) const;
};

} // namespace CerebrumLux // Yeni eklendi

#endif // UNICODE_SANITIZER_H