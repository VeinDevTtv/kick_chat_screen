#ifndef KICKCHATCLIENT_H
#define KICKCHATCLIENT_H

#include <QObject>
#include <QWebSocket>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QTimer>
#include "chatmessage.h"

class KickChatClient : public QObject {
    Q_OBJECT

public:
    explicit KickChatClient(QObject* parent = nullptr);
    ~KickChatClient();

    void connectToChannel(const QString& channelName);
    void disconnectFromChannel();
    bool isConnected() const;

signals:
    void connected();
    void disconnected();
    void messageReceived(const ChatMessage& message);
    void error(const QString& errorMessage);

private slots:
    void onConnected();
    void onDisconnected();
    void onTextMessageReceived(const QString& message);
    void onError(QAbstractSocket::SocketError error);
    void onChannelInfoReceived(QNetworkReply* reply);
    void onPingTimerTimeout();
    void onReconnectTimer();

private:
    QWebSocket m_webSocket;
    QNetworkAccessManager m_networkManager;
    QString m_channelName;
    QString m_channelId;
    QTimer m_pingTimer;
    QTimer m_reconnectTimer;
    int m_reconnectAttempts;
    int m_maxReconnectAttempts;
    
    void fetchChannelInfo(const QString& channelName);
    void connectWebSocket();
    void processMessage(const QJsonDocument& jsonDoc);
    void startReconnectTimer();
};

#endif // KICKCHATCLIENT_H 