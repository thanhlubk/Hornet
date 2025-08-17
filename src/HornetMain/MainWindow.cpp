#include "MainWindow.h"
#include <FancyWidgets/FancyButton.h>
#include <QVBoxLayout>
#include <QWidget>
#include <HornetView/GLViewWindow.h>
#include "ui_MainWindow.h"
#include "SecondDialog.h"
#include <QTextEdit>
#include <FancyWidgets/FOutputWidget.h>
#include <FancyWidgets/FArrowButton.h>
#include <FancyWidgets/FPanel.h>
#include <QHeaderView>
#include <FancyWidgets/FListWidgetScrollAware.h>
#include <FancyWidgets/FPropertiesPalette.h>
#include <FancyWidgets/FThemeableWidget.h>

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

    ui->sideBar->createTab("Assemble", ":/HornetMain/res/icon/unfilled/portrait.png", ":/HornetMain/res/icon/filled/portrait.png", createTreeWidget, true);

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

    // // Test only
    // FTreeItem *treewidget = new FTreeItem("New Business", 33, this);
    // auto pButton = new FArrowButton("Expandable", "", ":/FancyWidgets/res/icon/small/forward.png", true, this);
    // ui->verticalLayout->addWidget(treewidget);
    // ui->verticalLayout->addWidget(pButton);
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

QWidget* MainWindow::createTreeWidget()
{
    auto pSplitter = new FSplitter(Qt::Vertical);
    pSplitter->setHandleSeparatorOffset(0, 4);
    pSplitter->setThemeStyle(FThemeableWidget::Custom1);

    pSplitter->setHandleWidth(2);
    pSplitter->setHandleSpacing(2);

    auto pScroll1 = new FListWidgetScrollAware(pSplitter);
    pScroll1->enableSpacingBorder(false);

    FTreeWidget *tree = new FTreeWidget(pScroll1);
    populateTree(tree);
    pScroll1->addWidget(tree);
    pScroll1->setSpacing(4);

    FPropertiesPalette *treeView = new FPropertiesPalette(pSplitter);
    treeView->setHeaderLabels({"Name", "Type"});
    treeView->setColumnCount(2); // One column: we customize item content fully
    auto pTest1 = treeView->addItem("EEEEEEEEEE", {"test1"});
    auto pTest3 = treeView->addItem("EEEEEEEEEE", {"test3"});

    treeView->addItem("EEEEEEEE", {"test12"}, pTest1);
    treeView->addItem("test13", {"test13"}, pTest1);
    auto pChild4 = treeView->addItem("test14", {"test14"}, pTest1);
    treeView->addItem("test141", {"test141"}, pChild4);
    treeView->addItem("test142", {"test142"}, pChild4);
    auto pChild6 = treeView->addItem("test16", {"test16"}, pTest1);
    treeView->addItem("test161", {"test161"}, pChild6);
    treeView->addItem("test162", {"test162"}, pChild6);
    auto pChild7 = treeView->addItem("test17", {"test17"}, pTest1);
    treeView->addItem("test171", {"test161"}, pChild7);
    treeView->addItem("test172", {"test162"}, pChild7);
    treeView->addItem("test15", {"test15"}, pTest1);
    auto pChild8 = treeView->addItem("test18", {"test18"}, pTest1);
    treeView->addItem("test171", {"test161"}, pChild8);
    treeView->addItem("test172", {"test162"}, pChild8);

    treeView->addItem("test32", {"test32"}, pTest3);
    treeView->addItem("test33", {"test33"}, pTest3);
    auto listChild = treeView->topLevelItemCount();
    pSplitter->addWidget(pScroll1);
    // pSplitter->addWidget(pScroll2);
    pSplitter->addWidget(treeView);

   
    return pSplitter;
}

void MainWindow::addRowToTable(FTable *table)
{
    if (!table)
        return;

    int row = table->rowCount();
    table->insertRow(row);

    for (int i = 0; i < 2; ++i)
    {
        if (i < table->columnCount())
        {
            auto pLabel = new QLabel(QString("Item %1-%2").arg(row, i), table);
            pLabel->setStyleSheet("QLabel { background: transparent; }");
            table->setCellWidget(row, i, pLabel);
        }
    }
}

void MainWindow::populateTree(FTreeWidget *tree)
{
    auto createLabel = [](const QString &text) -> QWidget *
    {
        FTreeItem *label = new FTreeItem(text, "D:\\Icon\\Apple\\bee.png");
        // auto label = new QPushButton(text);
        return label;
    };

    auto createCheckBox = []() -> QWidget *
    {
        FCheckBox *label = new FCheckBox();
        return label;
    };

    QList<QWidget*> listWidget;
    listWidget.append(createLabel("Desk 1"));
    // listWidget.append(createCheckBox());
    auto child1 = tree->addItem(nullptr, listWidget);

    QList<QWidget *> listWidget2;
    listWidget2.append(createLabel("1 - New Business"));
    // listWidget2.append(createCheckBox());
    auto child2 = tree->addItem(child1, listWidget2);

    QList<QWidget *> listWidget3;
    listWidget3.append(createLabel("2 - Contacted"));
    // listWidget3.append(createCheckBox());
    auto child3 = tree->addItem(child1, listWidget3);

    QList<QWidget *> listWidget4;
    listWidget4.append(createLabel("Non-Compliant"));
    // listWidget4.append(createCheckBox());
    auto child4 = tree->addItem(child3, listWidget4);

    QList<QWidget *> listWidget5;
    listWidget5.append(createLabel("Compliant"));
    // listWidget5.append(createCheckBox());
    auto child5 = tree->addItem(child3, listWidget5);

    QList<QWidget *> listWidget6;
    listWidget6.append(createLabel("Desk 2"));
    // listWidget6.append(createCheckBox());
    auto child6 = tree->addItem(nullptr, listWidget6);

    QList<QWidget *> listWidget7;
    listWidget7.append(createLabel("Desk 2"));
    // listWidget7.append(createCheckBox());
    auto child7 = tree->addItem(child6, listWidget7);

    // auto createLabel = [](const QString &text) -> QWidget *
    // {
    //     QLabel *label = new QLabel(text);
    //     return label;
    // };

    // // Top-level item: My Computer
    // QTreeWidgetItem *computer = new QTreeWidgetItem(tree);
    // tree->setItemWidget(computer, 0, createLabel("My Computer"));
    // tree->setItemWidget(computer, 1, createLabel("System"));
    // tree->setItemWidget(computer, 2, createLabel("-"));

    // // Child: Documents
    // QTreeWidgetItem *documents = new QTreeWidgetItem(computer);
    // tree->setItemWidget(documents, 0, createLabel("Documents"));
    // tree->setItemWidget(documents, 1, createLabel("Folder"));
    // tree->setItemWidget(documents, 2, createLabel("-"));

    // // Child of Documents: resume.pdf
    // QTreeWidgetItem *resume = new QTreeWidgetItem(documents);
    // tree->setItemWidget(resume, 0, createLabel("resume.pdf"));
    // tree->setItemWidget(resume, 1, createLabel("PDF File"));
    // tree->setItemWidget(resume, 2, createLabel("245 KB"));

    // // Another top-level item: Downloads
    // QTreeWidgetItem *downloads = new QTreeWidgetItem(tree);
    // tree->setItemWidget(downloads, 0, createLabel("Downloads"));
    // tree->setItemWidget(downloads, 1, createLabel("Folder"));
    // tree->setItemWidget(downloads, 2, createLabel("-"));

    // // Child: installer.exe
    // QTreeWidgetItem *installer = new QTreeWidgetItem(downloads);
    // tree->setItemWidget(installer, 0, createLabel("installer.exe"));
    // tree->setItemWidget(installer, 1, createLabel("Executable"));
    // tree->setItemWidget(installer, 2, createLabel("15 MB"));
}
