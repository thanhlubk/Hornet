#pragma once

#include <QMainWindow>
#include <QGraphicsDropShadowEffect>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <FancyWidgets/FTable.h>
#include <FancyWidgets/FTreeWidget.h>
#include <HornetBase/NotifyDispatcher.h>
#include <HornetBase/DatabaseSession.h>
#include <FancyWidgets/FSplitWidget.h>

namespace Ui {
class MainWindow;
}

class AppBase;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(AppBase* app, QWidget *parent = nullptr);
    ~MainWindow();

    static QWidget *createTreeWidget();
    // static void addRowToTable(FTable *table);
    static void populateTree(FTreeWidget *tree);
protected:
    void initTheme();
    void initWindow();

    void initTestOnly();
private slots:
    void on_fancyButton_clicked();
    void change_active_doc(FSplitWidget* node);

private:
    void createDocumentModel();
    Ui::MainWindow *ui;
    AppBase* m_app;
    std::unordered_map<QWidget*, QString> m_widgetToName;

};
