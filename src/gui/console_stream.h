#ifndef CEREBRUM_LUX_CONSOLE_STREAM_H
#define CEREBRUM_LUX_CONSOLE_STREAM_H

#include <QTextEdit>
#include <QObject>
#include <iostream>

// QTextEdit'e yazmak için ostream arabelleği
class QTextEditStreamBuf : public std::basic_streambuf<char> {
public:
    QTextEditStreamBuf(QTextEdit* editor, std::ostream& originalStream)
        : editor_(editor), originalStream_(originalStream) {}

    // YENİ: originalStream'e erişim için public getter metodu
    std::ostream& originalStream() { return originalStream_; }

protected:
    virtual int overflow(int c) override {
        if (c != EOF) {
            char ch = static_cast<char>(c);
            // QTextEdit'e ekle
            editor_->insertPlainText(QString(ch));
            // Orijinal stream'e de yaz (opsiyonel, debug için faydalı olabilir)
            originalStream_.put(ch); 
        }
        return c;
    }

    virtual std::streamsize xsputn(const char* s, std::streamsize num) override {
        // QTextEdit'e ekle
        editor_->insertPlainText(QString::fromUtf8(s, static_cast<int>(num)));
        // Orijinal stream'e de yaz
        originalStream_.write(s, num);
        return num;
    }

private:
    QTextEdit* editor_;
    std::ostream& originalStream_;
};

// std::cout ve std::cerr'i QTextEdit'e yönlendiren QObject
class ConsoleStream : public QObject {
    Q_OBJECT
public:
    ConsoleStream(QTextEdit* editor, std::ostream& stream, QObject* parent = nullptr)
        : QObject(parent), editor_(editor), original_stream_(stream), stream_buf_(editor, stream) {
        
        // Stream'in arabelleğini kendi arabelleğimize yönlendiriyoruz
        old_buf_ = stream.rdbuf();
        stream.rdbuf(&stream_buf_);
    }

    ~ConsoleStream() override {
        // Uygulama kapanırken orijinal arabelleği geri yüklüyoruz
        original_stream_.rdbuf(old_buf_);
    }

private:
    QTextEdit* editor_;
    std::ostream& original_stream_;
    QTextEditStreamBuf stream_buf_;
    std::streambuf* old_buf_;
};

#endif // CEREBRUM_LUX_CONSOLE_STREAM_H