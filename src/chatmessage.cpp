#include "chatmessage.h"

ChatMessage::ChatMessage(const QString& username, const QString& message, 
                         const QColor& usernameColor, const QDateTime& timestamp)
    : m_username(username)
    , m_message(message)
    , m_usernameColor(usernameColor)
    , m_timestamp(timestamp)
{
}

QString ChatMessage::username() const
{
    return m_username;
}

QString ChatMessage::message() const
{
    return m_message;
}

QColor ChatMessage::usernameColor() const
{
    return m_usernameColor;
}

QDateTime ChatMessage::timestamp() const
{
    return m_timestamp;
} 