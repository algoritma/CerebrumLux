#ifndef WEB_SEARCH_RESULT_H
#define WEB_SEARCH_RESULT_H

#include <string>
#include <vector> // std::vector için
#include <QMetaType> // Q_DECLARE_METATYPE için

namespace CerebrumLux {

// Yapılandırılmış web arama sonuçlarını temsil eder
struct WebSearchResult {
    std::string title;
    std::string url;
    std::string snippet; // Kısa açıklama
    std::string content_hash; // İçerik hash'i, tekillik için kullanılabilir
    std::string main_content; // Web sayfasının ayıklanmış ana metin içeriği

    // Default constructor
    WebSearchResult() : title(""), url(""), snippet(""), content_hash(""), main_content("") {}
    // Complete constructor
    WebSearchResult(const std::string& title_val, const std::string& url_val,
        const std::string& snippet_val, const std::string& content_hash_val = "", const std::string& main_content_val = "")
        : title(title_val), url(url_val), snippet(snippet_val), content_hash(content_hash_val), main_content(main_content_val) {}
};

} // namespace CerebrumLux

Q_DECLARE_METATYPE(CerebrumLux::WebSearchResult)
Q_DECLARE_METATYPE(std::vector<CerebrumLux::WebSearchResult>)

#endif // WEB_SEARCH_RESULT_H