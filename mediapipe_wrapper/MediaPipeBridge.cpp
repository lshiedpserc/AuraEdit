#include "MediaPipeBridge.h"
#include <iostream>
#include <cmath>

MediaPipeBridge::MediaPipeBridge() : initialized(false) {}
MediaPipeBridge::~MediaPipeBridge() {}

bool MediaPipeBridge::initialize(const std::string& modelPath) {
    std::cout << "Mock MediaPipeBridge: Initializing Face Mesh model from " << modelPath << std::endl;
    initialized = true;
    return true;
}

std::vector<Landmark> MediaPipeBridge::processImage(int imageWidth, int imageHeight) {
    std::vector<Landmark> landmarks;
    if (!initialized) {
        std::cerr << "Mock MediaPipeBridge: Not initialized!" << std::endl;
        return landmarks;
    }

    float centerX = 0.5f;
    float centerY = 0.5f;
    float faceWidth = 0.3f;
    float faceHeight = 0.4f;

    for (int i = 0; i < 468; ++i) {
        float angle = (i / 468.0f) * 2.0f * 3.14159f;
        float radiusMod = 0.5f + 0.5f * (float)(rand() % 100) / 100.0f;

        Landmark lm;
        lm.x = centerX + cos(angle) * faceWidth * radiusMod;
        lm.y = centerY + sin(angle) * faceHeight * radiusMod;
        lm.z = 0.0f;
        landmarks.push_back(lm);
    }

    landmarks[152] = {centerX, centerY + faceHeight, 0.0f};
    landmarks[159] = {centerX - faceWidth*0.4f, centerY - faceHeight*0.2f, 0.0f};
    landmarks[386] = {centerX + faceWidth*0.4f, centerY - faceHeight*0.2f, 0.0f};

    return landmarks;
}
