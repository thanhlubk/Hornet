#include <FancyWidgets/FTitleBar.h>
#include <QStyle>
#include <QApplication>

FTitleBar::FTitleBar(QWidget* parent)
    : QWidget(parent)
{
    setFixedHeight(40);

    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setContentsMargins(10, 0, 10, 0);
    layout->setSpacing(10);

    // Logo or app icon
    QLabel* logo = new QLabel(this);
    logo->setPixmap(QPixmap(":/icons/logo.png").scaled(24, 24, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    layout->addWidget(logo);

    layout->addStretch();

    // Buttons (Minimize, Maximize, Close)
    QPushButton* minimizeBtn = new QPushButton("🟢", this);
    QPushButton* maximizeBtn = new QPushButton("🟡", this);
    QPushButton* closeBtn    = new QPushButton("🔴", this);

    for (QPushButton* btn : { minimizeBtn, maximizeBtn, closeBtn }) {
        btn->setFixedSize(20, 20);
        btn->setStyleSheet("QPushButton { border: none; background: transparent; font-size: 16px; }"
                           "QPushButton:hover { background-color: #dddddd; border-radius: 10px; }");
        layout->addWidget(btn);
    }

    // Connect signals
    connect(closeBtn, &QPushButton::clicked, this, &FTitleBar::signalClose);
    connect(minimizeBtn, &QPushButton::clicked, this, &FTitleBar::signalMinimize);
    connect(maximizeBtn, &QPushButton::clicked, this, &FTitleBar::signalToggleMaximize);
}

void FTitleBar::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton)
        dragStartPos = event->globalPos() - window()->frameGeometry().topLeft();
}

void FTitleBar::mouseMoveEvent(QMouseEvent* event)
{
    if ((event->buttons() & Qt::LeftButton) && (window()->windowFlags() & Qt::FramelessWindowHint)) {
        window()->move(event->globalPos() - dragStartPos);
    }
}
