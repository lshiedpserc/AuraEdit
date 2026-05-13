#pragma once

#include <QObject>

class AppController : public QObject {
    Q_OBJECT

public:
    explicit AppController(QObject *parent = nullptr);

public slots:
    // Called from JavaScript
    void onSkinSmoothnessChanged(int value);
    void onFaceShapeChanged(int value);
    void onEyeSizeChanged(int value);
    void logMessage(const QString &msg);

signals:
    // Emitted to C++
    void skinSmoothnessChanged(int value);
    void faceShapeChanged(int value);
    void eyeSizeChanged(int value);
};
