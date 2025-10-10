#include "WebFetcher.h"
#include <QNetworkRequest>
#include <QTextDocument> // HTML'den metin ayıklamak için (basit yöntem)
#include <QCoreApplication> // Event loop için (test amaçlı)
#include <regex> // Regex tabanlı parsing için
#include <QUrlQuery> // Form verilerini kodlamak için
#include <QVariantMap> // Form verileri için

namespace CerebrumLux { // TÜM İMPLEMENTASYON BU NAMESPACE İÇİNDE OLACAK

WebFetcher::WebFetcher(QObject *parent) : QObject(parent),
    network_manager(new QNetworkAccessManager(this)),
    m_cookie_jar(new QNetworkCookieJar(this)) // Cookie jar'ı başlat
{

    connect(network_manager, &QNetworkAccessManager::finished, this, &CerebrumLux::WebFetcher::on_network_reply_finished);
    LOG_DEFAULT(LogLevel::INFO, "WebFetcher: Initialized.");
}


WebFetcher::~WebFetcher() {
    LOG_DEFAULT(LogLevel::INFO, "WebFetcher: Destructor called.");
    // m_cookie_jar, network_manager'a atanmışsa QNetworkAccessManager tarafından yönetilir, aksi takdirde 'this' parent olduğu için otomatik silinir.
}

void WebFetcher::fetch_url(const std::string& url_str) { // Parametre adı değiştirildi
    std::string processed_url_str = url_str;

    // Eğer URL'de protokol belirtilmemişse, varsayılan olarak HTTPS ekle
    if (processed_url_str.find("http://") != 0 && processed_url_str.find("https://") != 0) {
        processed_url_str = "https://" + processed_url_str;
        LOG_DEFAULT(LogLevel::DEBUG, "WebFetcher: URL'ye varsayilan 'https://' protokolü eklendi: " << processed_url_str);
    }

    LOG_DEFAULT(LogLevel::INFO, "WebFetcher: URL cekiliyor: " << processed_url_str);
    QUrl q_url(QString::fromStdString(processed_url_str)); // İşlenmiş URL'yi kullan

    if (!q_url.isValid()) {
        QString error_msg = "Gecersiz URL: " + QString::fromStdString(processed_url_str);
        LOG_DEFAULT(LogLevel::WARNING, "WebFetcher: " << error_msg.toStdString());
        emit fetch_error(QString::fromStdString(processed_url_str), error_msg);
        return;
    }

    network_manager->setCookieJar(m_cookie_jar); // Cookie jar'ı network manager'a ayarla

    QNetworkRequest request(q_url);

    // YENİ KOD: User-Agent başlığı ekle - Google'ın bot algılamasını atlatmaya yardımcı olabilir
    request.setHeader(QNetworkRequest::UserAgentHeader, "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/91.0.4472.124 Safari/537.36 CerebrumLux/1.0");

    network_manager->get(request);
}

void WebFetcher::on_network_reply_finished(QNetworkReply* reply) {
    QString url = reply->request().url().toString();
    if (reply->error() == QNetworkReply::NoError) {
        // Çekilen HTML içeriğini ve HTTP durum kodunu her durumda logla
        std::string html_content = reply->readAll().toStdString();
        int http_status_code = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        LOG_DEFAULT(LogLevel::DEBUG, "WebFetcher: HTTP Status Code: " << http_status_code << ", URL: " << url.toStdString());
        LOG_DEFAULT(LogLevel::DEBUG, "WebFetcher: Çekilen HTML İçeriği (ilk 2000 karakter): \n" << html_content.substr(0, std::min((size_t)2000, html_content.length())));

        // Google'ın çerez onay sayfasını kontrol et
        if (html_content.find("Bevor Sie zu Google weitergehen") != std::string::npos ||
            html_content.find("Before you continue to Google") != std::string::npos ||
            html_content.find("Çerez tercihlerinizi yönetin") != std::string::npos || 
            url.contains("consent.google.com", Qt::CaseInsensitive))
        {
            LOG_DEFAULT(LogLevel::DEBUG, "WebFetcher: Google Çerez Onayı Sayfası HTML İçeriği (ilk 1000 karakter): " << html_content.substr(0, std::min((size_t)1000, html_content.length())));

            // Orijinal URL'yi sakla
            m_original_fetch_url_on_consent = url;

            // Çerez onay formunu ayrıştır (mevcut mantık)
            // Çerez onay formunu ayrıştır
            std::smatch form_match;
            std::regex form_regex(R"(<form[^>]*action=["']([^"']*)["'][^>]*method=["']post["'])", std::regex::ECMAScript | std::regex::icase);
            std::string form_action;

            if (std::regex_search(html_content, form_match, form_regex) && form_match.size() > 1) {
                form_action = form_match[1].str();
                LOG_DEFAULT(LogLevel::DEBUG, "WebFetcher: Google onay formu Action URL'si: " << form_action);
            } else {
                LOG_DEFAULT(LogLevel::WARNING, "WebFetcher: Google onay formu action URL'si bulunamadı.");
                emit fetch_error(url, "Google çerez onay formu action URL'si bulunamadı.");
                reply->deleteLater();
                return;
            }

            // Tüm gizli input alanlarını topla
            QUrlQuery post_data;
            std::regex input_regex(R"(<input type=["']hidden["'][^>]*name=["']([^"']*)["'][^>]*value=["']([^"']*)["'])", std::regex::ECMAScript | std::regex::icase);
            std::smatch input_match;
            std::string::const_iterator search_start(html_content.cbegin());
            while (std::regex_search(search_start, html_content.cend(), input_match, input_regex)) {
                if (input_match.size() > 2) {
                    post_data.addQueryItem(QString::fromStdString(input_match[1].str()), QString::fromStdString(input_match[2].str()));
                    QString name = QString::fromStdString(input_match[1].str());
                    QString value = QString::fromStdString(input_match[2].str());
                    post_data.addQueryItem(name, value);
                    LOG_DEFAULT(LogLevel::TRACE, "WebFetcher: Hidden input (consent): Name=" << name << ", Value=" << value);
                }
                search_start = input_match.suffix().first;
            }

            // Genellikle "submit_agree" gibi bir butonla kabul edilir
            // Google'ın güncel yapısına göre bu "submit_agree" yerine "cn" gibi bir isim olabilir.
            post_data.addQueryItem("cn", "true"); // Varsayılan kabul butonu
            LOG_DEFAULT(LogLevel::DEBUG, "WebFetcher: Google onay formu gönderiliyor...");

            // Onay formunu göndermek için yeni bir istek oluştur
            QUrl consent_url = reply->request().url().resolved(QUrl(QString::fromStdString(form_action)));
            QNetworkRequest consent_request(consent_url);
            consent_request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
            consent_request.setHeader(QNetworkRequest::UserAgentHeader, "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/91.0.4472.124 Safari/537.36 CerebrumLux/1.0");
            consent_request.setRawHeader("Referer", reply->request().url().toString().toUtf8()); // Referer başlığı ekle

            QNetworkReply* consent_reply = network_manager->post(consent_request, post_data.query(QUrl::FullyEncoded).toUtf8());
            // Google çerez onay formunun gönderilmesi tamamlandığında tetiklenecek lambda
            connect(consent_reply, &QNetworkReply::finished, this,
                    [this, consent_reply]() { this->on_google_consent_submission_finished(consent_reply); }, // Lambda
                    Qt::QueuedConnection); // Qt::QueuedConnection lambda'dan sonra gelmeli

            reply->deleteLater();
            return;
        }

        // ----------- YENİ KOD: JavaScript Gereksinimi Sayfası Yönlendirmesini Takip Et -----------
        // Eğer HTML içeriği "Klicke hier" veya "enablejs" gibi ifadeler içeriyorsa (JavaScript gereksinimi)
        if (html_content.find("enablejs") != std::string::npos || html_content.find("Klicke hier") != std::string::npos || html_content.find("please click here") != std::string::npos) {
            LOG_DEFAULT(LogLevel::INFO, "WebFetcher: Google 'JavaScript gereksinimi' sayfası tespit edildi. URL: " << url.toStdString());

            std::smatch refresh_match;
            std::regex refresh_regex(R"(<meta[^>]*http-equiv=["']refresh["'][^>]*content=["'][^;]*;url=([^"']*)["'])", std::regex::ECMAScript | std::regex::icase);

            if (std::regex_search(html_content, refresh_match, refresh_regex) && refresh_match.size() > 1) {
                std::string redirect_url_str = refresh_match[1].str();
                QUrl current_url(url);
                QUrl resolved_url = current_url.resolved(QUrl(QString::fromStdString(redirect_url_str)));
                LOG_DEFAULT(LogLevel::INFO, "WebFetcher: Meta refresh ile yönlendirme tespit edildi. Hedef: " << resolved_url.toString().toStdString());
                fetch_url(resolved_url.toString().toStdString());
                reply->deleteLater();
                return;
            }
        }
        // ---------------------------------------------------------------------------------------

        // WebPageParser kullanarak HTML içeriğini ayrıştır
        WebPageParser parser; // Parser objesi
        QString q_url_str = reply->request().url().toString(); // Orijinal URL'yi al

        // URL'nin Google arama sonucu olup olmadığını kontrol et
        // Basit bir kontrol: URL "google.com/search" içeriyorsa
        bool is_google_search_result_page = q_url_str.contains("google.com/search", Qt::CaseInsensitive);

        if (is_google_search_result_page) {
            std::vector<WebSearchResult> search_results = parser.parse_search_results(html_content);
            if (!search_results.empty()) {
                LOG_DEFAULT(LogLevel::INFO, "WebFetcher: URL'den yapılandırılmış arama sonuçları başarıyla çekildi ve ayrıştırıldı: " << url.toStdString() << ". Toplam sonuç: " << search_results.size());
                emit structured_content_fetched(url, search_results); // Yeni sinyali emit et
        } else if (!html_content.empty()) { // HTML içeriği boş değilse genel sayfa olarak ayrıştır
                LOG_DEFAULT(LogLevel::WARNING, "WebFetcher: Google arama sonuçları sayfasından yapılandırılmış arama sonuçları bulunamadı veya ayıklanamadı: " << url.toStdString());
                // Fallback olarak genel sayfa ayrıştırmayı deneyebiliriz, veya hata verebiliriz.
                WebSearchResult general_page_result = parser.parse_general_page(html_content, url.toStdString());
                emit structured_content_fetched(url, {general_page_result}); // Tek bir sonuç olarak gönder
            }
        } else { // Genel bir web sayfası
            WebSearchResult general_page_result = parser.parse_general_page(html_content, url.toStdString());
            LOG_DEFAULT(LogLevel::INFO, "WebFetcher: Genel web sayfasından içerik ayrıştırıldı: " << url.toStdString());
            emit structured_content_fetched(url, {general_page_result}); // Tek bir sonuç olarak gönder
        }
    } else {
        QString error_msg = "WebFetcher: HTTP Hatası: " + reply->errorString();
        LOG_DEFAULT(LogLevel::WARNING, "WebFetcher: " << error_msg.toStdString() << " URL: " << url.toStdString());
        emit fetch_error(url, error_msg);
    }
    reply->deleteLater(); // Yanıt nesnesini serbest bırak
}

// Google çerez onay formunun gönderilmesi tamamlandığında tetiklenir
void WebFetcher::on_google_consent_submission_finished(QNetworkReply* reply) {
    QString consent_url = reply->request().url().toString();
    LOG_DEFAULT(LogLevel::INFO, "WebFetcher: Google çerez onay formu gönderim yanıtı alındı. URL: " << consent_url.toStdString());

    if (reply->error() == QNetworkReply::NoError) {
        // POST sonrası bir yönlendirme var mı kontrol et
        QUrl redirectionTarget = reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();
        if (!redirectionTarget.isEmpty() && redirectionTarget != consent_url) {
            LOG_DEFAULT(LogLevel::INFO, "WebFetcher: Google çerez onayı sonrası yönlendirme tespit edildi. Hedef: " << redirectionTarget.toString().toStdString());
            fetch_url(redirectionTarget.toString().toStdString());
            m_original_fetch_url_on_consent.clear(); // Orijinal URL'yi temizle, işlem tamamlandı
        } else if (!m_original_fetch_url_on_consent.isEmpty()) {
            LOG_DEFAULT(LogLevel::INFO, "WebFetcher: Google çerez onay formu başarıyla gönderildi, ancak belirgin bir yönlendirme hedefi bulunamadı. Orijinal URL tekrar çekiliyor: " << m_original_fetch_url_on_consent.toStdString());
            fetch_url(m_original_fetch_url_on_consent.toStdString()); // Yönlendirme yoksa orijinali tekrar dene
            m_original_fetch_url_on_consent.clear(); // Orijinal URL'yi temizle, işlem tamamlandı

        } else {
            LOG_DEFAULT(LogLevel::WARNING, "WebFetcher: Google çerez onayı sonrası tekrar çekilecek orijinal URL bulunamadı.");
            emit fetch_error(consent_url, "Google çerez onayı sonrası geçerli URL bulunamadı.");
            return; // Hata durumunda metottan çık
        }
    } else {
        QString error_msg = "WebFetcher: Google çerez onay formu gönderme hatası: " + reply->errorString();
        LOG_DEFAULT(LogLevel::ERR_CRITICAL, "WebFetcher: " << error_msg.toStdString() << " URL: " << consent_url.toStdString());
        emit fetch_error(consent_url, error_msg);
    }
    reply->deleteLater();
}

} // namespace CerebrumLux