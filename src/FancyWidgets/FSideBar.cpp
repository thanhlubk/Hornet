#include <FancyWidgets/FSideBar.h>
#include <FancyWidgets/FHorizontalSeparator.h>
#include <FancyWidgets/FUtil.h>
#include <QEvent>
#include <QThread>
#include <QLabel>
#include <QParallelAnimationGroup>
#include <QPropertyAnimation>
#include <QMouseEvent>
#include <FancyWidgets/FArrowButton.h>
#include <FancyWidgets/FPanelSplitButton.h>
#include <FancyWidgets/FPanel.h>
#include <QFile>
#include <QJsonParseError>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>

FSideBar::FSideBar(QWidget* parent)
    : QWidget(parent), FThemeableWidget()
{
    m_pLayout = new QHBoxLayout(this);
    m_pLayout->setSpacing(0);
    setLayout(m_pLayout);

    // Let layout manage height, we will manage width
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);

    m_pScrollArea = new FSideBarScrollArea(this);

    m_pStackedWidget = new FSideBarStackedWidget(this);
    m_pStackedWidget->setMinimumWidth(0);     // Allows collapsing to zero
    m_pStackedWidget->setMaximumWidth(0);     // Initial max width
    m_pStackedWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

    m_pLayout->addWidget(m_pScrollArea);
    m_pLayout->addWidget(m_pStackedWidget);
    m_pLayout->addStretch(); 
    observeWidget(m_pScrollArea);
    observeWidget(m_pStackedWidget);

    // Optional: initial sizing
    updateContainerWidth();

    connect(m_pScrollArea, &FSideBarScrollArea::scrollingPositionChanged, m_pStackedWidget, &FSideBarStackedWidget::updateFillet);
    connect(m_pStackedWidget, &FSideBarStackedWidget::collapseChanged, this, &FSideBar::toggleCollapse);

    SET_UP_THEME(FSideBar)
}

void FSideBar::applyTheme() 
{
    m_pLayout->setContentsMargins(0, 0, 0, 0);
    // m_pLayout->setContentsMargins(0, 0, 0, GET_OFFSET_SIZE(1));

    m_pScrollArea->setFixedWidth(72);

}

QColor FSideBar::getColorTheme(int idx)
{
    SET_UP_COLOR(0, Default, backgroundApp);
    SET_UP_COLOR(0, Custom1, backgroundApp);
    return QColor(qRgb(0, 0, 0));
}

QVariant FSideBar::getDisplaySize(int idx)
{
    return QVariant(0);
}

void FSideBar::observeWidget(QWidget* child) {
    if (!child || m_listObservedChild.contains(child))
        return;

    m_listObservedChild.append(child);
    child->installEventFilter(this);
}

bool FSideBar::eventFilter(QObject* watched, QEvent* event) {
    if (event->type() == QEvent::Resize) {
        updateContainerWidth();
    }
    return QWidget::eventFilter(watched, event);
}

#if 0
QWidget* FSideBar::createPage(const QString &xml)
{
    QWidget* page = new QWidget;
    QVBoxLayout* layout = new QVBoxLayout(page);
    layout->setSpacing(0);
    layout->setContentsMargins(0, 0, 0, 0);


    // Temporary
    auto pLabel = new QLabel(xml, page);
    pLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    FUtil::setFontWidget(pLabel, GET_FONT_SIZE(HeaderSize1), true);
    pLabel->setStyleSheet("background: yellow");

    auto pSeparator = new FHorizontalSeparator(page);
    pSeparator->setFixedHeight(10);
    pSeparator->setLineWidth(2);
    pSeparator->setLinePosition(3);
    pSeparator->setOffsetLeft(5);
    pSeparator->setOffsetRight(5);
    pSeparator->setColorBackground(QColor("green"));
    pSeparator->setColorLine(QColor("blue"));

    auto testButton = new FArrowButton("Check Button", QIcon("D:\\VMShare\\Hornet\\src\\HornetMain\\res\\toolbar-button-cube-normal.png"),  page);
    connect(testButton, &FArrowButton::clicked, this, []() { qDebug() << "Text/left side clicked";});
    // connect(testButton, &FArrowButton::arrowToggled, this, [](bool expanded) { qDebug() << "Arrow toggled: " << expanded; });

    auto button = new FPanelSplitButton(m_iFillet, page);
    button->setIcon(QIcon("D:\\VMShare\\Hornet\\src\\HornetMain\\res\\toolbar-button-cube-normal.png"));
    button->setText("Settings");

    auto menu = new FPanel(this);

    layout->addWidget(pLabel);
    layout->addWidget(pSeparator);
    layout->addWidget(testButton);
    layout->addWidget(button);
    layout->addWidget(menu);

    layout->addStretch();

    // page->setStyleSheet(QString("background-color: red; border-radius: %1px; margin-top: %2px; margin-bottom: %2px;").arg(QString::number(m_iFillet*2), QString::number(OFFSET_SIZE_2)));
    page->setStyleSheet(QString("background-color: red; border-radius: %1px;").arg(QString::number(m_iFillet*2)));


    return page;
}
#endif

void FSideBar::updateContainerWidth() {
    int totalWidth = 0;
    for (int i = 0; i < m_pLayout->count(); ++i) {
        QWidget* widget = m_pLayout->itemAt(i)->widget();
        if (widget) {
            totalWidth += widget->width();
        }
    }

    resize(totalWidth, height());         // only update width
    setFixedWidth(totalWidth);            // lock it so parent layout respects it
}

#if 0
void FSideBar::createTab(int idx, const QString &icon1, const QString &icon2, const QString& xml, bool bPin)
{
    FSideBarTab* button = new FSideBarTab(m_pScrollArea);
    button->setIcon(icon1, icon2);

    // Temp name
    button->setTitle(QString("Item %1").arg(idx));

    if (m_pScrollArea)
    {
        if (!bPin)
            m_pScrollArea->addItem(button);
        else
            m_pScrollArea->addPinItem(button);
    }

    // m_pStackedWidget->registerPage(idx, [this, xml]() {
    //     return this->m_pStackedWidget->addPageTest(xml);
    // });

    connect(button, &FSideBarTab::clicked, [=]() {
        m_pStackedWidget->setCurrentLazyIndex(idx);
    });
}
#endif

void FSideBar::createTabSetting(const QString &icon1, const QString &icon2)
{

}

void FSideBar::createTabOutline(const QString &icon1, const QString &icon2)
{
    
}

void FSideBar::readJson(const QString& jsonFile)
{
    // Open file
    QFile file(jsonFile);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "Failed to open file:" << jsonFile;
        return;
    }

    // Parse JSON
    QByteArray jsonData = file.readAll();
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(jsonData, &parseError);
    if (doc.isNull()) {
        qDebug() << "JSON parsing error:" << parseError.errorString();
        return;
    }

    // Check if root is an array
    if (!doc.isArray()) {
        qDebug() << "Root must be an array";
        return;
    }

    // Parse tabs
    QJsonArray tabsArray = doc.array();
    for (const QJsonValue& tabVal : tabsArray) {
        if (!tabVal.isObject()) continue;
        QJsonObject jsonObj = tabVal.toObject();

        if (!CHECK_JSON_TYPE_VALID(jsonObj, type) || jsonObj["type"].toString() != "Tab")
            continue;
        
        QString strTitle;
        QString strIcon;
        QString strIconCheck;
        bool bKeepIcon = false;

        GET_JSON_VALUE_STRING(jsonObj, strTitle, title)
        GET_JSON_VALUE_STRING(jsonObj, strIcon, icon)
        GET_JSON_VALUE_STRING(jsonObj, strIconCheck, iconCheck)
        GET_JSON_VALUE_BOOL(jsonObj, bKeepIcon, keepIcon)

        FSideBarTab* button = new FSideBarTab(m_pScrollArea);
        button->setIcon(strIcon, strIconCheck);
        button->setTitle(strTitle);

        m_pScrollArea->addItem(button);

        if (CHECK_JSON_TYPE_VALID(jsonObj, childs)) 
        {
            QJsonArray panelsArray = jsonObj["childs"].toArray();

            auto idx = m_pStackedWidget->registerPage([this, panelsArray, strTitle]() {
                return this->m_pStackedWidget->addPageTest2(panelsArray, strTitle);
            });

            connect(button, &FSideBarTab::clicked, [=]() {
                m_pStackedWidget->setCurrentLazyIndex(idx);
            });
        }
    }
}

// void FSideBar::setScrollIcon(const QString &iconPath, bool bKeep)
// {
//     m_pScrollArea->setScrollIcon(iconPath, bKeep);
// }

// void FSideBar::setCollapseIcon(const QString &iconPath, bool bKeep)
// {
//     m_pStackedWidget->setCollapseIcon(iconPath, bKeep);
// }

void FSideBar::toggleCollapse()
{
    int startWidth = m_pStackedWidget->getCollapseWidth();
    int endWidth = 0;
    int animationTime = 100;
    if (m_pStackedWidget->isCollapsed())
    {
        startWidth = 0;
        endWidth = m_pStackedWidget->getCollapseWidth();
    }

    QPropertyAnimation* anim = new QPropertyAnimation(m_pStackedWidget, "maximumWidth");
    anim->setDuration(animationTime);
    anim->setStartValue(startWidth);
    anim->setEndValue(endWidth);
    anim->setEasingCurve(QEasingCurve::OutQuad);

    QPropertyAnimation* anim2 = new QPropertyAnimation(m_pStackedWidget, "minimumWidth");
    anim2->setDuration(animationTime);
    anim2->setStartValue(startWidth);
    anim2->setEndValue(endWidth);
    // Create the parallel animation group
    QParallelAnimationGroup* group = new QParallelAnimationGroup;

    // Add animations to the group
    group->addAnimation(anim);
    group->addAnimation(anim2);

    connect(anim, &QVariantAnimation::valueChanged, this, [=]() {
        this->updateContainerWidth();  // manually recalculate total width
    });
    group->start(QAbstractAnimation::DeleteWhenStopped);

    m_pStackedWidget->setCollapsed(!(m_pStackedWidget->isCollapsed()));
    connect(anim, &QAbstractAnimation::finished, this, & FSideBar::updateContainerWidth);

    if (m_pStackedWidget->isCollapsed())
    {
        m_pScrollArea->uncheckAllTab();
    }
}