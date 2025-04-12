# Building Kick Chat Overlay

Follow these steps to build and run the Kick Chat Overlay on your system:

## Prerequisites

Before starting, you need to install:

1. **Qt 6**: Download and install from [Qt's official website](https://www.qt.io/download)
   - Make sure to select Qt 6.x with at least the following components:
     - Qt Core
     - Qt Gui
     - Qt Widgets
     - Qt Network
     - Qt WebSockets

2. **CMake**: Download and install from [CMake's official website](https://cmake.org/download/)
   - Make sure to add CMake to your system PATH during installation

3. **Visual Studio**: If on Windows, install Visual Studio with C++ development tools
   - Or another C++ compiler of your choice

## Build Steps

### Using Qt Creator (Easiest Method)

1. Open Qt Creator
2. Select "Open Project"
3. Navigate to this directory and open the `CMakeLists.txt` file
4. Configure the project when prompted
5. Click the "Build" button (hammer icon)
6. Run the application by clicking the "Run" button (play icon)

### Using Command Line

1. Open a command prompt or terminal
2. Navigate to this directory
3. Create a build directory and build the project:

```
mkdir build
cd build
cmake ..
cmake --build . --config Release
```

4. The executable will be in the build directory (Windows) or build/Release directory

## Running the Application

1. Launch the application
2. Right-click on the overlay to access the menu
3. Select "Connect to channel..." and enter your Kick.com channel name
4. Drag the overlay to your preferred position on screen

## Troubleshooting

If you encounter build errors:

1. Make sure Qt is properly installed and the PATH environment variable includes Qt's bin directory
2. Verify that CMake can find Qt by running: `cmake .. -DCMAKE_PREFIX_PATH=path/to/qt/installation`
3. On Windows, you might need to specify the generator: `cmake .. -G "Visual Studio 17 2022"`

For runtime errors:

1. Make sure all required Qt DLLs are available (usually in the same directory as the executable)
2. If DLLs are missing, copy them from your Qt installation's bin directory to the executable's directory 