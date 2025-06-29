#include "MainWindow.h"
#include <FancyWidgets/FancyButton.h>
#include <QVBoxLayout>
#include <QWidget>
#include <HornetView/GLViewWindow.h>
#include "ui_MainWindow.h"
#include "SecondDialog.h"
#include <QTextEdit>
#include <FancyWidgets/FOutputWidget.h>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent), ui(new Ui::MainWindow)
{
    // Init data
    initTheme();

    // Remove native title bar
    setWindowFlags(Qt::FramelessWindowHint | Qt::Window);
    setAttribute(Qt::WA_TranslucentBackground, false);

    // Setup UI
    ui->setupUi(this);

    connect(ui->fancyButton, &QPushButton::clicked, this, []() {
        SecondDialog dialog;
        dialog.exec();
    });

    // Test
    ui->horizontalLayout->setContentsMargins(4, 0, 4, 4);
    ui->horizontalLayout->setSpacing(4);
    ui->sideBar->readJson(":/HornetMain/res/template/SideBar.json");
    ui->glViewWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    ui->titleBar->setAppTitleIcon("", ":/HornetMain/res/icon/custom/bee.png");
    ui->tabBar->setFixedHeight(200);
    ui->tabBar->addTabBar(100);
    ui->tabBar->addTabBar(100);

    for (auto i = 0; i < 6; i++)
    {
        FOutputWidget *output = new FOutputWidget(this);
        output->enableInput(true); // Enable input area
        output->appendMessage("Info: Build started", Qt::white);
        output->appendMessage("Warning: Missing include", Qt::yellow);
        output->appendMessage("Error: Failed to compile", Qt::red);

        connect(output, &FOutputWidget::userInputSubmitted, this, [](const QString &cmd)
                {
                    qDebug() << "User typed:" << cmd;
                    // Handle commands or routing here
                });

        QString title = QString("Output %1").arg(QString::number(i));
        if (i > 2)
            ui->tabBar->addPane(1, title, output);
        else
            ui->tabBar->addPane(0, title, output);
    }
    

    // ui->tabWidget_2->setFixedHeight(200);

    // ui->tabWidget->addTab(new QTextEdit("Tab 1 in Widget 1"), "Tab 1");
    // ui->tabWidget->addTab(new QTextEdit("Tab 2 in Widget 1"), "Tab 2");
    // ui->tabWidget_2->addTab(new QTextEdit("Tab A in Widget 2"), "Tab A");
    // ui->tabWidget_2->addTab(new QTextEdit("Tab B in Widget 2"), "Tab B");


    // Connect signal
    connect(ui->titleBar, &FTitleBar::signalClose, this, &MainWindow::close);
    connect(ui->titleBar, &FTitleBar::signalMinimize, [&]() {
        this->showMinimized();
    });
    connect(ui->titleBar, &FTitleBar::signalToggleMaximize, [&]() {
        static bool maximized = false;
        maximized = !maximized;
        maximized ? this->showMaximized() : this->showNormal();
    });
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::initTheme()
{
    FThemeManager::instance().load(":/HornetMain/res/theme/Themes.json", ":/HornetMain/res/theme/NumericDisplay.json");
    FThemeManager::instance().setTheme("Dark");
}
