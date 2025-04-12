cmake_minimum_required(VERSION 3.14)
project(KickChatOverlay VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 COMPONENTS Core Gui Widgets Network WebSockets REQUIRED)

# Source files
set(SOURCES
    src/main.cpp
    src/chatoverlay.cpp
    src/chatmessage.cpp
    src/kickchatclient.cpp
)

# Header files
set(HEADERS
    src/chatoverlay.h
    src/chatmessage.h
    src/kickchatclient.h
)

# UI files
set(UI_FILES
    src/chatoverlay.ui
)

# Create executable
add_executable(KickChatOverlay ${SOURCES} ${HEADERS} ${UI_FILES})

# Link libraries
target_link_libraries(KickChatOverlay PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Network
    Qt6::WebSockets
)

# Include directories
target_include_directories(KickChatOverlay PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
) 