#include "GLImageWidget.h"
#include <QImage>
#include <QDebug>
#include <QVector2D>

GLImageWidget::GLImageWidget(QWidget *parent)
    : QOpenGLWidget(parent), shaderProgram(nullptr), texture(nullptr),
      skinSmoothness(64), faceShape(42), eyeSize(0), vbo(0), vao(0), ebo(0)
{
    mpBridge.initialize("face_landmarker.task");
}

GLImageWidget::~GLImageWidget() {
    makeCurrent();
    if (texture) delete texture;
    if (shaderProgram) delete shaderProgram;
    if (vbo) glDeleteBuffers(1, &vbo);
    if (ebo) glDeleteBuffers(1, &ebo);
    if (vao) glDeleteVertexArrays(1, &vao);
    doneCurrent();
}

void GLImageWidget::loadImage(const QString &filePath) {
    QImage img(filePath);
    if (img.isNull()) {
        qWarning() << "Failed to load image:" << filePath;
        return;
    }

    // Process image through MediaPipe to get landmarks
    faceLandmarks = mpBridge.processImage(img.width(), img.height());

    makeCurrent();
    if (texture) {
        delete texture;
    }
    // OpenGL expects origin at bottom-left, but we can handle UV in shader or mirror
    texture = new QOpenGLTexture(img.mirrored());
    texture->setMinificationFilter(QOpenGLTexture::LinearMipMapLinear);
    texture->setMagnificationFilter(QOpenGLTexture::Linear);
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
    shaderProgram = new QOpenGLShaderProgram();

    // Vertex Shader: Simple passthrough
    shaderProgram->addShaderFromSourceCode(QOpenGLShader::Vertex,
        "#version 330 core\n"
        "layout (location = 0) in vec2 aPos;\n"
        "layout (location = 1) in vec2 aTexCoord;\n"
        "out vec2 TexCoord;\n"
        "void main() {\n"
        "   gl_Position = vec4(aPos.x, aPos.y, 0.0, 1.0);\n"
        "   TexCoord = aTexCoord;\n"
        "}\n"
    );

    // Fragment Shader: Warping + Bilateral Smoothing
    shaderProgram->addShaderFromSourceCode(QOpenGLShader::Fragment,
        "#version 330 core\n"
        "out vec4 FragColor;\n"
        "in vec2 TexCoord;\n"

        "uniform sampler2D ourTexture;\n"
        "uniform float skinSmoothness;\n"
        "uniform float faceShape;\n"
        "uniform float eyeSize;\n"

        "uniform vec2 leftEye;\n"
        "uniform vec2 rightEye;\n"
        "uniform vec2 chin;\n"

        "vec2 warpEye(vec2 uv, vec2 center, float radius, float strength) {\n"
        "    vec2 dir = uv - center;\n"
        "    float dist = length(dir);\n"
        "    if (dist < radius && strength > 0.0) {\n"
        "        // Simple bulge distortion\n"
        "        float percent = dist / radius;\n"
        "        float curve = percent * percent * percent;\n"
        "        float s = mix(1.0, 1.0 - strength * 0.5, 1.0 - percent);\n"
        "        return center + dir * s;\n"
        "    }\n"
        "    return uv;\n"
        "}\n"

        "vec2 warpChin(vec2 uv, vec2 center, float radius, float strength) {\n"
        "    vec2 dir = uv - center;\n"
        "    float dist = length(dir);\n"
        "    if (dist < radius && strength > 0.0) {\n"
        "        // Pinch distortion (pulling up/in)\n"
        "        float percent = dist / radius;\n"
        "        float s = mix(1.0, 1.0 + strength * 0.2, 1.0 - percent);\n"
        "        // mostly pull Y up\n"
        "        vec2 offset = dir * (s - 1.0);\n"
        "        offset.y *= 1.5; \n"
        "        return uv + offset;\n"
        "    }\n"
        "    return uv;\n"
        "}\n"

        "vec4 simpleBilateral(sampler2D tex, vec2 uv, float intensity) {\n"
        "    // A very simplified blur for skin smoothing demonstration\n"
        "    if (intensity <= 0.0) return texture(tex, uv);\n"
        "    \n"
        "    vec4 sum = vec4(0.0);\n"
        "    float weightSum = 0.0;\n"
        "    vec4 centerColor = texture(tex, uv);\n"
        "    float blurRadius = 0.01 * intensity;\n"
        "    \n"
        "    for(float x = -2.0; x <= 2.0; x += 1.0) {\n"
        "        for(float y = -2.0; y <= 2.0; y += 1.0) {\n"
        "            vec2 offset = vec2(x, y) * blurRadius;\n"
        "            vec4 sampleCol = texture(tex, uv + offset);\n"
        "            \n"
        "            float spatialDist = length(offset);\n"
        "            float colorDist = length(sampleCol.rgb - centerColor.rgb);\n"
        "            \n"
        "            float w = exp(-(spatialDist*spatialDist)/0.01 - (colorDist*colorDist)/0.1);\n"
        "            sum += sampleCol * w;\n"
        "            weightSum += w;\n"
        "        }\n"
        "    }\n"
        "    return sum / weightSum;\n"
        "}\n"

        "void main() {\n"
        "   vec2 uv = TexCoord;\n"
        "   \n"
        "   // 1. Warp eyes\n"
        "   if (eyeSize > 0.0) {\n"
        "       uv = warpEye(uv, leftEye, 0.15, eyeSize);\n"
        "       uv = warpEye(uv, rightEye, 0.15, eyeSize);\n"
        "   }\n"
        "   \n"
        "   // 2. Warp chin/face shape\n"
        "   if (faceShape > 0.0) {\n"
        "       uv = warpChin(uv, chin, 0.3, faceShape);\n"
        "   }\n"
        "   \n"
        "   // 3. Sample and apply skin smoothing\n"
        "   vec4 color = simpleBilateral(ourTexture, uv, skinSmoothness);\n"
        "   \n"
        "   FragColor = color;\n"
        "}\n"
    );
    shaderProgram->link();

    texLoc = shaderProgram->uniformLocation("ourTexture");
    skinSmoothnessLoc = shaderProgram->uniformLocation("skinSmoothness");
    faceShapeLoc = shaderProgram->uniformLocation("faceShape");
    eyeSizeLoc = shaderProgram->uniformLocation("eyeSize");

    leftEyeLoc = shaderProgram->uniformLocation("leftEye");
    rightEyeLoc = shaderProgram->uniformLocation("rightEye");
    chinLoc = shaderProgram->uniformLocation("chin");
}

void GLImageWidget::initializeGL() {
    initializeOpenGLFunctions();
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);

    createShaders();

    // Setup fullscreen quad
    float vertices[] = {
        // positions   // texture coords
         1.0f,  1.0f,  1.0f, 1.0f, // top right
         1.0f, -1.0f,  1.0f, 0.0f, // bottom right
        -1.0f, -1.0f,  0.0f, 0.0f, // bottom left
        -1.0f,  1.0f,  0.0f, 1.0f  // top left
    };
    unsigned int indices[] = {
        0, 1, 3, // first triangle
        1, 2, 3  // second triangle
    };

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);

    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // Position attribute
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // Texture coord attribute
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

void GLImageWidget::resizeGL(int w, int h) {
    glViewport(0, 0, w, h);
}

void GLImageWidget::paintGL() {
    glClear(GL_COLOR_BUFFER_BIT);

    if (texture && shaderProgram) {
        shaderProgram->bind();
        texture->bind(0);
        shaderProgram->setUniformValue(texLoc, 0);

        // Intensity scaling mapping HTML sliders (0-100) to shader logical values
        shaderProgram->setUniformValue(skinSmoothnessLoc, skinSmoothness / 100.0f);
        shaderProgram->setUniformValue(faceShapeLoc, faceShape / 100.0f);
        shaderProgram->setUniformValue(eyeSizeLoc, eyeSize / 100.0f);

        // Set landmarks (if any detected)
        if (faceLandmarks.size() >= 468) {
            // Note: MediaPipe Y is top-to-bottom, OpenGL is bottom-to-top (because we mirrored image)
            // So we invert Y for the shader
            QVector2D lEye(faceLandmarks[159].x, 1.0f - faceLandmarks[159].y);
            QVector2D rEye(faceLandmarks[386].x, 1.0f - faceLandmarks[386].y);
            QVector2D cn(faceLandmarks[152].x, 1.0f - faceLandmarks[152].y);

            shaderProgram->setUniformValue(leftEyeLoc, lEye);
            shaderProgram->setUniformValue(rightEyeLoc, rEye);
            shaderProgram->setUniformValue(chinLoc, cn);
        } else {
            // Defaults (center of screen)
            shaderProgram->setUniformValue(leftEyeLoc, QVector2D(0.4f, 0.6f));
            shaderProgram->setUniformValue(rightEyeLoc, QVector2D(0.6f, 0.6f));
            shaderProgram->setUniformValue(chinLoc, QVector2D(0.5f, 0.2f));
        }

        glBindVertexArray(vao);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);

        shaderProgram->release();
    }
}
