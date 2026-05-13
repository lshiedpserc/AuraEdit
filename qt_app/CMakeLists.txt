cmake_minimum_required(VERSION 3.16)

project(AuraEdit VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 6.4 REQUIRED COMPONENTS
    Core
    Gui
    Widgets
    WebEngineWidgets
    WebChannel
    OpenGLWidgets
)

# Qt Resource file (for UI HTML/JS/CSS)
set(RESOURCES resources/resources.qrc)

set(SOURCES
    src/main.cpp
    src/MainWindow.cpp
    src/MainWindow.h
    src/GLImageWidget.cpp
    src/GLImageWidget.h
    src/AppController.cpp
    src/AppController.h
)

add_subdirectory(../mediapipe_wrapper mediapipe_wrapper)

add_executable(AuraEdit
    ${SOURCES}
    ${RESOURCES}
)

target_link_libraries(AuraEdit PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::WebEngineWidgets
    Qt6::WebChannel
    Qt6::OpenGLWidgets
    mediapipe_bridge
)
