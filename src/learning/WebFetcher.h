#ifndef WEB_FETCHER_H
#define WEB_FETCHER_H

#include <string>
#include <vector>
#include <QObject> // QObject'ten türemesi için
#include <QtNetwork/QNetworkAccessManager> // HTTP istekleri için
#include <QtNetwork/QNetworkReply> // HTTP yanıtları için
#include <QtNetwork/QNetworkCookieJar> // Çerez yönetimi için
#include <QUrl> // URL yönetimi için

// CerebrumLux Logger için
#include "../core/logger.h"
#include "web_page_parser.h" // Yeni WebPageParser sınıfı için

namespace CerebrumLux { // WebFetcher sınıfı bu namespace içine alınacak

class WebFetcher : public QObject
{
    Q_OBJECT // QObject'ten türediği için MOC tarafından işlenmeli

public:
    explicit WebFetcher(QObject *parent = nullptr);
    ~WebFetcher();

    // Bir URL'den içerik getirir (asenkron)
    void fetch_url(const std::string& url);

signals:
    // Yapılandırılmış arama sonuçları başarıyla çekildiğinde veya hata oluştuğunda sinyal gönderir
    void structured_content_fetched(const QString& url, const std::vector<CerebrumLux::WebSearchResult>& searchResults);
    void fetch_error(const QString& url, const QString& error_message);

private slots:
    void on_network_reply_finished(QNetworkReply* reply);
    // Google çerez onay formunun gönderilmesi tamamlandığında tetiklenir
    void on_google_consent_submission_finished(QNetworkReply* reply);

private:
    QNetworkAccessManager *network_manager;

    // Yeni üye değişkeni: Google çerez onayına takıldığında orijinal URL'yi saklamak için
    QString m_original_fetch_url_on_consent;

    // Yeni üye değişkeni: Çerezleri yönetmek için
    QNetworkCookieJar* m_cookie_jar;
};

} // namespace CerebrumLux

#endif // WEB_FETCHER_H