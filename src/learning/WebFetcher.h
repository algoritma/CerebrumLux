#ifndef WEBFETCHER_H
#define WEBFETCHER_H

#include <string>
#include <vector>

struct WebResult {
    std::string content;
    std::string source;
};

class WebFetcher {
public:
    WebFetcher() = default;

    // Basit arama fonksiyonu; placeholder
    virtual std::vector<WebResult> search(const std::string& query); // DÜZELTİLDİ: virtual eklendi
};

#endif // WEBFETCHER_H