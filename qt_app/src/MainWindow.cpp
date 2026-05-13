#include "MainWindow.h"
#include "GLImageWidget.h"
#include "AppController.h"

#include <QWebEngineView>
#include <QWebChannel>
#include <QVBoxLayout>
#include <QResizeEvent>
#include <QFileDialog>
#include <QAction>
#include <QMenuBar>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setupUI();
    setupWebChannel();

    // Add a simple menu to load images for testing
    QMenu *fileMenu = menuBar()->addMenu("&File");
    QAction *openAction = fileMenu->addAction("&Open Image...");
    connect(openAction, &QAction::triggered, this, [this]() {
        QString fileName = QFileDialog::getOpenFileName(this, "Open Image", "", "Images (*.png *.xpm *.jpg)");
        if (!fileName.isEmpty()) {
            glWidget->loadImage(fileName);
        }
    });
}

MainWindow::~MainWindow() {
}

void MainWindow::setupUI() {
    // 1. OpenGL Backend Widget
    glWidget = new GLImageWidget(this);
    setCentralWidget(glWidget);

    // 2. WebEngine UI overlay
    webView = new QWebEngineView(glWidget); // Parent to glWidget to overlay

    // Make WebEngine background transparent
    webView->page()->setBackgroundColor(Qt::transparent);
    webView->setAttribute(Qt::WA_TranslucentBackground);
    webView->setStyleSheet("background:transparent;");

    // Load UI
    webView->setUrl(QUrl("qrc:/ui/editor.html"));
}

void MainWindow::setupWebChannel() {
    QWebChannel *channel = new QWebChannel(this);
    appController = new AppController(this);

    // Connect controller to GL widget
    connect(appController, &AppController::skinSmoothnessChanged, glWidget, &GLImageWidget::setSkinSmoothness);
    connect(appController, &AppController::faceShapeChanged, glWidget, &GLImageWidget::setFaceShape);
    connect(appController, &AppController::eyeSizeChanged, glWidget, &GLImageWidget::setEyeSize);

    // Register object to be accessible from JS as 'appController'
    channel->registerObject(QStringLiteral("appController"), appController);
    webView->page()->setWebChannel(channel);
}

void MainWindow::resizeEvent(QResizeEvent *event) {
    QMainWindow::resizeEvent(event);
    if (webView) {
        webView->resize(event->size());
    }
}
