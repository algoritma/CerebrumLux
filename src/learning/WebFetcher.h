#ifndef WEB_FETCHER_H
#define WEB_FETCHER_H

#include <string>
#include <vector>
#include <QObject> // QObject'ten türemesi için
#include <QtNetwork/QNetworkAccessManager> // HTTP istekleri için
#include <QtNetwork/QNetworkReply> // HTTP yanıtları için
#include <QUrl> // URL yönetimi için

// CerebrumLux Logger için
#include "../core/logger.h"

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
    // İçerik başarıyla çekildiğinde veya hata oluştuğunda sinyal gönderir
    void content_fetched(const QString& url, const QString& content);
    void fetch_error(const QString& url, const QString& error_message);

private slots:
    void on_network_reply_finished(QNetworkReply* reply);

private:
    QNetworkAccessManager *network_manager;

    // Basit HTML'den metin ayıklama (şimdilik placeholder)
    std::string extract_text_from_html(const std::string& html_content) const;
};

} // namespace CerebrumLux

#endif // WEB_FETCHER_H