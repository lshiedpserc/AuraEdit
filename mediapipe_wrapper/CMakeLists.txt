cmake_minimum_required(VERSION 3.16)
project(MediaPipeBridge VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(mediapipe_bridge STATIC
    MediaPipeBridge.cpp
    MediaPipeBridge.h
)

target_include_directories(mediapipe_bridge PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
