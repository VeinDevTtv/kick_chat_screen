# Kick Chat Overlay

A C++ application that displays Kick.com chat messages as an overlay for streamers. Perfect for placing chat on your stream without having to capture a browser window.

## Features

- Displays chat messages from any Kick.com channel
- Customizable appearance (colors, opacity, font size)
- Adjustable message retention (number of messages and duration)
- Draggable overlay window that stays on top of other applications
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

### Command Line Options

You can also specify a channel name directly when launching the application:

```
KickChatOverlay --channel YourChannelName
```

Or use the short form:

```
KickChatOverlay -c YourChannelName
```

### Customization

Right-click on the overlay to access the menu with the following options:

- Connect to/Disconnect from a channel
- Set background color
- Set text color
- Adjust opacity
- Change font size
- Set maximum number of messages
- Set message duration (how long messages stay visible)
- Save settings

## License

This project is licensed under the MIT License. 