#include "WebFetcher.h"
#include <QNetworkRequest>
#include <QTextDocument> // HTML'den metin ayıklamak için (basit yöntem)
#include <QCoreApplication> // Event loop için (test amaçlı)
#include <iostream> // std::cout için (test amaçlı)

namespace CerebrumLux { // TÜM İMPLEMENTASYON BU NAMESPACE İÇİNDE OLACAK

WebFetcher::WebFetcher(QObject *parent) : QObject(parent),
    network_manager(new QNetworkAccessManager(this))
{
    connect(network_manager, &QNetworkAccessManager::finished, this, &CerebrumLux::WebFetcher::on_network_reply_finished);
    LOG_DEFAULT(LogLevel::INFO, "WebFetcher: Initialized.");
}

WebFetcher::~WebFetcher() {
    LOG_DEFAULT(LogLevel::INFO, "WebFetcher: Destructor called.");
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

    QNetworkRequest request(q_url);
    network_manager->get(request);
}

void WebFetcher::on_network_reply_finished(QNetworkReply* reply) {
    QString url = reply->request().url().toString();
    if (reply->error() == QNetworkReply::NoError) {
        std::string html_content = reply->readAll().toStdString();
        std::string extracted_text = extract_text_from_html(html_content);
        LOG_DEFAULT(LogLevel::INFO, "WebFetcher: URL'den içerik başarıyla çekildi: " << url.toStdString());
        emit content_fetched(url, QString::fromStdString(extracted_text));
    } else {
        QString error_msg = "WebFetcher: HTTP Hatası: " + reply->errorString();
        LOG_DEFAULT(LogLevel::WARNING, "WebFetcher: " << error_msg.toStdString() << " URL: " << url.toStdString());
        emit fetch_error(url, error_msg);
    }
    reply->deleteLater(); // Yanıt nesnesini serbest bırak
}

std::string WebFetcher::extract_text_from_html(const std::string& html_content) const {
    QTextDocument text_doc;
    text_doc.setHtml(QString::fromStdString(html_content));
    return text_doc.toPlainText().toStdString();
}

} // namespace CerebrumLux