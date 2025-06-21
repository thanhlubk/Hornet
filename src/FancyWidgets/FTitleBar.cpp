#include <FancyWidgets/FTitleBar.h>
#include <QStyle>
#include <QStyleOption>
#include <QPainter>
#include <FancyWidgets/FUtil.h>

FTitleBar::FTitleBar(QWidget* parent)
    : QWidget(parent)
{

    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // Logo or app icon
    m_pLabelAppIcon = new QLabel(this);
    m_pLabelAppIcon->setAlignment(Qt::AlignCenter);
    m_pLabelAppTitle = new QLabel(this);
    m_pLabelAppTitle->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    
    m_pSearchBar = new FSearchBar(this);
    m_pSearchBar->setPlaceholderText("Search here...");
    m_pSearchBar->setRole(Role::Secondary);
    
    layout->addWidget(m_pLabelAppIcon);
    layout->addWidget(m_pLabelAppTitle);

    layout->addStretch();
    layout->addWidget(m_pSearchBar);
    layout->addStretch();

    // Buttons (Minimize, Maximize, Close)
    auto layoutAlignButton = new QVBoxLayout();
    layoutAlignButton->setContentsMargins(0, 0, 0, 0);
    layoutAlignButton->setSpacing(0);

    auto layoutButton = new QHBoxLayout();
    layoutButton->setContentsMargins(0, 0, 0, 0);
    layoutButton->setSpacing(0);

    m_pButtonMinimize = new QPushButton("", this);
    m_pButtonMaximize = new QPushButton("", this);
    m_pButtonClose = new QPushButton("", this);

    for (QPushButton* pButton : { m_pButtonMinimize, m_pButtonMaximize, m_pButtonClose}) 
    {
        pButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
        layoutButton->addWidget(pButton);
    }

    layoutAlignButton->addLayout(layoutButton);
    layoutAlignButton->addStretch();
    layout->addLayout(layoutAlignButton);

    // Connect signals
    connect(m_pButtonClose, &QPushButton::clicked, this, &FTitleBar::signalClose);
    connect(m_pButtonMinimize, &QPushButton::clicked, this, &FTitleBar::signalMinimize);
    connect(m_pButtonMaximize, &QPushButton::clicked, this, &FTitleBar::signalToggleMaximize);

    SET_UP_THEME(FTitleBar)
}

void FTitleBar::applyTheme() 
{
    updateIcon();
    int count = 0;
    QVector<QString> vecIcon = {":/FancyWidgets/res/icon/small/close.png", ":/FancyWidgets/res/icon/small/minus.png", ":/FancyWidgets/res/icon/small/window.png"};
    for (QPushButton* pButton : { m_pButtonClose, m_pButtonMinimize, m_pButtonMaximize }) 
    {
        pButton->setIcon(FUtil::changeIconColor(vecIcon[count], getColorTheme(2)));

        pButton->setIconSize(QSize(getDisplaySize(0).toInt(), getDisplaySize(0).toInt()));
        pButton->setFixedSize(32, getDisplaySize(5).toInt() - getDisplaySize(3).toInt()*3);
        pButton->setStyleSheet(QString("QPushButton { border: none; background: %1; } QPushButton:hover { background-color: %2; }").arg(getColorTheme(0).name(QColor::HexArgb), count == 0 ? getColorTheme(4).name() : getColorTheme(3).name()));

        count++;
    }

    m_pSearchBar->setFixedWidth(getDisplaySize(4).toInt());
    setStyleSheet(QString("background: %1;").arg(getColorTheme(1).name()));
    setFixedHeight(getDisplaySize(5).toInt());
}

QColor FTitleBar::getColorTheme(int idx)
{
    SET_UP_COLOR(0, Default, transparent);
    SET_UP_COLOR(1, Default, backgroundApp);
    SET_UP_COLOR(2, Default, textHeader1);
    SET_UP_COLOR(3, Default, backgroundSection1);
    SET_UP_COLOR(4, Default, fail);
    SET_UP_COLOR(5, Default, textHeader2);

    return QColor(qRgb(0, 0, 0));
}

QVariant FTitleBar::getDisplaySize(int idx)
{
    SET_UP_DISPLAY_SIZE(0, Primary, iconSize3);
    SET_UP_DISPLAY_SIZE(1, Primary, sidebarTabWidth);
    SET_UP_DISPLAY_SIZE(2, Primary, offsetSize1);
    SET_UP_DISPLAY_SIZE(3, Primary, offsetSize2);
    SET_UP_DISPLAY_SIZE(4, Primary, searchbarWidth);
    SET_UP_DISPLAY_SIZE(5, Primary, titlebarHeight);
    SET_UP_DISPLAY_SIZE(6, Primary, fontNormalSize);
    SET_UP_DISPLAY_SIZE(7, Primary, iconSize1);

    return QVariant(0);
}

void FTitleBar::paintEvent(QPaintEvent* event)
{
    QStyleOption opt;
    opt.initFrom(this);
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}

void FTitleBar::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton)
        m_pointDragStartPos = event->globalPos() - window()->frameGeometry().topLeft();
}

void FTitleBar::mouseMoveEvent(QMouseEvent* event)
{
    if ((event->buttons() & Qt::LeftButton) && (window()->windowFlags() & Qt::FramelessWindowHint)) {
        window()->move(event->globalPos() - m_pointDragStartPos);
    }
}

void FTitleBar::setAppTitleIcon(const QString &text, const QString &icon)
{
    m_strIconApp = icon;
    m_pLabelAppTitle->setText(text);
    updateIcon();
}

void FTitleBar::setAppIcon(const QString &icon)
{
    m_strIconApp = icon;
    updateIcon();
}

void FTitleBar::setAppTitle(const QString &text)
{
    m_pLabelAppTitle->setText(text);
    updateIcon();
}

void FTitleBar::updateIcon()
{
    m_pLabelAppIcon->setStyleSheet(QString("background: %1; margin-left: %2px; margin-right: %3px;").arg(getColorTheme(0).name(QColor::HexArgb), QString::number(getDisplaySize(3).toInt()*3), QString::number(getDisplaySize(3).toInt())));

    if (!m_strIconApp.isNull())
    {
        QPixmap pixmap = QPixmap(m_strIconApp);
        m_pLabelAppIcon->setPixmap(pixmap.scaled(getDisplaySize(7).toInt(), getDisplaySize(7).toInt(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }

    m_pLabelAppTitle->setStyleSheet(QString("background: %1; color: %3;").arg(getColorTheme(0).name(QColor::HexArgb), getColorTheme(5).name()));
    // m_pLabelAppTitle->setStyleSheet(QString("background: %1;").arg(getColorTheme(0).name(QColor::HexArgb)));

    FUtil::setFontWidget(m_pLabelAppTitle, getDisplaySize(6).toInt(), false);
}