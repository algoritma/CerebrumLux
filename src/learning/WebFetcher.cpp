#include "WebFetcher.h"
#include <iostream>

std::vector<WebResult> WebFetcher::search(const std::string& query) {
    std::vector<WebResult> results;

    // Burada gerçek web API entegrasyonu yapılabilir (Google API, Forum API)
    // Şimdilik placeholder:
    WebResult r1;
    r1.content = "Web içerik örneği 1: " + query;
    r1.source = "https://example.com/source1";
    results.push_back(r1);

    WebResult r2;
    r2.content = "Web içerik örneği 2: " + query;
    r2.source = "https://example.com/source2";
    results.push_back(r2);

    return results;
}
