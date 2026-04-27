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
class HornetWindow;
}

class AppBase;

class HornetWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit HornetWindow(AppBase* app, QWidget *parent = nullptr);
    ~HornetWindow();

protected:
    void initWindow();

private slots:
    void onImportModel();

private:
    void createDocumentModel();
    Ui::HornetWindow *ui;
    AppBase* m_app;
    QWidget* m_pViewWidget;
};
