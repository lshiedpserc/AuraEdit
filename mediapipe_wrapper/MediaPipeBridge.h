#pragma once

#include <vector>
#include <string>

struct Landmark {
    float x;
    float y;
    float z;
};

class MediaPipeBridge {
public:
    MediaPipeBridge();
    ~MediaPipeBridge();

    bool initialize(const std::string& modelPath);
    std::vector<Landmark> processImage(int imageWidth, int imageHeight);

private:
    bool initialized;
};
