#pragma once

#include <QOpenGLWidget>
#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include "../../mediapipe_wrapper/MediaPipeBridge.h"

class GLImageWidget : public QOpenGLWidget, protected QOpenGLFunctions_3_3_Core {
    Q_OBJECT

public:
    explicit GLImageWidget(QWidget *parent = nullptr);
    ~GLImageWidget();

    void loadImage(const QString &filePath);

public slots:
    void setSkinSmoothness(int value);
    void setFaceShape(int value);
    void setEyeSize(int value);

protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;

private:
    QOpenGLShaderProgram *shaderProgram;
    QOpenGLTexture *texture;
    int skinSmoothness;
    int faceShape;
    int eyeSize;

    // Geometry for a fullscreen quad
    GLuint vbo, vao, ebo;

    // Landmarks data for shaders
    std::vector<Landmark> faceLandmarks;

    // Shader Uniform Locations
    int texLoc;
    int skinSmoothnessLoc;
    int faceShapeLoc;
    int eyeSizeLoc;

    // Feature points
    int leftEyeLoc;
    int rightEyeLoc;
    int chinLoc;

    // Utility to run MediaPipe
    MediaPipeBridge mpBridge;

    void createShaders();
};
