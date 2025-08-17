#pragma once

#include <QMainWindow>
#include <QGraphicsDropShadowEffect>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <FancyWidgets/FTable.h>
#include <FancyWidgets/FTreeWidget.h>

class FancyButton;

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    static QWidget *createTreeWidget();
    static void addRowToTable(FTable *table);
    static void populateTree(FTreeWidget *tree);
protected:
    void initTheme();

private:
    Ui::MainWindow *ui;
};
