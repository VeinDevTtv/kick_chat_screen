#include "chatoverlay.h"
#include "ui_chatoverlay.h"

#include <QPainter>
#include <QMouseEvent>
#include <QMenu>
#include <QAction>
#include <QInputDialog>
#include <QColorDialog>
#include <QMessageBox>
#include <QSettings>
#include <QVBoxLayout>
#include <QLabel>
#include <QDateTime>
#include <QScreen>
#include <QApplication>

ChatOverlay::ChatOverlay(QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::ChatOverlay)
    , m_dragging(false)
    , m_backgroundColor(0, 0, 0)
    , m_textColor(255, 255, 255)
    , m_opacity(0.7f)
    , m_maxMessages(50)
    , m_messageDuration(60)
    , m_fontSize(12)
{
    setupUi();
    setupContextMenu();
    
    // Connect to chat client signals
    connect(&m_chatClient, &KickChatClient::messageReceived, this, &ChatOverlay::onMessageReceived);
    connect(&m_chatClient, &KickChatClient::connected, this, &ChatOverlay::onConnected);
    connect(&m_chatClient, &KickChatClient::disconnected, this, &ChatOverlay::onDisconnected);
    connect(&m_chatClient, &KickChatClient::error, this, &ChatOverlay::onError);
    
    // Setup cleanup timer
    connect(&m_cleanupTimer, &QTimer::timeout, this, &ChatOverlay::onCleanupTimer);
    m_cleanupTimer.start(10000); // Check for old messages every 10 seconds
    
    // Load saved settings
    onLoadSettings();
}

ChatOverlay::~ChatOverlay()
{
    m_chatClient.disconnectFromChannel();
    delete ui;
}

void ChatOverlay::setupUi()
{
    ui->setupUi(this);
    
    // Configure window flags for overlay
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_ShowWithoutActivating);
    
    // Set default position to top right
    QScreen* screen = QApplication::primaryScreen();
    if (screen) {
        QRect screenGeometry = screen->geometry();
        int x = screenGeometry.width() - this->width() - 20;
        int y = 20;
        move(x, y);
    }
}

void ChatOverlay::setupContextMenu()
{
    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, &QWidget::customContextMenuRequested, this, [this](const QPoint& pos) {
        QMenu contextMenu(tr("Chat Overlay Menu"), this);
        
        // Connect actions
        QAction connectAction("Connect to channel...", this);
        connect(&connectAction, &QAction::triggered, this, [this]() {
            QString channelName = QInputDialog::getText(this, tr("Connect to Channel"),
                                                       tr("Enter Kick.com channel name:"));
            if (!channelName.isEmpty()) {
                connectToChannel(channelName);
            }
        });
        
        QAction disconnectAction("Disconnect", this);
        connect(&disconnectAction, &QAction::triggered, this, &ChatOverlay::disconnectFromChannel);
        disconnectAction.setEnabled(m_chatClient.isConnected());
        
        QAction bgColorAction("Set background color...", this);
        connect(&bgColorAction, &QAction::triggered, this, [this]() {
            QColor color = QColorDialog::getColor(m_backgroundColor, this);
            if (color.isValid()) {
                setBackgroundColor(color);
            }
        });
        
        QAction textColorAction("Set text color...", this);
        connect(&textColorAction, &QAction::triggered, this, [this]() {
            QColor color = QColorDialog::getColor(m_textColor, this);
            if (color.isValid()) {
                setTextColor(color);
            }
        });
        
        QAction opacityAction("Set opacity...", this);
        connect(&opacityAction, &QAction::triggered, this, [this]() {
            bool ok;
            int opacity = QInputDialog::getInt(this, tr("Set Opacity"),
                                           tr("Opacity (0-100):"), 
                                           static_cast<int>(m_opacity * 100),
                                           0, 100, 1, &ok);
            if (ok) {
                setBackgroundOpacity(opacity / 100.0f);
            }
        });
        
        QAction maxMsgAction("Set max messages...", this);
        connect(&maxMsgAction, &QAction::triggered, this, [this]() {
            bool ok;
            int count = QInputDialog::getInt(this, tr("Set Maximum Messages"),
                                         tr("Number of messages:"), 
                                         m_maxMessages, 1, 200, 1, &ok);
            if (ok) {
                setMaxMessages(count);
            }
        });
        
        QAction durationAction("Set message duration...", this);
        connect(&durationAction, &QAction::triggered, this, [this]() {
            bool ok;
            int seconds = QInputDialog::getInt(this, tr("Set Message Duration"),
                                           tr("Duration in seconds (0 for no limit):"), 
                                           m_messageDuration, 0, 3600, 1, &ok);
            if (ok) {
                setMessageDuration(seconds);
            }
        });
        
        QAction fontSizeAction("Set font size...", this);
        connect(&fontSizeAction, &QAction::triggered, this, [this]() {
            bool ok;
            int size = QInputDialog::getInt(this, tr("Set Font Size"),
                                        tr("Font size:"), 
                                        m_fontSize, 8, 32, 1, &ok);
            if (ok) {
                setFontSize(size);
            }
        });
        
        QAction saveAction("Save settings", this);
        connect(&saveAction, &QAction::triggered, this, &ChatOverlay::onSaveSettings);
        
        QAction exitAction("Exit", this);
        connect(&exitAction, &QAction::triggered, this, &QWidget::close);
        
        // Add actions to menu
        contextMenu.addAction(&connectAction);
        contextMenu.addAction(&disconnectAction);
        contextMenu.addSeparator();
        contextMenu.addAction(&bgColorAction);
        contextMenu.addAction(&textColorAction);
        contextMenu.addAction(&opacityAction);
        contextMenu.addAction(&fontSizeAction);
        contextMenu.addAction(&maxMsgAction);
        contextMenu.addAction(&durationAction);
        contextMenu.addSeparator();
        contextMenu.addAction(&saveAction);
        contextMenu.addSeparator();
        contextMenu.addAction(&exitAction);
        
        // Show the menu
        contextMenu.exec(mapToGlobal(pos));
    });
}

void ChatOverlay::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    // Draw background with opacity
    QColor bgColor = m_backgroundColor;
    bgColor.setAlphaF(m_opacity);
    painter.setBrush(bgColor);
    painter.setPen(Qt::NoPen);
    painter.drawRoundedRect(rect(), 10, 10);
    
    // Let Qt handle the widget painting
    QWidget::paintEvent(event);
}

void ChatOverlay::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragging = true;
        m_dragPosition = event->pos();
    }
    QWidget::mousePressEvent(event);
}

void ChatOverlay::mouseMoveEvent(QMouseEvent* event)
{
    if (m_dragging && (event->buttons() & Qt::LeftButton)) {
        move(event->globalPosition().toPoint() - m_dragPosition);
    }
    QWidget::mouseMoveEvent(event);
}

void ChatOverlay::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragging = false;
    }
    QWidget::mouseReleaseEvent(event);
}

void ChatOverlay::connectToChannel(const QString& channelName)
{
    ui->statusLabel->setText(tr("Connecting to %1...").arg(channelName));
    m_chatClient.connectToChannel(channelName);
}

void ChatOverlay::disconnectFromChannel()
{
    m_chatClient.disconnectFromChannel();
}

void ChatOverlay::onConnected()
{
    ui->statusLabel->setText(tr("Connected to %1").arg(m_channelName));
}

void ChatOverlay::onDisconnected()
{
    ui->statusLabel->setText(tr("Disconnected"));
}

void ChatOverlay::onError(const QString& errorMessage)
{
    ui->statusLabel->setText(tr("Error: %1").arg(errorMessage));
}

void ChatOverlay::onMessageReceived(const ChatMessage& message)
{
    // Add message to the list
    m_messages.append(message);
    
    // Enforce maximum messages limit
    while (m_messages.size() > m_maxMessages) {
        m_messages.removeFirst();
    }
    
    // Update display
    updateDisplay();
}

void ChatOverlay::updateDisplay()
{
    // Clear the layout
    QLayoutItem* item;
    while ((item = ui->scrollArea->widget()->layout()->takeAt(0)) != nullptr) {
        delete item->widget();
        delete item;
    }
    
    // Add current messages
    for (const ChatMessage& msg : m_messages) {
        QLabel* messageLabel = new QLabel(ui->scrollArea->widget());
        
        // Format text with HTML
        QString formattedMessage = QString("<span style='color: %1; font-weight: bold;'>%2:</span> <span style='color: %3;'>%4</span>")
            .arg(msg.usernameColor().name(), msg.username(), m_textColor.name(), msg.message());
        
        messageLabel->setText(formattedMessage);
        messageLabel->setTextFormat(Qt::RichText);
        messageLabel->setWordWrap(true);
        
        // Set font size
        QFont font = messageLabel->font();
        font.setPointSize(m_fontSize);
        messageLabel->setFont(font);
        
        // Add to layout
        ui->scrollArea->widget()->layout()->addWidget(messageLabel);
    }
    
    // Scroll to bottom
    QLayoutItem* lastItem = ui->scrollArea->widget()->layout()->itemAt(ui->scrollArea->widget()->layout()->count() - 1);
    if (lastItem && lastItem->widget()) {
        ui->scrollArea->ensureWidgetVisible(lastItem->widget());
    }
}

void ChatOverlay::onCleanupTimer()
{
    if (m_messageDuration <= 0) {
        return; // No expiration
    }
    
    QDateTime cutoffTime = QDateTime::currentDateTime().addSecs(-m_messageDuration);
    bool messagesRemoved = false;
    
    // Remove messages older than the duration
    while (!m_messages.isEmpty() && m_messages.first().timestamp() < cutoffTime) {
        m_messages.removeFirst();
        messagesRemoved = true;
    }
    
    // Update the display if messages were removed
    if (messagesRemoved) {
        updateDisplay();
    }
}

void ChatOverlay::setBackgroundColor(const QColor& color)
{
    m_backgroundColor = color;
    update();
}

void ChatOverlay::setTextColor(const QColor& color)
{
    m_textColor = color;
    updateDisplay();
}

void ChatOverlay::setBackgroundOpacity(float opacity)
{
    m_opacity = qBound(0.0f, opacity, 1.0f);
    update();
}

void ChatOverlay::setMaxMessages(int count)
{
    m_maxMessages = count;
    
    // Remove excess messages if necessary
    while (m_messages.size() > m_maxMessages) {
        m_messages.removeFirst();
    }
    
    updateDisplay();
}

void ChatOverlay::setMessageDuration(int seconds)
{
    m_messageDuration = seconds;
}

void ChatOverlay::setFontSize(int size)
{
    m_fontSize = size;
    updateDisplay();
}

void ChatOverlay::setPosition(const QPoint& position)
{
    move(position);
}

void ChatOverlay::onSaveSettings()
{
    QSettings settings("KickChatOverlay", "Settings");
    
    settings.setValue("backgroundColor", m_backgroundColor);
    settings.setValue("textColor", m_textColor);
    settings.setValue("opacity", m_opacity);
    settings.setValue("maxMessages", m_maxMessages);
    settings.setValue("messageDuration", m_messageDuration);
    settings.setValue("fontSize", m_fontSize);
    settings.setValue("position", pos());
    
    QMessageBox::information(this, tr("Settings Saved"), tr("Your settings have been saved."));
}

void ChatOverlay::onLoadSettings()
{
    QSettings settings("KickChatOverlay", "Settings");
    
    if (settings.contains("backgroundColor")) {
        setBackgroundColor(settings.value("backgroundColor").value<QColor>());
    }
    
    if (settings.contains("textColor")) {
        setTextColor(settings.value("textColor").value<QColor>());
    }
    
    if (settings.contains("opacity")) {
        setBackgroundOpacity(settings.value("opacity").toFloat());
    }
    
    if (settings.contains("maxMessages")) {
        setMaxMessages(settings.value("maxMessages").toInt());
    }
    
    if (settings.contains("messageDuration")) {
        setMessageDuration(settings.value("messageDuration").toInt());
    }
    
    if (settings.contains("fontSize")) {
        setFontSize(settings.value("fontSize").toInt());
    }
    
    if (settings.contains("position")) {
        setPosition(settings.value("position").toPoint());
    }
} 