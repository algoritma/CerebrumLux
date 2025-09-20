#include "qtextedit_stream_buf.h"

QTextEditStreamBuf::QTextEditStreamBuf(QTextEdit* textEdit, std::ostream& stream, QMutex* mutex)
    : m_textEdit(textEdit), m_originalStream(stream), m_mutex(mutex) {
    m_oldBuf = stream.rdbuf(this);
}

QTextEditStreamBuf::~QTextEditStreamBuf() {
    m_originalStream.rdbuf(m_oldBuf);
}

int QTextEditStreamBuf::overflow(int v) {
    if (v == EOF) {
        return EOF;
    }
    m_buffer += static_cast<char>(v);
    if (v == '\n') {
        if (m_mutex) m_mutex->lock();
        QMetaObject::invokeMethod(m_textEdit, "append", Qt::QueuedConnection,
                                  Q_ARG(QString, QString::fromStdString(m_buffer)));
        m_buffer.clear();
        if (m_mutex) m_mutex->unlock();
    }
    return v;
}

std::streamsize QTextEditStreamBuf::xsputn(const char* p, std::streamsize n) {
    std::string text(p, n);
    m_buffer += text;

    size_t lastNewline = m_buffer.find_last_of('\n');
    if (lastNewline != std::string::npos) {
        std::string lineToFlush = m_buffer.substr(0, lastNewline + 1);
        m_buffer = m_buffer.substr(lastNewline + 1);

        if (m_mutex) m_mutex->lock();
        QMetaObject::invokeMethod(m_textEdit, "append", Qt::QueuedConnection,
                                  Q_ARG(QString, QString::fromStdString(lineToFlush)));
        if (m_mutex) m_mutex->unlock();
    }
    return n;
}