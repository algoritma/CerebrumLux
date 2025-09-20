#ifndef QTEXTEDIT_STREAM_BUF_H
#define QTEXTEDIT_STREAM_BUF_H

#include <streambuf>
#include <string>
#include <iostream>
#include <QTextEdit>
#include <QMutex>
#include <QMetaObject>
#include <QString>

class QTextEditStreamBuf : public std::basic_streambuf<char> {
public:
    QTextEditStreamBuf(QTextEdit* textEdit, std::ostream& stream, QMutex* mutex = nullptr);
    ~QTextEditStreamBuf();

protected:
    int overflow(int v) override;
    std::streamsize xsputn(const char* p, std::streamsize n) override;

private:
    QTextEdit* m_textEdit;         
    std::ostream& m_originalStream; 
    std::streambuf* m_oldBuf;     
    std::string m_buffer;         
    QMutex* m_mutex;              
};

#endif // QTEXTEDIT_STREAM_BUF_H