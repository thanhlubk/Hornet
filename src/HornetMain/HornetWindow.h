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
#include <HornetUtil/HVector.h>

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

    void solveLinearAnalysis();
    void solveCrackPropagation();

private slots:
    void onImportModel();
    void onSolve();
    void onShowResult();
    void onUnshowResult();
    void onToggleMeshLine();
    void onToggleNode();
    void onToggleLbc();
    void onToggleDeformation();
    void onStepChanged(int index);
    void onEnableCrack(bool enabled);

private:
    void createDocumentModel();
    Ui::HornetWindow *ui;
    AppBase* m_app;
    QWidget* m_pViewWidget;

    std::vector<std::vector<HVector2d>> m_vecCrack;
};
