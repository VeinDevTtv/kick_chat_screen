#ifndef CHATOVERLAY_H
#define CHATOVERLAY_H

#include <QWidget>
#include <QList>
#include <QTimer>
#include <QPoint>
#include <QAction>
#include <QLabel>
#include <QQueue>
#include <QKeySequence>
#include <QShortcut>
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
    void setClickThrough(bool enabled);
    void setToggleHotkeySequence(const QKeySequence& sequence);
    void setLockPositionHotkeySequence(const QKeySequence& sequence);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void contextMenuEvent(QContextMenuEvent* event) override;
    bool nativeEvent(const QByteArray& eventType, void* message, qintptr* result) override;

private slots:
    void onMessageReceived(const ChatMessage& message);
    void onConnected();
    void onDisconnected();
    void onError(const QString& errorMessage);
    void onCleanupTimer();
    void onSaveSettings();
    void onLoadSettings();
    void onUpdateDisplayTimer();
    void toggleVisibility();
    void toggleLockPosition();
    void toggleClickThrough();

private:
    Ui::ChatOverlay* ui;
    KickChatClient m_chatClient;
    QList<ChatMessage> m_messages;
    QTimer m_cleanupTimer;
    QTimer m_updateDisplayTimer;
    QPoint m_dragPosition;
    bool m_dragging;
    bool m_displayNeedsUpdate;
    bool m_clickThroughEnabled;
    bool m_positionLocked;
    
    // Keyboard shortcuts
    QShortcut* m_toggleVisibilityShortcut;
    QShortcut* m_lockPositionShortcut;
    QKeySequence m_toggleVisibilitySequence;
    QKeySequence m_lockPositionSequence;
    
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
    QAction* m_clickThroughAction;
    QAction* m_lockPositionAction;
    QAction* m_setHotkeyAction;
    
    // Settings
    QColor m_backgroundColor;
    QColor m_textColor;
    float m_opacity;
    int m_maxMessages;
    int m_messageDuration;
    int m_fontSize;
    int m_updateInterval;

    QQueue<QLabel*> m_messageWidgetPool;
    int m_maxPoolSize;

    void setupUi();
    void setupContextMenu();
    void setupShortcuts();
    void createSettingsDialog();
    void updateDisplay();
    void updateWindowFlags();

    QLabel* getMessageLabel();
    void recycleMessageLabel(QLabel* label);
    
    void showHotkeyDialog();
};

#endif // CHATOVERLAY_H 