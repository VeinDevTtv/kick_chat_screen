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
#include <QTimer>
#include <QKeySequenceEdit>
#include <QDialogButtonBox>
#include <QFormLayout>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

ChatOverlay::ChatOverlay(QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::ChatOverlay)
    , m_dragging(false)
    , m_displayNeedsUpdate(false)
    , m_clickThroughEnabled(false)
    , m_positionLocked(false)
    , m_toggleVisibilityShortcut(nullptr)
    , m_lockPositionShortcut(nullptr)
    , m_toggleVisibilitySequence(Qt::CTRL | Qt::Key_F10)
    , m_lockPositionSequence(Qt::CTRL | Qt::Key_F11)
    , m_connectAction(nullptr)
    , m_disconnectAction(nullptr)
    , m_bgColorAction(nullptr)
    , m_textColorAction(nullptr)
    , m_opacityAction(nullptr)
    , m_maxMsgAction(nullptr)
    , m_durationAction(nullptr)
    , m_fontSizeAction(nullptr)
    , m_saveAction(nullptr)
    , m_exitAction(nullptr)
    , m_clickThroughAction(nullptr)
    , m_lockPositionAction(nullptr)
    , m_setHotkeyAction(nullptr)
    , m_backgroundColor(0, 0, 0)
    , m_textColor(255, 255, 255)
    , m_opacity(0.7f)
    , m_maxMessages(50)
    , m_messageDuration(60)
    , m_fontSize(12)
    , m_updateInterval(250) // Update display every 250ms maximum
    , m_maxPoolSize(100)  // Maximum size of the widget pool
{
    setupUi();
    setupContextMenu();
    setupShortcuts();
    
    // Connect to chat client signals
    connect(&m_chatClient, &KickChatClient::messageReceived, this, &ChatOverlay::onMessageReceived);
    connect(&m_chatClient, &KickChatClient::connected, this, &ChatOverlay::onConnected);
    connect(&m_chatClient, &KickChatClient::disconnected, this, &ChatOverlay::onDisconnected);
    connect(&m_chatClient, &KickChatClient::error, this, &ChatOverlay::onError);
    
    // Setup cleanup timer
    connect(&m_cleanupTimer, &QTimer::timeout, this, &ChatOverlay::onCleanupTimer);
    m_cleanupTimer.start(10000); // Check for old messages every 10 seconds
    
    // Setup display update timer
    connect(&m_updateDisplayTimer, &QTimer::timeout, this, &ChatOverlay::onUpdateDisplayTimer);
    m_updateDisplayTimer.setSingleShot(false);
    m_updateDisplayTimer.setInterval(m_updateInterval);
    m_updateDisplayTimer.start();
    
    // Load saved settings
    onLoadSettings();
}

ChatOverlay::~ChatOverlay()
{
    m_chatClient.disconnectFromChannel();
    
    // Clean up actions
    delete m_connectAction;
    delete m_disconnectAction;
    delete m_bgColorAction;
    delete m_textColorAction;
    delete m_opacityAction;
    delete m_maxMsgAction;
    delete m_durationAction;
    delete m_fontSizeAction;
    delete m_saveAction;
    delete m_exitAction;
    delete m_clickThroughAction;
    delete m_lockPositionAction;
    delete m_setHotkeyAction;
    
    // Clean up shortcuts
    delete m_toggleVisibilityShortcut;
    delete m_lockPositionShortcut;
    
    // Clean up widget pool
    while (!m_messageWidgetPool.isEmpty()) {
        delete m_messageWidgetPool.dequeue();
    }
    
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
    
    // Create actions
    m_connectAction = new QAction("Connect to channel...", this);
    m_disconnectAction = new QAction("Disconnect", this);
    m_bgColorAction = new QAction("Set background color...", this);
    m_textColorAction = new QAction("Set text color...", this);
    m_opacityAction = new QAction("Set opacity...", this);
    m_maxMsgAction = new QAction("Set max messages...", this);
    m_durationAction = new QAction("Set message duration...", this);
    m_fontSizeAction = new QAction("Set font size...", this);
    m_saveAction = new QAction("Save settings", this);
    m_exitAction = new QAction("Exit", this);
    m_clickThroughAction = new QAction("Click-through mode", this);
    m_lockPositionAction = new QAction("Lock position", this);
    m_setHotkeyAction = new QAction("Configure hotkeys...", this);
    
    m_clickThroughAction->setCheckable(true);
    m_lockPositionAction->setCheckable(true);
    
    // Connect action signals
    connect(m_connectAction, &QAction::triggered, this, [this]() {
        QString channelName = QInputDialog::getText(this, tr("Connect to Channel"),
                                                   tr("Enter Kick.com channel name:"));
        if (!channelName.isEmpty()) {
            connectToChannel(channelName);
        }
    });
    
    connect(m_disconnectAction, &QAction::triggered, this, &ChatOverlay::disconnectFromChannel);
    
    connect(m_bgColorAction, &QAction::triggered, this, [this]() {
        QColor color = QColorDialog::getColor(m_backgroundColor, this);
        if (color.isValid()) {
            setBackgroundColor(color);
        }
    });
    
    connect(m_textColorAction, &QAction::triggered, this, [this]() {
        QColor color = QColorDialog::getColor(m_textColor, this);
        if (color.isValid()) {
            setTextColor(color);
        }
    });
    
    connect(m_opacityAction, &QAction::triggered, this, [this]() {
        bool ok;
        int opacity = QInputDialog::getInt(this, tr("Set Opacity"),
                                       tr("Opacity (0-100):"), 
                                       static_cast<int>(m_opacity * 100),
                                       0, 100, 1, &ok);
        if (ok) {
            setBackgroundOpacity(opacity / 100.0f);
        }
    });
    
    connect(m_maxMsgAction, &QAction::triggered, this, [this]() {
        bool ok;
        int count = QInputDialog::getInt(this, tr("Set Maximum Messages"),
                                     tr("Number of messages:"), 
                                     m_maxMessages, 1, 200, 1, &ok);
        if (ok) {
            setMaxMessages(count);
        }
    });
    
    connect(m_durationAction, &QAction::triggered, this, [this]() {
        bool ok;
        int seconds = QInputDialog::getInt(this, tr("Set Message Duration"),
                                       tr("Duration in seconds (0 for no limit):"), 
                                       m_messageDuration, 0, 3600, 1, &ok);
        if (ok) {
            setMessageDuration(seconds);
        }
    });
    
    connect(m_fontSizeAction, &QAction::triggered, this, [this]() {
        bool ok;
        int size = QInputDialog::getInt(this, tr("Set Font Size"),
                                    tr("Font size:"), 
                                    m_fontSize, 8, 32, 1, &ok);
        if (ok) {
            setFontSize(size);
        }
    });
    
    connect(m_saveAction, &QAction::triggered, this, &ChatOverlay::onSaveSettings);
    connect(m_exitAction, &QAction::triggered, this, &QWidget::close);
    
    connect(m_clickThroughAction, &QAction::triggered, this, &ChatOverlay::toggleClickThrough);
    connect(m_lockPositionAction, &QAction::triggered, this, &ChatOverlay::toggleLockPosition);
    connect(m_setHotkeyAction, &QAction::triggered, this, &ChatOverlay::showHotkeyDialog);
    
    // Connect context menu request signal
    connect(this, &QWidget::customContextMenuRequested, this, [this](const QPoint& pos) {
        // Only show context menu if position is not locked
        if (!m_positionLocked || !m_clickThroughEnabled) {
            QMenu contextMenu(tr("Chat Overlay Menu"), this);
            
            // Update action states
            m_disconnectAction->setEnabled(m_chatClient.isConnected());
            m_clickThroughAction->setChecked(m_clickThroughEnabled);
            m_lockPositionAction->setChecked(m_positionLocked);
            
            // Add actions to menu
            contextMenu.addAction(m_connectAction);
            contextMenu.addAction(m_disconnectAction);
            contextMenu.addSeparator();
            contextMenu.addAction(m_bgColorAction);
            contextMenu.addAction(m_textColorAction);
            contextMenu.addAction(m_opacityAction);
            contextMenu.addAction(m_fontSizeAction);
            contextMenu.addAction(m_maxMsgAction);
            contextMenu.addAction(m_durationAction);
            contextMenu.addSeparator();
            contextMenu.addAction(m_clickThroughAction);
            contextMenu.addAction(m_lockPositionAction);
            contextMenu.addAction(m_setHotkeyAction);
            contextMenu.addSeparator();
            contextMenu.addAction(m_saveAction);
            contextMenu.addSeparator();
            contextMenu.addAction(m_exitAction);
            
            // Show the menu
            contextMenu.exec(mapToGlobal(pos));
        }
    });
}

void ChatOverlay::setupShortcuts()
{
    // Create global shortcuts
    m_toggleVisibilityShortcut = new QShortcut(m_toggleVisibilitySequence, this);
    m_lockPositionShortcut = new QShortcut(m_lockPositionSequence, this);
    
    // Connect shortcuts
    connect(m_toggleVisibilityShortcut, &QShortcut::activated, this, &ChatOverlay::toggleVisibility);
    connect(m_lockPositionShortcut, &QShortcut::activated, this, &ChatOverlay::toggleLockPosition);
}

void ChatOverlay::showHotkeyDialog()
{
    QDialog dialog(this);
    dialog.setWindowTitle(tr("Configure Hotkeys"));
    
    QFormLayout* layout = new QFormLayout(&dialog);
    
    QKeySequenceEdit* toggleEdit = new QKeySequenceEdit(m_toggleVisibilitySequence, &dialog);
    QKeySequenceEdit* lockEdit = new QKeySequenceEdit(m_lockPositionSequence, &dialog);
    
    layout->addRow(tr("Toggle visibility:"), toggleEdit);
    layout->addRow(tr("Lock/unlock position:"), lockEdit);
    
    QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    layout->addRow(buttons);
    
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    
    if (dialog.exec() == QDialog::Accepted) {
        // Update hotkeys
        setToggleHotkeySequence(toggleEdit->keySequence());
        setLockPositionHotkeySequence(lockEdit->keySequence());
    }
}

void ChatOverlay::setToggleHotkeySequence(const QKeySequence& sequence)
{
    if (!sequence.isEmpty()) {
        m_toggleVisibilitySequence = sequence;
        m_toggleVisibilityShortcut->setKey(sequence);
    }
}

void ChatOverlay::setLockPositionHotkeySequence(const QKeySequence& sequence)
{
    if (!sequence.isEmpty()) {
        m_lockPositionSequence = sequence;
        m_lockPositionShortcut->setKey(sequence);
    }
}

void ChatOverlay::toggleVisibility()
{
    setVisible(!isVisible());
}

void ChatOverlay::toggleLockPosition()
{
    m_positionLocked = !m_positionLocked;
    m_lockPositionAction->setChecked(m_positionLocked);
}

void ChatOverlay::toggleClickThrough()
{
    m_clickThroughEnabled = !m_clickThroughEnabled;
    m_clickThroughAction->setChecked(m_clickThroughEnabled);
    updateWindowFlags();
}

void ChatOverlay::setClickThrough(bool enabled)
{
    if (m_clickThroughEnabled != enabled) {
        m_clickThroughEnabled = enabled;
        m_clickThroughAction->setChecked(enabled);
        updateWindowFlags();
    }
}

void ChatOverlay::updateWindowFlags()
{
#ifdef Q_OS_WIN
    // Get window handle
    HWND hwnd = (HWND)this->winId();
    
    if (m_clickThroughEnabled) {
        // Make the window click-through
        SetWindowLong(hwnd, GWL_EXSTYLE, GetWindowLong(hwnd, GWL_EXSTYLE) | WS_EX_TRANSPARENT);
    } else {
        // Make the window normal (not click-through)
        SetWindowLong(hwnd, GWL_EXSTYLE, GetWindowLong(hwnd, GWL_EXSTYLE) & ~WS_EX_TRANSPARENT);
    }
#endif
}

void ChatOverlay::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton && !m_positionLocked) {
        m_dragging = true;
        m_dragPosition = event->pos();
    }
    QWidget::mousePressEvent(event);
}

void ChatOverlay::mouseMoveEvent(QMouseEvent* event)
{
    if (m_dragging && (event->buttons() & Qt::LeftButton) && !m_positionLocked) {
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
    ui->statusLabel->setText(tr("Connected"));
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
    
    // Mark display for update but don't update immediately
    // This prevents too many UI updates when messages come in rapidly
    m_displayNeedsUpdate = true;
}

void ChatOverlay::onUpdateDisplayTimer()
{
    if (m_displayNeedsUpdate) {
        updateDisplay();
        m_displayNeedsUpdate = false;
    }
}

QLabel* ChatOverlay::getMessageLabel()
{
    // Reuse an existing label if available, otherwise create a new one
    if (!m_messageWidgetPool.isEmpty()) {
        return m_messageWidgetPool.dequeue();
    }
    
    return new QLabel(ui->scrollArea->widget());
}

void ChatOverlay::recycleMessageLabel(QLabel* label)
{
    if (!label) {
        return;
    }
    
    // Reset the label state
    label->clear();
    label->setParent(nullptr);  // Remove from layout
    
    // Add to pool if not too big
    if (m_messageWidgetPool.size() < m_maxPoolSize) {
        m_messageWidgetPool.enqueue(label);
    } else {
        delete label;  // Pool is full, just delete it
    }
}

// Add this function to escape HTML characters
QString escapeHtml(const QString& input)
{
    QString escaped = input;
    escaped.replace("&", "&amp;");
    escaped.replace("<", "&lt;");
    escaped.replace(">", "&gt;");
    escaped.replace("\"", "&quot;");
    escaped.replace("'", "&#39;");
    return escaped;
}

void ChatOverlay::updateDisplay()
{
    // Store widgets to recycle
    QList<QLabel*> labelsToRecycle;
    
    // Clear the layout but keep the widgets for recycling
    QLayoutItem* item;
    while ((item = ui->scrollArea->widget()->layout()->takeAt(0)) != nullptr) {
        QLabel* label = qobject_cast<QLabel*>(item->widget());
        if (label) {
            labelsToRecycle.append(label);
        }
        delete item;
    }
    
    // Add current messages
    for (const ChatMessage& msg : m_messages) {
        QLabel* messageLabel = getMessageLabel();
        
        // Format text with HTML (escape user content to prevent XSS)
        QString formattedMessage = QString("<span style='color: %1; font-weight: bold;'>%2:</span> <span style='color: %3;'>%4</span>")
            .arg(msg.usernameColor().name(), 
                 escapeHtml(msg.username()), 
                 m_textColor.name(), 
                 escapeHtml(msg.message()));
        
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
    
    // Recycle unused labels
    for (QLabel* label : labelsToRecycle) {
        recycleMessageLabel(label);
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
        m_displayNeedsUpdate = true;
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
    updateDisplay(); // User action, update immediately
    m_displayNeedsUpdate = false;
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
    settings.setValue("clickThroughEnabled", m_clickThroughEnabled);
    settings.setValue("positionLocked", m_positionLocked);
    settings.setValue("toggleVisibilitySequence", m_toggleVisibilitySequence.toString());
    settings.setValue("lockPositionSequence", m_lockPositionSequence.toString());
    
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
    
    if (settings.contains("clickThroughEnabled")) {
        setClickThrough(settings.contains("clickThroughEnabled"));
    }
    
    if (settings.contains("positionLocked")) {
        m_positionLocked = settings.value("positionLocked").toBool();
        m_lockPositionAction->setChecked(m_positionLocked);
    }
    
    if (settings.contains("toggleVisibilitySequence")) {
        QString seqStr = settings.value("toggleVisibilitySequence").toString();
        setToggleHotkeySequence(QKeySequence(seqStr));
    }
    
    if (settings.contains("lockPositionSequence")) {
        QString seqStr = settings.value("lockPositionSequence").toString();
        setLockPositionHotkeySequence(QKeySequence(seqStr));
    }
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