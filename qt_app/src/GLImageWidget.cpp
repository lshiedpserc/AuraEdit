#include "GLImageWidget.h"
#include <QImage>
#include <QDebug>
#include <QVector2D>

GLImageWidget::GLImageWidget(QWidget *parent)
    : QOpenGLWidget(parent), mainShader(nullptr), dispShader(nullptr),
      texture(nullptr), fbo(nullptr),
      skinSmoothness(64), faceShape(42), eyeSize(0), quadVbo(0), quadVao(0), quadEbo(0)
{
    mpBridge.initialize("face_landmarker.task");
}

GLImageWidget::~GLImageWidget() {
    makeCurrent();
    if (texture) delete texture;
    if (fbo) delete fbo;
    if (mainShader) delete mainShader;
    if (dispShader) delete dispShader;
    if (quadVbo) glDeleteBuffers(1, &quadVbo);
    if (quadEbo) glDeleteBuffers(1, &quadEbo);
    if (quadVao) glDeleteVertexArrays(1, &quadVao);
    doneCurrent();
}

void GLImageWidget::loadImage(const QString &filePath) {
    QImage img(filePath);
    if (img.isNull()) {
        qWarning() << "Failed to load image:" << filePath;
        return;
    }

    faceLandmarks = mpBridge.processImage(img.width(), img.height());

    makeCurrent();
    if (texture) delete texture;
    texture = new QOpenGLTexture(img.mirrored());
    texture->setMinificationFilter(QOpenGLTexture::Linear);
    texture->setMagnificationFilter(QOpenGLTexture::Linear);

    if (fbo) delete fbo;
    fbo = new QOpenGLFramebufferObject(img.width(), img.height(), QOpenGLFramebufferObject::NoAttachment, GL_TEXTURE_2D, GL_RGBA16F);

    doneCurrent();
    update();
}

void GLImageWidget::setSkinSmoothness(int value) {
    skinSmoothness = value;
    update();
}

void GLImageWidget::setFaceShape(int value) {
    faceShape = value;
    update();
}

void GLImageWidget::setEyeSize(int value) {
    eyeSize = value;
    update();
}

void GLImageWidget::createShaders() {
    // 1. Displacement Shader
    dispShader = new QOpenGLShaderProgram();
    dispShader->addShaderFromSourceCode(QOpenGLShader::Vertex,
        "#version 330 core\n"
        "layout (location = 0) in vec2 aPos;\n"
        "layout (location = 1) in vec2 aTexCoord;\n"
        "out vec2 TexCoord;\n"
        "void main() {\n"
        "   gl_Position = vec4(aPos, 0.0, 1.0);\n"
        "   TexCoord = aTexCoord;\n"
        "}\n"
    );

    dispShader->addShaderFromSourceCode(QOpenGLShader::Fragment,
        "#version 330 core\n"
        "out vec4 FragColor;\n"
        "in vec2 TexCoord;\n"

        "uniform float faceShape;\n"
        "uniform float eyeSize;\n"
        "uniform vec2 leftEye;\n"
        "uniform vec2 rightEye;\n"
        "uniform vec2 chin;\n"

        "vec2 warpEye(vec2 uv, vec2 center, float radius, float strength) {\n"
        "    vec2 dir = uv - center;\n"
        "    float dist = length(dir);\n"
        "    if (dist < radius && strength > 0.0) {\n"
        "        float percent = dist / radius;\n"
        "        float s = mix(1.0, 1.0 - strength * 0.5, 1.0 - percent);\n"
        "        return dir * (s - 1.0);\n"
        "    }\n"
        "    return vec2(0.0);\n"
        "}\n"

        "vec2 warpChin(vec2 uv, vec2 center, float radius, float strength) {\n"
        "    vec2 dir = uv - center;\n"
        "    float dist = length(dir);\n"
        "    if (dist < radius && strength > 0.0) {\n"
        "        float percent = dist / radius;\n"
        "        float s = mix(1.0, 1.0 + strength * 0.2, 1.0 - percent);\n"
        "        vec2 offset = dir * (s - 1.0);\n"
        "        offset.y *= 1.5; \n"
        "        return offset;\n"
        "    }\n"
        "    return vec2(0.0);\n"
        "}\n"

        "void main() {\n"
        "   vec2 displacement = vec2(0.0);\n"
        "   \n"
        "   if (eyeSize > 0.0) {\n"
        "       displacement += warpEye(TexCoord, leftEye, 0.15, eyeSize);\n"
        "       displacement += warpEye(TexCoord, rightEye, 0.15, eyeSize);\n"
        "   }\n"
        "   if (faceShape > 0.0) {\n"
        "       displacement += warpChin(TexCoord, chin, 0.3, faceShape);\n"
        "   }\n"
        "   \n"
        "   // Map displacement from [-1, 1] to [0, 1] for 8-bit texture support, or use floating point texture directly.\n"
        "   // Since we use GL_RGBA16F for FBO, we can output raw float values, but for safety, mapping:\n"
        "   FragColor = vec4(displacement.x, displacement.y, 0.0, 1.0);\n"
        "}\n"
    );
    dispShader->link();

    // 2. Main Shader (Applies displacement & smoothing)
    mainShader = new QOpenGLShaderProgram();
    mainShader->addShaderFromSourceCode(QOpenGLShader::Vertex,
        "#version 330 core\n"
        "layout (location = 0) in vec2 aPos;\n"
        "layout (location = 1) in vec2 aTexCoord;\n"
        "out vec2 TexCoord;\n"
        "void main() {\n"
        "   gl_Position = vec4(aPos, 0.0, 1.0);\n"
        "   TexCoord = aTexCoord;\n"
        "}\n"
    );

    mainShader->addShaderFromSourceCode(QOpenGLShader::Fragment,
        "#version 330 core\n"
        "out vec4 FragColor;\n"
        "in vec2 TexCoord;\n"

        "uniform sampler2D imageTex;\n"
        "uniform sampler2D dispMap;\n"
        "uniform float skinSmoothness;\n"

        "vec4 simpleBilateral(sampler2D tex, vec2 uv, float intensity) {\n"
        "    if (intensity <= 0.0) return texture(tex, uv);\n"
        "    vec4 sum = vec4(0.0);\n"
        "    float weightSum = 0.0;\n"
        "    vec4 centerColor = texture(tex, uv);\n"
        "    float blurRadius = 0.01 * intensity;\n"
        "    for(float x = -2.0; x <= 2.0; x += 1.0) {\n"
        "        for(float y = -2.0; y <= 2.0; y += 1.0) {\n"
        "            vec2 offset = vec2(x, y) * blurRadius;\n"
        "            vec4 sampleCol = texture(tex, uv + offset);\n"
        "            float spatialDist = length(offset);\n"
        "            float colorDist = length(sampleCol.rgb - centerColor.rgb);\n"
        "            float w = exp(-(spatialDist*spatialDist)/0.01 - (colorDist*colorDist)/0.1);\n"
        "            sum += sampleCol * w;\n"
        "            weightSum += w;\n"
        "        }\n"
        "    }\n"
        "    return sum / weightSum;\n"
        "}\n"

        "void main() {\n"
        "   // Read displacement vector from map\n"
        "   vec4 d = texture(dispMap, TexCoord);\n"
        "   vec2 warpedUV = TexCoord + d.xy;\n"
        "   \n"
        "   // Sample original image with warped coordinates\n"
        "   vec4 color = simpleBilateral(imageTex, warpedUV, skinSmoothness);\n"
        "   FragColor = color;\n"
        "}\n"
    );
    mainShader->link();
}

void GLImageWidget::initializeGL() {
    initializeOpenGLFunctions();
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);

    createShaders();

    float vertices[] = {
         1.0f,  1.0f,  1.0f, 1.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
        -1.0f, -1.0f,  0.0f, 0.0f,
        -1.0f,  1.0f,  0.0f, 1.0f
    };
    unsigned int indices[] = { 0, 1, 3, 1, 2, 3 };

    glGenVertexArrays(1, &quadVao);
    glGenBuffers(1, &quadVbo);
    glGenBuffers(1, &quadEbo);

    glBindVertexArray(quadVao);
    glBindBuffer(GL_ARRAY_BUFFER, quadVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, quadEbo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

void GLImageWidget::resizeGL(int w, int h) {
    glViewport(0, 0, w, h);
}

void GLImageWidget::renderDisplacementMap() {
    if (!fbo) return;

    fbo->bind();
    glViewport(0, 0, fbo->width(), fbo->height());
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // 0 displacement
    glClear(GL_COLOR_BUFFER_BIT);

    dispShader->bind();
    dispShader->setUniformValue("faceShape", faceShape / 100.0f);
    dispShader->setUniformValue("eyeSize", eyeSize / 100.0f);

    if (faceLandmarks.size() >= 468) {
        QVector2D lEye(faceLandmarks[159].x, 1.0f - faceLandmarks[159].y);
        QVector2D rEye(faceLandmarks[386].x, 1.0f - faceLandmarks[386].y);
        QVector2D cn(faceLandmarks[152].x, 1.0f - faceLandmarks[152].y);

        dispShader->setUniformValue("leftEye", lEye);
        dispShader->setUniformValue("rightEye", rEye);
        dispShader->setUniformValue("chin", cn);
    } else {
        dispShader->setUniformValue("leftEye", QVector2D(0.4f, 0.6f));
        dispShader->setUniformValue("rightEye", QVector2D(0.6f, 0.6f));
        dispShader->setUniformValue("chin", QVector2D(0.5f, 0.2f));
    }

    glBindVertexArray(quadVao);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    dispShader->release();
    fbo->release();
}

void GLImageWidget::paintGL() {
    if (!texture || !mainShader) {
        glClear(GL_COLOR_BUFFER_BIT);
        return;
    }

    // 1. Render displacement to FBO
    renderDisplacementMap();

    // 2. Render to Screen
    glViewport(0, 0, width(), height());
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    mainShader->bind();

    glActiveTexture(GL_TEXTURE0);
    texture->bind();
    mainShader->setUniformValue("imageTex", 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, fbo->texture());
    mainShader->setUniformValue("dispMap", 1);

    mainShader->setUniformValue("skinSmoothness", skinSmoothness / 100.0f);

    glBindVertexArray(quadVao);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    mainShader->release();
}
