#include "web_page_parser.h"
#include "../core/utils.h" // generate_unique_id için

#include <algorithm> // std::min için
#include <string_view> // std::string_view için

namespace CerebrumLux {

 WebPageParser::WebPageParser() {
    try {
        LOG_DEFAULT(LogLevel::INFO, "WebPageParser: Gumbo tabanlı parser başlatılıyor.");
    } catch (const std::exception& e) {
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "WebPageParser: Başlatma hatası: " << e.what());
        throw;
    }
    LOG_DEFAULT(LogLevel::INFO, "WebPageParser: Başlatıldı.");
}

WebPageParser::~WebPageParser() {
    LOG_DEFAULT(LogLevel::INFO, "WebPageParser: Sonlandırıldı.");
}

// Belirli bir HTML etiketinin gürültülü olup olmadığını kontrol eder
bool WebPageParser::is_noisy_html_tag(GumboTag tag) const {
    return tag == GUMBO_TAG_SCRIPT ||
           tag == GUMBO_TAG_STYLE ||
           tag == GUMBO_TAG_NAV ||
           tag == GUMBO_TAG_FOOTER ||
           tag == GUMBO_TAG_HEADER ||
           tag == GUMBO_TAG_ASIDE ||
           tag == GUMBO_TAG_IFRAME ||
           tag == GUMBO_TAG_FORM ||
           tag == GUMBO_TAG_NOSCRIPT || // Genellikle JavaScript kapalı olduğunda gösterilen içerik
           tag == GUMBO_TAG_IMG ||     // Görsel açıklamasını bazen içerik olarak istemeyebiliriz
           tag == GUMBO_TAG_AUDIO ||
           tag == GUMBO_TAG_VIDEO ||
           tag == GUMBO_TAG_CANVAS ||
           tag == GUMBO_TAG_SVG;
}

// Gumbo ile DOM ağacını dolaşarak metin çıkarır. Belirli etiketleri yoksayar.
std::string WebPageParser::traverse_and_extract_text(const GumboNode* node, bool is_in_pre_tag) const {
    if (!node) {
        return "";
    }

    std::string text;
    if (node->type == GUMBO_NODE_TEXT || node->type == GUMBO_NODE_CDATA) {
        text += node->v.text.text;
    } else if (node->type == GUMBO_NODE_ELEMENT || node->type == GUMBO_NODE_TEMPLATE) {
        // Gürültülü etiketleri ve içeriklerini tamamen atla
        if (is_noisy_html_tag(node->v.element.tag)) {
            return "";
        }

        bool pre_tag_context = is_in_pre_tag || (node->v.element.tag == GUMBO_TAG_PRE);

        // Metin düğümleri arasında boşluk bırakmak için
        if ((node->v.element.tag == GUMBO_TAG_P || node->v.element.tag == GUMBO_TAG_BR ||
             node->v.element.tag == GUMBO_TAG_DIV || node->v.element.tag == GUMBO_TAG_LI) && !pre_tag_context) {
            text += "\n";
        }

        for (unsigned int i = 0; i < node->v.element.children.length; ++i) {
            text += traverse_and_extract_text(static_cast<GumboNode*>(node->v.element.children.data[i]), pre_tag_context);
        }

        // Bazı etiketlerden sonra boşluk bırak
        if ((node->v.element.tag == GUMBO_TAG_P || node->v.element.tag == GUMBO_TAG_DIV) && !pre_tag_context) {
             text += "\n";
        }
    }
    return text;
}

// Belirli bir tag'e sahip ilk çocuğu bulur.
const GumboNode* WebPageParser::find_child_with_tag(const GumboNode* parent, GumboTag tag) const {
    if (!parent || parent->type != GUMBO_NODE_ELEMENT) {
        return nullptr;
    }
    for (unsigned int i = 0; i < parent->v.element.children.length; ++i) {
        GumboNode* child = static_cast<GumboNode*>(parent->v.element.children.data[i]);
        if (child->type == GUMBO_NODE_ELEMENT && child->v.element.tag == tag) {
            return child;
        }
    }
    return nullptr;
}

// Belirli bir node'un child node'ları arasında belirli bir tag'e sahip tüm node'ları bulur (özyinelemeli)
void WebPageParser::find_elements_by_tag(const GumboNode* parent, GumboTag tag, std::vector<const GumboNode*>& found_nodes) const {
    if (!parent || parent->type != GUMBO_NODE_ELEMENT) {
        return;
    }
    for (unsigned int i = 0; i < parent->v.element.children.length; ++i) {
        GumboNode* child = static_cast<GumboNode*>(parent->v.element.children.data[i]);
        if (child->type == GUMBO_NODE_ELEMENT) {
            if (child->v.element.tag == tag) {
                found_nodes.push_back(child);
            }
            find_elements_by_tag(child, tag, found_nodes); // Özyinelemeli arama
        }
    }
}

// Belirli bir node'un child node'ları arasında belirli bir class ismine sahip tüm node'ları bulur (özyinelemeli)
void WebPageParser::find_elements_by_class(const GumboNode* parent, const std::string& class_name, std::vector<const GumboNode*>& found_nodes) const {
    if (!parent || parent->type != GUMBO_NODE_ELEMENT) {
        return;
    }

    // Bu node'un class'ını kontrol et
    if (parent->v.element.tag != GUMBO_TAG_UNKNOWN) {
        GumboAttribute* class_attr = gumbo_get_attribute(&parent->v.element.attributes, "class");
        if (class_attr && std::string(class_attr->value).find(class_name) != std::string::npos) {
            found_nodes.push_back(parent);
        }
    }
    
    // Çocuklarda ara
    for (unsigned int i = 0; i < parent->v.element.children.length; ++i) {
        find_elements_by_class(static_cast<GumboNode*>(parent->v.element.children.data[i]), class_name, found_nodes);
    }
}

// Bir düğümün metinsel içeriğini elde eder (gumbo_node_get_text_from_node gibi çalışır ama daha kontrollü)
std::string WebPageParser::get_node_text(const GumboNode* node) const {
    if (!node) return "";
    if (node->type == GUMBO_NODE_TEXT || node->type == GUMBO_NODE_CDATA) {
        return node->v.text.text;
    } else if (node->type == GUMBO_NODE_ELEMENT) {
        return traverse_and_extract_text(node); // Kendi traverse fonksiyonumuzu kullan
    }
    return "";
}

std::vector<WebSearchResult> WebPageParser::parse_search_results(const std::string& html_content) const {
    std::vector<WebSearchResult> results;
    LOG_DEFAULT(LogLevel::TRACE, "WebPageParser: Arama sonuçları HTML'i ayrıştırılıyor. HTML boyutu: " << html_content.length());

    GumboOutput* output = gumbo_parse(html_content.c_str());
    if (!output) {
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "WebPageParser: Gumbo parse error for search results.");
        return results;
    }

    // Google arama sonuçlarının tipik yapısını hedefliyoruz:
    // Genellikle <div class="g"> veya <div class="rc"> gibi elementler içinde <a href="..."> linkleri bulunur.
    // Bu yapı sürekli değişebilir, bu nedenle burada biraz tahmin yürütüyoruz.
    std::vector<const GumboNode*> search_result_divs;
    find_elements_by_class(output->root, "g", search_result_divs); // "g" class'ına sahip divleri ara
    
    // Eski veya alternatif Google yapıları için de kontrol (örneğin "rc" class'ı)
    std::vector<const GumboNode*> alternative_search_result_divs;
    find_elements_by_class(output->root, "rc", alternative_search_result_divs);
    search_result_divs.insert(search_result_divs.end(), alternative_search_result_divs.begin(), alternative_search_result_divs.end());

    for (const GumboNode* result_div : search_result_divs) {
        std::vector<const GumboNode*> links;
        find_elements_by_tag(result_div, GUMBO_TAG_A, links);

        for (const GumboNode* link : links) {
            GumboAttribute* href_attr = gumbo_get_attribute(&link->v.element.attributes, "href");
            if (href_attr) {
                std::string url = href_attr->value;
                // Google'ın kendi dahili linklerini filtrele (örneğin /url?q=...)
                if (url.find("http://") == 0 || url.find("https://") == 0) {
                    std::string title = get_node_text(link);
                    // Snippet için linkin hemen altındaki bir metin bloğunu arayabiliriz,
                    // veya basitçe linkin text içeriğini kullanabiliriz.
                    // Daha gelişmiş snippet ayıklama için daha fazla DOM traversalı gerekebilir.
                    std::string snippet = title; // Placeholder, daha sonra iyileştirilebilir.

                    if (!title.empty() && url.find("google.com/url?q=") == std::string::npos) { // Google yönlendirme linklerini de filtrele
                        WebSearchResult result(title, url, snippet, generate_content_hash(title + url + snippet));
                        results.push_back(result);
                        LOG_DEFAULT(LogLevel::TRACE, "WebPageParser: Ayiklanan sonuc - Title: '" << result.title << "', URL: '" << result.url << "'");
                    }
                }
            }
        }
    }
    
    gumbo_destroy_output(&kGumboDefaultOptions, output);
    LOG_DEFAULT(LogLevel::DEBUG, "WebPageParser: HTML ayrıştırma tamamlandi. Toplam bulunan arama sonucu: " << results.size());
    return results;
}

// Genel bir HTML sayfasından başlık ve temizlenmiş ana metin içeriğini WebSearchResult olarak ayıklar
WebSearchResult WebPageParser::parse_general_page(const std::string& html_content, const std::string& url) const {
    LOG_DEFAULT(LogLevel::TRACE, "WebPageParser: Genel sayfa HTML'i ayrıştırılıyor. URL: " << url);

    GumboOutput* output = gumbo_parse(html_content.c_str());
    if (!output) {
        LOG_ERROR_CERR(LogLevel::ERR_CRITICAL, "WebPageParser: Gumbo parse error for general page.");
        return WebSearchResult("", url, "", "", ""); // Hata durumunda boş bir WebSearchResult döndür
    }

    std::string page_title;
    const GumboNode* title_node = find_child_with_tag(output->root, GUMBO_TAG_TITLE);
    if (title_node && title_node->v.element.children.length > 0) {
        GumboNode* title_text_node = static_cast<GumboNode*>(title_node->v.element.children.data[0]);
        if (title_text_node->type == GUMBO_NODE_TEXT) {
            page_title = title_text_node->v.text.text;
        }
    }

    // GumboOutput kullanarak ana metni elde et
    std::string main_content = clean_html_for_text_extraction(output);

    // Boşlukları ve yeni satırları düzenle (clean_html_for_text_extraction içinde yapıldığı için burada tekrara gerek kalmadı, ancak yine de son bir trim yapabiliriz)
    // Tekrar boşluk temizliği: Birden fazla boşluğu tek boşluğa indir
    std::string_view sv_main_content(main_content);
    std::string result_content;
    bool prev_char_whitespace = false;
    for(char c : sv_main_content) {
        if (std::isspace(c)) {
            if (!prev_char_whitespace) {
                result_content += ' ';
            }
            prev_char_whitespace = true;
        } else {
            result_content += c;
            prev_char_whitespace = false;
        }
    }
    main_content = result_content;
    
    
    // Baştaki ve sondaki boşlukları temizle
    main_content.erase(0, main_content.find_first_not_of(" \t\n\r"));
    main_content.erase(main_content.find_last_not_of(" \t\n\r") + 1);

    // Oluşturulan WebSearchResult'ı döndürürken snippet'ı main_content'ten alalım.
    std::string snippet = main_content.substr(0, std::min(main_content.length(), (size_t)500));

    LOG_DEFAULT(LogLevel::TRACE, "WebPageParser: Genel sayfa ayrıştırma tamamlandi. Baslik: '" << page_title << "', İçerik uzunlugu: " << main_content.length());
    gumbo_destroy_output(&kGumboDefaultOptions, output); // Gumbo çıktısını serbest bırak
    return WebSearchResult(page_title, url, snippet, generate_content_hash(page_title + url + main_content), main_content);
}

// HTML etiketlerini kaldırarak ve boşlukları düzenleyerek metin çıkarır (Gumbo tabanlı)
std::string WebPageParser::clean_html_for_text_extraction(const GumboOutput* output) const {
    if (!output || !output->root) {
        return "";
    }
    std::string cleaned_text = traverse_and_extract_text(output->root);

    // Boşlukları ve yeni satırları manuel olarak temizle
    std::string_view sv_cleaned_text(cleaned_text);
    std::string result_content;
    result_content.reserve(sv_cleaned_text.size()); // Performans optimizasyonu
    bool prev_char_is_whitespace = false;
    int consecutive_newlines = 0;

    for(char c : sv_cleaned_text) {
        if (std::isspace(c)) {
            if (c == '\n') {
                consecutive_newlines++;
            } else {
                consecutive_newlines = 0; // Başka bir boşluk türü yeni satır sayacını sıfırlar
            }

            if (!prev_char_is_whitespace) { // Sadece bir boşluk bırak
                result_content += ' ';
            }
            prev_char_is_whitespace = true;
        } else {
            if (consecutive_newlines > 0) { // Eğer önceki karakterler yeni satırsa ve şimdi bir metin geldi
                if (consecutive_newlines == 1) {
                    result_content += '\n'; // Tek yeni satırı koru
                } else {
                    result_content += "\n\n"; // İki veya daha fazla yeni satırı ikiye indir
                }
            }
            result_content += c;
            prev_char_is_whitespace = false;
            consecutive_newlines = 0;
        }
    }
    cleaned_text = result_content;

    // Baştaki ve sondaki boşlukları temizle
    size_t first_char = cleaned_text.find_first_not_of(" \t\n\r");
    if (std::string::npos == first_char) {
        return ""; // Tamamen boş veya boşluklardan ibaretse
    }
    size_t last_char = cleaned_text.find_last_not_of(" \t\n\r");
    cleaned_text = cleaned_text.substr(first_char, (last_char - first_char + 1));

    return cleaned_text;
}

std::string WebPageParser::generate_content_hash(const std::string& content) const {
    // Daha sağlam ve basit bir hash fonksiyonu (CRC32 benzeri)
    // Bu, std::hash'in bazı platformlardaki veya uzun stringlerdeki potansiyel sorunlarını aşar.
    unsigned int hash = 0x811C9DC5; // FNV offset basis
    unsigned int prime = 0x01000193; // FNV prime

    for (char c : content) {
        hash ^= static_cast<unsigned char>(c);
        hash *= prime;
    }
    return std::to_string(hash);
}

} // namespace CerebrumLux