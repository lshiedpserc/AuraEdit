#pragma once

#include <QOpenGLWidget>
#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QOpenGLFramebufferObject>
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
    QOpenGLShaderProgram *mainShader;
    QOpenGLShaderProgram *dispShader;

    QOpenGLTexture *texture;
    QOpenGLFramebufferObject *fbo;

    int skinSmoothness;
    int faceShape;
    int eyeSize;

    GLuint quadVbo, quadVao, quadEbo;

    std::vector<Landmark> faceLandmarks;
    MediaPipeBridge mpBridge;

    void createShaders();
    void renderDisplacementMap();
};
