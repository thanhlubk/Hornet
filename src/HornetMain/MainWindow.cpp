#include "MainWindow.h"
#include <FancyWidgets/FancyButton.h>
#include <QVBoxLayout>
#include <QWidget>
#include <HornetView/GLViewWindow.h>
#include "ui_MainWindow.h"
#include "SecondDialog.h"
#include <QParallelAnimationGroup>
#include <QThread>
#include <QColor>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent), ui(new Ui::MainWindow)
{
    // Init data
    initTheme();

    // Setup UI
    ui->setupUi(this);

    connect(ui->fancyButton, &QPushButton::clicked, this, []() {
        SecondDialog dialog;
        dialog.exec();
    });

    // Test
    // ui->autoSize->setScrollIcon(":/HornetMain/res/icon/filled/scroll-up.png");
    // ui->autoSize->setCollapseIcon(":/toolbar/res/close.png");
    ui->autoSize->readJson(":/HornetMain/res/template/SideBar.json");
    // for (int i = 1; i <= 20; ++i) {
    //     ui->autoSize->createTab(i, ":/toolbar/res/toolbar-button-home-normal.png", ":/toolbar/res/toolbar-button-home-check.png", QString("page %1").arg(i), false);
    // }

    // for (int i = 1; i <= 2; ++i) {
    //     ui->autoSize->createTab(i + 100, ":/toolbar/res/toolbar-button-home-normal.png", ":/toolbar/res/toolbar-button-home-check.png", QString("page %1").arg(i + 100), true);
    // }

    ui->glViewWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    // Remove native title bar
    setWindowFlags(Qt::FramelessWindowHint | Qt::Window);
    setAttribute(Qt::WA_TranslucentBackground, false);

    ui->titleBar->setAppTitleIcon("", ":/HornetMain/res/icon/custom/bee.png");

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

    // popup1 = new MyPopupWidget(this);
    // popup2 = new MyPopupWidget(this);

    // QObject::connect(popup1, &MyPopupWidget::requestOpenChildPopup, popup2, &MyPopupWidget::appear);

    // connect(ui->animationButton, &QPushButton::clicked, popup1, &MyPopupWidget::appear);
    
    // QIcon icon("D:/up.svg");
    // QPixmap pix = icon.pixmap(24, 24);
    // qDebug() << "Loaded:" << !pix.isNull();

    // QColor trans(0, 0, 0, 0);
    // qDebug() << "color:" << trans.name(QColor::HexArgb);
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

// MyPopupWidget::MyPopupWidget(QWidget* parent)
//     : QWidget(parent, Qt::Popup) // Important: Qt::Popup makes it behave like a dropdown
// {
//     auto* layout = new QVBoxLayout(this);
//     auto* btn = new QPushButton("Open Another Popup", this);
//     auto* btn2 = new MyClickableWidget(this);

//     layout->addWidget(btn);
//     layout->addWidget(btn2);

//     setLayout(layout);

//     connect(btn, &QPushButton::clicked, this, [this]() {
//         emit requestOpenChildPopup(); // notify that a child popup should be opened
//     });

//     connect(btn2, &MyClickableWidget::customSignal, this, [this]() {
//         emit requestOpenChildPopup(); // notify that a child popup should be opened
//     });
// }

// MyPopupWidget::~MyPopupWidget() {
//     qDebug() << "MyPopupWidget destroyed";
// }

// void MyPopupWidget::appear()
// {
//     auto pSender = qobject_cast<QWidget*>(sender());
//     QPoint globalPos = pSender->mapToGlobal(pSender->rect().bottomLeft());
//     move(globalPos);
//     setFocus();
//     show();
//     raise();
// }

// MyClickableWidget::MyClickableWidget(QWidget* parent)
//     : QWidget(parent)
// {
//     setFixedSize(120, 30);
//     setStyleSheet("background-color: lightgreen; border: 1px solid black;");
//     setCursor(Qt::PointingHandCursor);
//     setFocusPolicy(Qt::StrongFocus);
// }

// void MyClickableWidget::mousePressEvent(QMouseEvent* event)
// {
//     QWidget::mousePressEvent(event);  // Call base class handler
//     // emit clicked(); // ✅ Emit the signal
//     QMetaObject::invokeMethod(this, "customSignal", Qt::QueuedConnection);
// }