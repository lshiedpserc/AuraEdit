#pragma once

#include <QMainWindow>

class GLImageWidget;
class QWebEngineView;
class AppController;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    GLImageWidget *glWidget;
    QWebEngineView *webView;
    AppController *appController;

    void setupUI();
    void setupWebChannel();
};
