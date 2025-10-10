#ifndef WEB_PAGE_PARSER_H
#define WEB_PAGE_PARSER_H

#include <string>
#include <vector> // std::vector için
#include <memory> // std::unique_ptr için // Hala std::unique_ptr kullanılıyor, bu satır gerekli.

#include "../core/logger.h" // Logger için
#include "../learning/web_search_result.h" // WebSearchResult struct'ı için

#include <gumbo.h> // gumbo-parser kütüphanesi için

namespace CerebrumLux {

// Ham HTML içeriğinden yapılandırılmış bilgileri ayıklamak için sınıf
class WebPageParser {
public:
    WebPageParser();
    ~WebPageParser();

    // Verilen HTML içeriğinden arama sonuçlarını ayıklamaya çalışır
    std::vector<WebSearchResult> parse_search_results(const std::string& html_content) const;

    // Verilen genel bir HTML sayfasından başlık, ana metin vb. bilgileri ayıklar
    WebSearchResult parse_general_page(const std::string& html_content, const std::string& url) const;

private:
    // Gumbo ile DOM ağacını dolaşarak metin çıkarır. Belirli etiketleri yoksayar.
    std::string traverse_and_extract_text(const GumboNode* node, bool is_in_pre_tag = false) const;

    // Belirli bir tag'e sahip ilk çocuğu bulur.
    const GumboNode* find_child_with_tag(const GumboNode* parent, GumboTag tag) const;

    // Belirli bir node'un child node'ları arasında belirli bir tag'e sahip tüm node'ları bulur
    void find_elements_by_tag(const GumboNode* parent, GumboTag tag, std::vector<const GumboNode*>& found_nodes) const;

    // Belirli bir node'un child node'ları arasında belirli bir class ismine sahip tüm node'ları bulur (basit implementasyon)
    void find_elements_by_class(const GumboNode* parent, const std::string& class_name, std::vector<const GumboNode*>& found_nodes) const;

    // Bir düğümün metinsel içeriğini elde eder (gumbo_node_get_text_from_node gibi çalışır ama daha kontrollü)
    std::string get_node_text(const GumboNode* node) const;

    // Belirli bir HTML etiketinin gürültülü olup olmadığını kontrol eder (örn: script, style, nav, footer)
    bool is_noisy_html_tag(GumboTag tag) const;

    // HTML etiketlerini kaldırarak ve boşlukları düzenleyerek metin çıkarır (eski regex tabanlıdan DOM tabanlıya değişecek)
    std::string clean_html_for_text_extraction(const GumboOutput* output) const; // Input değişti

    std::string generate_content_hash(const std::string& content) const;
};

} // namespace CerebrumLux

#endif // WEB_PAGE_PARSER_H