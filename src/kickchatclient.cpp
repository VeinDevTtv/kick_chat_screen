#include "kickchatclient.h"
#include <QJsonObject>
#include <QJsonArray>
#include <QNetworkRequest>
#include <QUrl>
#include <QUrlQuery>
#include <QRandomGenerator>

KickChatClient::KickChatClient(QObject* parent)
    : QObject(parent)
{
    // Connect WebSocket signals
    connect(&m_webSocket, &QWebSocket::connected, this, &KickChatClient::onConnected);
    connect(&m_webSocket, &QWebSocket::disconnected, this, &KickChatClient::onDisconnected);
    connect(&m_webSocket, &QWebSocket::textMessageReceived, this, &KickChatClient::onTextMessageReceived);
    connect(&m_webSocket, QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::error),
            this, &KickChatClient::onError);
    
    // Setup ping timer for keeping connection alive
    connect(&m_pingTimer, &QTimer::timeout, this, &KickChatClient::onPingTimerTimeout);
    m_pingTimer.setInterval(30000); // 30 seconds
}

KickChatClient::~KickChatClient()
{
    disconnectFromChannel();
}

void KickChatClient::connectToChannel(const QString& channelName)
{
    if (channelName.isEmpty()) {
        emit error("Channel name cannot be empty");
        return;
    }
    
    m_channelName = channelName;
    fetchChannelInfo(channelName);
}

void KickChatClient::disconnectFromChannel()
{
    if (m_webSocket.isValid()) {
        m_webSocket.close();
    }
    m_pingTimer.stop();
    m_channelId.clear();
    m_channelName.clear();
}

bool KickChatClient::isConnected() const
{
    return m_webSocket.state() == QAbstractSocket::ConnectedState;
}

void KickChatClient::fetchChannelInfo(const QString& channelName)
{
    // Build URL for Kick API request
    QUrl url("https://kick.com/api/v1/channels/" + channelName);
    QNetworkRequest request(url);
    
    // Send the request
    QNetworkReply* reply = m_networkManager.get(request);
    connect(reply, &QNetworkReply::finished, [this, reply]() {
        onChannelInfoReceived(reply);
    });
}

void KickChatClient::onChannelInfoReceived(QNetworkReply* reply)
{
    reply->deleteLater();
    
    if (reply->error() != QNetworkReply::NoError) {
        emit error("Failed to get channel info: " + reply->errorString());
        return;
    }
    
    QByteArray data = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonObject channelInfo = doc.object();
    
    // Extract the channel ID
    if (channelInfo.contains("id")) {
        m_channelId = QString::number(channelInfo["id"].toInt());
        connectWebSocket();
    } else {
        emit error("Failed to get channel ID: Invalid channel data");
    }
}

void KickChatClient::connectWebSocket()
{
    // Kick.com uses PusherJS for WebSockets
    QString appKey = "eb1d5f283081a78b932c";
    QString cluster = "us2";
    
    QUrl url;
    url.setScheme("wss");
    url.setHost(QString("%1.pusher.com").arg(cluster));
    url.setPath("/app/" + appKey);
    
    QUrlQuery query;
    query.addQueryItem("protocol", "7");
    query.addQueryItem("client", "js");
    query.addQueryItem("version", "7.4.0");
    url.setQuery(query);
    
    m_webSocket.open(url);
}

void KickChatClient::onConnected()
{
    // Subscribe to the channel chat after connection
    QJsonObject subscribeMsg;
    subscribeMsg["event"] = "pusher:subscribe";
    
    QJsonObject data;
    data["channel"] = QString("chatrooms.%1.v2").arg(m_channelId);
    subscribeMsg["data"] = data;
    
    m_webSocket.sendTextMessage(QJsonDocument(subscribeMsg).toJson(QJsonDocument::Compact));
    
    // Start the ping timer to keep the connection alive
    m_pingTimer.start();
    
    emit connected();
}

void KickChatClient::onDisconnected()
{
    m_pingTimer.stop();
    emit disconnected();
}

void KickChatClient::onTextMessageReceived(const QString& message)
{
    QJsonDocument jsonDoc = QJsonDocument::fromJson(message.toUtf8());
    if (!jsonDoc.isObject()) {
        return;
    }
    
    processMessage(jsonDoc);
}

void KickChatClient::processMessage(const QJsonDocument& jsonDoc)
{
    QJsonObject messageObj = jsonDoc.object();
    QString eventName = messageObj["event"].toString();
    
    // Handle chat messages
    if (eventName == "App\\Events\\ChatMessageEvent") {
        QJsonObject dataObj = QJsonDocument::fromJson(messageObj["data"].toString().toUtf8()).object();
        QJsonObject messageData = dataObj["message"].toObject();
        
        QString username = messageData["sender"].toObject()["username"].toString();
        QString content = messageData["content"].toString();
        
        // Get color from the message if available, or generate a random one
        QColor userColor;
        if (messageData["sender"].toObject().contains("identity") && 
            messageData["sender"].toObject()["identity"].toObject().contains("color")) {
            QString colorStr = messageData["sender"].toObject()["identity"].toObject()["color"].toString();
            userColor = QColor(colorStr);
        } else {
            // Generate a random color if none is provided
            int r = QRandomGenerator::global()->bounded(128, 256);
            int g = QRandomGenerator::global()->bounded(128, 256);
            int b = QRandomGenerator::global()->bounded(128, 256);
            userColor = QColor(r, g, b);
        }
        
        // Create and emit the chat message
        ChatMessage chatMsg(username, content, userColor);
        emit messageReceived(chatMsg);
    }
}

void KickChatClient::onError(QAbstractSocket::SocketError error)
{
    emit this->error("WebSocket error: " + m_webSocket.errorString());
}

void KickChatClient::onPingTimerTimeout()
{
    // Send a ping to keep the connection alive
    if (m_webSocket.state() == QAbstractSocket::ConnectedState) {
        QJsonObject pingMsg;
        pingMsg["event"] = "pusher:ping";
        pingMsg["data"] = QJsonObject();
        
        m_webSocket.sendTextMessage(QJsonDocument(pingMsg).toJson(QJsonDocument::Compact));
    }
} 