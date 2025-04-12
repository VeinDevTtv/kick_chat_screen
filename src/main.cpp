#include "chatoverlay.h"
#include <QApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>

int main(int argc, char *argv[])
{
    // Create application
    QApplication app(argc, argv);
    app.setApplicationName("KickChatOverlay");
    app.setApplicationVersion("1.0.0");
    
    // Parse command line arguments
    QCommandLineParser parser;
    parser.setApplicationDescription("Kick.com Chat Overlay for Streamers");
    parser.addHelpOption();
    parser.addVersionOption();
    
    // Add option for auto-connecting to a channel
    QCommandLineOption channelOption(QStringList() << "c" << "channel",
                                    "Auto-connect to channel <name>",
                                    "name");
    parser.addOption(channelOption);
    
    parser.process(app);
    
    // Create and show chat overlay
    ChatOverlay overlay;
    overlay.show();
    
    // Auto-connect to channel if specified
    if (parser.isSet(channelOption)) {
        QString channelName = parser.value(channelOption);
        overlay.connectToChannel(channelName);
    }
    
    return app.exec();
} 