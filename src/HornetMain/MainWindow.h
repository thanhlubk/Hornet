#pragma once

#include <QMainWindow>
#include <QGraphicsDropShadowEffect>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>

class FancyButton;
// Single custom popup widget class

// class MyClickableWidget : public QWidget {
//     Q_OBJECT

// public:
//     explicit MyClickableWidget(QWidget* parent = nullptr);

// signals:
//     void customSignal(); // ✅ Public signal for external use

// protected:
//     void mousePressEvent(QMouseEvent* event) override;
// };

// class MyPopupWidget : public QWidget {
//     Q_OBJECT
// public:
//     explicit MyPopupWidget(QWidget* parent = nullptr);
//     ~MyPopupWidget(); 

// signals:
//     void requestOpenChildPopup(); // signal to open next-level popup

// public slots:
//     void appear();
// };

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    // MyPopupWidget* popup1;
    // MyPopupWidget* popup2;

protected:
    void initTheme();

private:
    Ui::MainWindow *ui;
};
