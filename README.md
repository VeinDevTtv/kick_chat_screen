# Kick Chat Overlay

A C++ application that displays Kick.com chat messages as an overlay for streamers. Perfect for placing chat on your stream without having to capture a browser window.

## Features

- Displays chat messages from any Kick.com channel
- Customizable appearance (colors, opacity, font size)
- Adjustable message retention (number of messages and duration)
- Draggable overlay window that stays on top of other applications
- Click-through mode that lets you interact with applications beneath the overlay
- Position locking to prevent accidental movement
- Global keyboard shortcuts for toggling visibility and locking position
- Settings are saved between sessions
- Lightweight and low resource usage

## Building from Source

### Requirements

- C++ compiler supporting C++17
- CMake 3.14 or later
- Qt 6.x (Core, Gui, Widgets, Network, and WebSockets components)

### Build Instructions

1. Clone this repository
2. Create a build directory and navigate to it:
   ```
   mkdir build
   cd build
   ```
3. Configure the project with CMake:
   ```
   cmake ..
   ```
4. Build the project:
   ```
   cmake --build .
   ```

## Usage

### Basic Usage

1. Launch the application
2. Right-click on the overlay to open the menu
3. Select "Connect to channel..." and enter the Kick.com channel name
4. Adjust position and settings as needed

### Command Line Options

You can also specify a channel name directly when launching the application:

```
KickChatOverlay --channel YourChannelName
```

Or use the short form:

```
KickChatOverlay -c YourChannelName
```

### Click-Through Mode

The click-through mode allows you to interact with applications beneath the overlay:

1. Right-click the overlay and select "Click-through mode"
2. The overlay will now allow mouse clicks to pass through to underlying windows
3. Use keyboard shortcuts to interact with the overlay when in click-through mode

### Keyboard Shortcuts

Default keyboard shortcuts:
- **Ctrl+F10**: Toggle overlay visibility
- **Ctrl+F11**: Lock/unlock overlay position

You can customize these shortcuts in the settings menu by selecting "Configure hotkeys..."

### Customization

Right-click on the overlay to access the menu with the following options:

- Connect to/Disconnect from a channel
- Set background color
- Set text color
- Adjust opacity
- Change font size
- Set maximum number of messages
- Set message duration (how long messages stay visible)
- Enable/disable click-through mode
- Lock/unlock position
- Configure keyboard shortcuts
- Save settings

## License

This project is licensed under the MIT License. 