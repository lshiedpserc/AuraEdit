#include <QApplication>
#include <QWebEngineProfile>
#include "MainWindow.h"

int main(int argc, char *argv[]) {
    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);

    QApplication app(argc, argv);

    MainWindow window;
    window.resize(1280, 800);
    window.show();

    return app.exec();
}
