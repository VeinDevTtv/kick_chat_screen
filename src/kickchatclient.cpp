#include "kickchatclient.h"
#include <QJsonObject>
#include <QJsonArray>
#include <QNetworkRequest>
#include <QUrl>
#include <QUrlQuery>
#include <QRandomGenerator>
#include <QDebug>

KickChatClient::KickChatClient(QObject* parent)
    : QObject(parent)
    , m_reconnectAttempts(0)
    , m_maxReconnectAttempts(5)
{
    // Connect WebSocket signals
    connect(&m_webSocket, &QWebSocket::connected, this, &KickChatClient::onConnected);
    connect(&m_webSocket, &QWebSocket::disconnected, this, &KickChatClient::onDisconnected);
    connect(&m_webSocket, &QWebSocket::textMessageReceived, this, &KickChatClient::onTextMessageReceived);
    connect(&m_webSocket, &QWebSocket::errorOccurred, this, &KickChatClient::onError);
    
    // Setup ping timer for keeping connection alive
    connect(&m_pingTimer, &QTimer::timeout, this, &KickChatClient::onPingTimerTimeout);
    m_pingTimer.setInterval(30000); // 30 seconds
    
    // Setup reconnect timer
    connect(&m_reconnectTimer, &QTimer::timeout, this, &KickChatClient::onReconnectTimer);
    m_reconnectTimer.setSingleShot(true);
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
    
    // Reset reconnect attempts when connecting to a new channel
    m_reconnectAttempts = 0;
    
    m_channelName = channelName;
    
    // Try direct approach - use a direct WebSocket connection to Kick's chat service
    qDebug() << "Attempting direct WebSocket connection for channel:" << channelName;
    
    // Directly connect to the chatroom without getting channel ID first
    m_channelId = channelName; // Use the channel name directly
    connectWebSocketDirect();
}

void KickChatClient::disconnectFromChannel()
{
    // Stop reconnect attempts
    m_reconnectTimer.stop();
    
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

void KickChatClient::connectWebSocketDirect()
{
    qDebug() << "Connecting to Kick WebSocket directly";
    
    // Connect to Kick's WebSocket server with the mt1 cluster explicitly specified
    QUrl url("wss://ws-mt1.pusher.com/app/eb1d5f283081a78b932c?protocol=7&client=js&version=7.4.0&cluster=mt1");
    
    qDebug() << "WebSocket URL:" << url.toString();
    
    m_webSocket.open(url);
}

void KickChatClient::onConnected()
{
    qDebug() << "WebSocket connected";
    
    // Reset reconnect counter on successful connection
    m_reconnectAttempts = 0;
    m_reconnectTimer.stop();
    
    // Subscribe to the channel chat after connection
    QJsonObject subscribeMsg;
    subscribeMsg["event"] = "pusher:subscribe";
    
    QJsonObject data;
    data["channel"] = QString("channel-%1").arg(m_channelName);
    subscribeMsg["data"] = data;
    
    QString message = QJsonDocument(subscribeMsg).toJson(QJsonDocument::Compact);
    qDebug() << "Sending subscription message:" << message;
    
    m_webSocket.sendTextMessage(message);
    
    // Start the ping timer to keep the connection alive
    m_pingTimer.start();
    
    emit connected();
}

void KickChatClient::onDisconnected()
{
    qDebug() << "WebSocket disconnected";
    m_pingTimer.stop();
    emit disconnected();
    
    // Try to reconnect if we still have a channel name
    if (!m_channelName.isEmpty()) {
        startReconnectTimer();
    }
}

void KickChatClient::onTextMessageReceived(const QString& message)
{
    qDebug() << "Received WebSocket message:" << message.left(200) + (message.length() > 200 ? "..." : "");
    
    QJsonDocument jsonDoc = QJsonDocument::fromJson(message.toUtf8());
    if (!jsonDoc.isObject()) {
        qDebug() << "Received non-object JSON";
        return;
    }
    
    processMessage(jsonDoc);
}

void KickChatClient::processMessage(const QJsonDocument& jsonDoc)
{
    QJsonObject messageObj = jsonDoc.object();
    QString eventName = messageObj["event"].toString();
    
    qDebug() << "Received event:" << eventName;
    
    // Handle chat messages
    if (eventName == "App\\Events\\ChatMessageEvent") {
        QString dataStr = messageObj["data"].toString();
        qDebug() << "Chat message data:" << dataStr.left(200) + (dataStr.length() > 200 ? "..." : "");
        
        QJsonObject dataObj = QJsonDocument::fromJson(dataStr.toUtf8()).object();
        QJsonObject messageData = dataObj["message"].toObject();
        
        QString username = messageData["sender"].toObject()["username"].toString();
        QString content = messageData["content"].toString();
        
        qDebug() << "Chat message from" << username << ":" << content;
        
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
    else if (eventName == "pusher:connection_established") {
        qDebug() << "Pusher connection established";
        
        // Subscribe to the channel chat after connection is established
        QJsonObject subscribeMsg;
        subscribeMsg["event"] = "pusher:subscribe";
        
        QJsonObject data;
        data["channel"] = QString("channel-%1").arg(m_channelName);
        subscribeMsg["data"] = data;
        
        QString message = QJsonDocument(subscribeMsg).toJson(QJsonDocument::Compact);
        qDebug() << "Sending subscription message:" << message;
        
        m_webSocket.sendTextMessage(message);
    }
    else if (eventName == "pusher_internal:subscription_succeeded") {
        qDebug() << "Successfully subscribed to chat channel";
    }
    else if (eventName == "pusher:error") {
        QJsonObject data = messageObj["data"].toObject();
        QString errorMessage = data["message"].toString();
        qDebug() << "Pusher error:" << errorMessage;
        emit error("Pusher error: " + errorMessage);
    }
}

void KickChatClient::onError(QAbstractSocket::SocketError error)
{
    qDebug() << "WebSocket error:" << m_webSocket.errorString();
    emit this->error("WebSocket error: " + m_webSocket.errorString());
    
    // Try to reconnect after an error
    if (!m_channelName.isEmpty()) {
        startReconnectTimer();
    }
}

void KickChatClient::startReconnectTimer()
{
    // Don't try to reconnect if we've reached the maximum attempts
    if (m_reconnectAttempts >= m_maxReconnectAttempts) {
        emit error("Maximum reconnection attempts reached. Please try again later.");
        return;
    }
    
    // Use exponential backoff for reconnect (1s, 2s, 4s, 8s, etc)
    int delay = 1000 * (1 << m_reconnectAttempts);
    m_reconnectTimer.setInterval(delay);
    m_reconnectTimer.start();
    m_reconnectAttempts++;
    
    emit error(QString("Connection lost. Attempting to reconnect in %1 seconds...").arg(delay / 1000));
}

void KickChatClient::onReconnectTimer()
{
    qDebug() << "Attempting to reconnect";
    if (!m_channelName.isEmpty()) {
        connectWebSocketDirect();
    }
}

void KickChatClient::onPingTimerTimeout()
{
    // Send a ping to keep the connection alive
    if (m_webSocket.state() == QAbstractSocket::ConnectedState) {
        QJsonObject pingMsg;
        pingMsg["event"] = "pusher:ping";
        pingMsg["data"] = QJsonObject();
        
        QString message = QJsonDocument(pingMsg).toJson(QJsonDocument::Compact);
        qDebug() << "Sending ping";
        
        m_webSocket.sendTextMessage(message);
    }
} 