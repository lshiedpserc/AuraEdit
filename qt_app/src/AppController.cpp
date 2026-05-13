#include "AppController.h"
#include <QDebug>

AppController::AppController(QObject *parent) : QObject(parent) {}

void AppController::onSkinSmoothnessChanged(int value) {
    qDebug() << "Skin smoothness changed:" << value;
    emit skinSmoothnessChanged(value);
}

void AppController::onFaceShapeChanged(int value) {
    qDebug() << "Face shape changed:" << value;
    emit faceShapeChanged(value);
}

void AppController::onEyeSizeChanged(int value) {
    qDebug() << "Eye size changed:" << value;
    emit eyeSizeChanged(value);
}

void AppController::logMessage(const QString &msg) {
    qDebug() << "JS Log:" << msg;
}
