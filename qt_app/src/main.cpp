#include <QApplication>
#include <QWebEngineProfile>
#include <QWebEngineSettings>
#include "MainWindow.h"

int main(int argc, char *argv[]) {
    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts); // Important for Zero-copy / WebEngine interaction

    QApplication app(argc, argv);

    // Basic WebEngine settings
    QWebEngineSettings::defaultSettings()->setAttribute(QWebEngineSettings::JavascriptEnabled, true);
    QWebEngineSettings::defaultSettings()->setAttribute(QWebEngineSettings::LocalContentCanAccessRemoteUrls, true);
    QWebEngineSettings::defaultSettings()->setAttribute(QWebEngineSettings::LocalContentCanAccessFileUrls, true);

    MainWindow window;
    window.resize(1280, 800);
    window.show();

    return app.exec();
}
