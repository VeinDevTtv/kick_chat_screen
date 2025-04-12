#ifndef CHATMESSAGE_H
#define CHATMESSAGE_H

#include <QString>
#include <QColor>
#include <QDateTime>

class ChatMessage {
public:
    ChatMessage(const QString& username, const QString& message, 
                const QColor& usernameColor = QColor(255, 255, 255), 
                const QDateTime& timestamp = QDateTime::currentDateTime());

    QString username() const;
    QString message() const;
    QColor usernameColor() const;
    QDateTime timestamp() const;

private:
    QString m_username;
    QString m_message;
    QColor m_usernameColor;
    QDateTime m_timestamp;
};

#endif // CHATMESSAGE_H 