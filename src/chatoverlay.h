#ifndef CHATOVERLAY_H
#define CHATOVERLAY_H

#include <QWidget>
#include <QList>
#include <QTimer>
#include <QPoint>
#include <QAction>
#include "kickchatclient.h"
#include "chatmessage.h"

namespace Ui {
class ChatOverlay;
}

class ChatOverlay : public QWidget {
    Q_OBJECT

public:
    explicit ChatOverlay(QWidget* parent = nullptr);
    ~ChatOverlay();

    void connectToChannel(const QString& channelName);
    void disconnectFromChannel();
    
    // Configuration methods
    void setBackgroundColor(const QColor& color);
    void setTextColor(const QColor& color);
    void setBackgroundOpacity(float opacity);
    void setMaxMessages(int count);
    void setMessageDuration(int seconds);
    void setFontSize(int size);
    void setPosition(const QPoint& position);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void contextMenuEvent(QContextMenuEvent* event) override;

private slots:
    void onMessageReceived(const ChatMessage& message);
    void onConnected();
    void onDisconnected();
    void onError(const QString& errorMessage);
    void onCleanupTimer();
    void onSaveSettings();
    void onLoadSettings();
    void onUpdateDisplayTimer();

private:
    Ui::ChatOverlay* ui;
    KickChatClient m_chatClient;
    QList<ChatMessage> m_messages;
    QTimer m_cleanupTimer;
    QTimer m_updateDisplayTimer;
    QPoint m_dragPosition;
    bool m_dragging;
    bool m_displayNeedsUpdate;
    
    // Context menu actions
    QAction* m_connectAction;
    QAction* m_disconnectAction;
    QAction* m_bgColorAction;
    QAction* m_textColorAction;
    QAction* m_opacityAction;
    QAction* m_maxMsgAction;
    QAction* m_durationAction;
    QAction* m_fontSizeAction;
    QAction* m_saveAction;
    QAction* m_exitAction;
    
    // Settings
    QColor m_backgroundColor;
    QColor m_textColor;
    float m_opacity;
    int m_maxMessages;
    int m_messageDuration;
    int m_fontSize;
    int m_updateInterval;

    void setupUi();
    void setupContextMenu();
    void createSettingsDialog();
    void updateDisplay();
};

#endif // CHATOVERLAY_H 