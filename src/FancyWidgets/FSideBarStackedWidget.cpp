#include <FancyWidgets/FSideBarStackedWidget.h>
#include <FancyWidgets/FUtil.h>
#include <QVBoxLayout>
#include <FancyWidgets/FArrowButton.h>
#include <FancyWidgets/FPanelSplitButton.h>
#include <FancyWidgets/FPanelButton.h>
#include <FancyWidgets/FPanel.h>
#include <FancyWidgets/FHorizontalSeparator.h>
#include <FancyWidgets/FExpandableButton.h>
#include <QJsonParseError>
#include <QJsonObject>
#include <QJsonDocument>

FSideBarStackedWidget::FSideBarStackedWidget(QWidget* parent)
    : QStackedWidget(parent), FThemeableWidget(), m_bCollapsed(true), m_iStartWidth(0), m_iCollapseWidth(220), m_bResizing(false)
{
    m_pFrameTop = new FSideBarStackedFilletFrame(this);
    m_pFrameBottom = new FSideBarStackedFilletFrame(this);

    m_pFrameTop->setAbove(true);
    m_pFrameTop->enablePaint(false);
    m_pFrameBottom->setAbove(false);
    m_pFrameBottom->enablePaint(false);

    m_pCollapseButton = new QPushButton("", this);
    m_pCollapseButton->raise();

    connect(m_pCollapseButton, &QPushButton::clicked, this, [this]() 
    {
        emit this->collapseChanged(); 
    });

    SET_UP_THEME(FSideBarStackedWidget)
}

void FSideBarStackedWidget::applyTheme() 
{
    setStyleSheet(QString("FSideBarStackedWidget { background: %1; }").arg(getColorTheme(3).name(QColor::HexArgb)));
    
    if (m_pCollapseButton)
    {
        m_pCollapseButton->setStyleSheet(QString("background: %1; text-align: center;").arg(getColorTheme(3).name(QColor::HexArgb)));
        m_pCollapseButton->resize(getDisplaySize(2).toInt(), getDisplaySize(2).toInt()); // Set button size
    }

    if (m_pFrameTop && m_pFrameBottom)
    {
        m_pFrameTop->setBrushColor(getColorTheme(0));
        m_pFrameBottom->setBrushColor(getColorTheme(0));
    }

    updateCollapseIcon();
    updateStyleCurrentPage();
}

QColor FSideBarStackedWidget::getColorTheme(int idx)
{
    SET_UP_COLOR(0, Default, backgroundSection1);
    SET_UP_COLOR(1, Default, textNormal);
    SET_UP_COLOR(2, Default, dominant);
    SET_UP_COLOR(3, Default, transparent);

    return QColor(qRgb(0, 0, 0));
}

QVariant FSideBarStackedWidget::getDisplaySize(int idx)
{
    SET_UP_DISPLAY_SIZE(0, Primary, filletSize2);
    SET_UP_DISPLAY_SIZE(1, Primary, fontHeaderSize2);
    SET_UP_DISPLAY_SIZE(2, Primary, iconSize1);
    SET_UP_DISPLAY_SIZE(3, Primary, iconSize3);
    SET_UP_DISPLAY_SIZE(4, Primary, offsetSize2);
    SET_UP_DISPLAY_SIZE(5, Primary, separatorWidth);
    SET_UP_DISPLAY_SIZE(6, Primary, scrollbarWidth);

    return QVariant(0);
}

void FSideBarStackedWidget::registerPage(int index, PageFactory factory) {
    m_mapPageFactories[index] = factory;
}

int FSideBarStackedWidget::registerPage(PageFactory factory) {
    int index = m_mapPageFactories.size();
    m_mapPageFactories[index] = factory;
    return index;
}

void FSideBarStackedWidget::setCurrentLazyIndex(int index) {
    if (!m_setInitializedPages.contains(index)) 
    {
        if (m_mapPageFactories.find(index) != m_mapPageFactories.end()) 
        {
            QWidget* page = m_mapPageFactories[index]();
            addWidget(page);
            m_setInitializedPages.insert(index);
            m_mapItemPageIdx[index] = count() - 1;
        }
    }
    setCurrentIndex(m_mapItemPageIdx[index]);
    updateStyleCurrentPage();
    
    QWidget* pCurrentWidget = this->widget(m_mapItemPageIdx[index]);
    if (pCurrentWidget)
    {
        pCurrentWidget->setMouseTracking(true);
        pCurrentWidget->installEventFilter(this);
        enableMouseTrackingRecursive(pCurrentWidget);
    }

    if (isCollapsed())
        emit collapseChanged();

    m_pCollapseButton->raise();
}

bool FSideBarStackedWidget::isCollapsed() const
{
    return m_bCollapsed;
}

void FSideBarStackedWidget::setCollapsed(bool collapse)
{
    m_bCollapsed = collapse;
}

int FSideBarStackedWidget::getCollapseWidth() 
{ 
    return m_iCollapseWidth; 
}

void FSideBarStackedWidget::resizeEvent(QResizeEvent* event)
{
    QStackedWidget::resizeEvent(event);
    auto iFillet = getDisplaySize(0).toInt();
    if (m_pFrameTop)
    {
        if (width() < iFillet*3)
            m_pFrameTop->setFixedSize(0, 0);
        else
            m_pFrameTop->setFixedSize(iFillet*2 + 5, iFillet*2);

        m_pFrameTop->move(0, 0);
        m_pFrameTop->lower();
    }

    if (m_pFrameBottom)
    {
        if (width() < iFillet*3)
            m_pFrameBottom->setFixedSize(0, 0);
        else
            m_pFrameBottom->setFixedSize(iFillet*2 + 5, iFillet*2);

        m_pFrameBottom->move(0, height() - iFillet);

        m_pFrameBottom->lower();
    }

    if (m_pCollapseButton) 
    {
        m_pCollapseButton->move(width() - m_pCollapseButton->width(), 0);
        m_pCollapseButton->raise();
    }
}

void FSideBarStackedWidget::updateFillet(int top, int bottom, bool pin)
{
    m_pFrameTop->enablePaint(false);
    m_pFrameBottom->enablePaint(false);
    auto iFillet = getDisplaySize(0).toInt();

    if (!pin)
    {
        int topPosition = top - iFillet/2;
        int bottomPosition = bottom + iFillet/2;

        if (topPosition >= iFillet || bottomPosition <= 0)
            m_pFrameTop->enablePaint(false);
        else
        {
            m_pFrameTop->setFilletPosition(topPosition);
            m_pFrameTop->enablePaint(true);
        }
    }
    else
    {
        auto bottomPos = height();
        if (bottom < bottomPos)
            m_pFrameBottom->enablePaint(false);
        else
        {
            m_pFrameBottom->setFilletPosition(0);
            m_pFrameBottom->enablePaint(true);
        }
    }

    m_pFrameTop->update();
    m_pFrameBottom->update();
}

bool FSideBarStackedWidget::eventFilter(QObject* obj, QEvent* event)
{
    if (event->type() == QEvent::MouseMove)
    {
        QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
        if (m_bResizing)
        {
            int newWidth = m_iStartWidth + mouseEvent->globalX() - m_pointStartPos.x();

            setMinimumWidth(newWidth);
            setMaximumWidth(newWidth);

            resize(newWidth, height());
            m_iCollapseWidth = newWidth;
        } 
        else 
        {
            if (isNearRightEdge(mouseEvent->pos()))
                setCursor(Qt::SizeHorCursor);
            else
                setCursor(Qt::ArrowCursor);
        }
    }
    return QWidget::eventFilter(obj, event);
}

void FSideBarStackedWidget::enableMouseTrackingRecursive(QWidget* widget)
{
    widget->setMouseTracking(true);

    for (QObject* child : widget->children())
    {
        QWidget* childWidget = qobject_cast<QWidget*>(child);
        if (childWidget)
            enableMouseTrackingRecursive(childWidget);
    }
}

void FSideBarStackedWidget::mousePressEvent(QMouseEvent* event)
{
    if (isNearRightEdge(event->pos()))
    {
        m_bResizing = true;
        m_pointStartPos = event->globalPos();
        m_iStartWidth = width();
    }
    QWidget::mousePressEvent(event);
}

void FSideBarStackedWidget::mouseReleaseEvent(QMouseEvent* event)
{
    m_bResizing = false;
    QWidget::mouseReleaseEvent(event);
}

bool FSideBarStackedWidget::isNearRightEdge(const QPoint& pos) const
{
    const int edgeMargin = 2;
    return (width() - pos.x()) < edgeMargin;
}

void FSideBarStackedWidget::updateCollapseIcon()
{
    QIcon icon = FUtil::changeIconColor(":/FancyWidgets/res/icon/small/close.png", getColorTheme(1));
    m_pCollapseButton->setIcon(icon);
    m_pCollapseButton->setIconSize(QSize(getDisplaySize(3).toInt(), getDisplaySize(3).toInt())); // Adjust size as needed
}

void FSideBarStackedWidget::updateStyleCurrentPage()
{
    QWidget* pCurrentWidget = currentWidget();
    if (!pCurrentWidget)
        return;
    auto pLayout = pCurrentWidget->layout();
    if (pLayout)
        pLayout->setContentsMargins(getDisplaySize(4).toInt(), getDisplaySize(4).toInt(), getDisplaySize(4).toInt()/2, getDisplaySize(4).toInt());

    for (QObject* obj : pCurrentWidget->children()) 
    {
        if (QLabel* pLabel = qobject_cast<QLabel*>(obj))
        {
            FUtil::setFontWidget(pLabel, getDisplaySize(1).toInt(), true);
            pLabel->setStyleSheet(QString("background: %1").arg(getColorTheme(0).name()));
        }
        else if (FHorizontalSeparator* pSeparator = qobject_cast<FHorizontalSeparator*>(obj))
        {
            pSeparator->setFixedHeight(getDisplaySize(4).toInt()*2);
            pSeparator->setLineWidth(getDisplaySize(5).toInt());

            pSeparator->setLinePosition(getDisplaySize(4).toInt());
            pSeparator->setOffsetLeft(0);
            pSeparator->setOffsetRight(getDisplaySize(4).toInt()/2);
            pSeparator->setColorBackground(QColor(getColorTheme(0).name()));
            pSeparator->setColorLine(QColor(getColorTheme(1).name()));
        }
        else if (ScrollAwareListWidget* pListContent = qobject_cast<ScrollAwareListWidget*>(obj))
        {
            pListContent->setSpacing(0);
            pListContent->reserveScrollBarSpace(getDisplaySize(6).toInt() + getDisplaySize(4).toInt()/2);

            auto strScrollbarStyle = FUtil::getStyleScrollbarVertical2(getDisplaySize(6).toInt(), getDisplaySize(4).toInt()/2, getDisplaySize(4).toInt()/2, 0, ":/FancyWidgets/res/icon/small/up.png", ":/FancyWidgets/res/icon/small/down.png", getColorTheme(1).name(), getColorTheme(1).name());

            pListContent->setStyleSheet("QListWidget { background: transparent; padding-left: 0px; padding-right: 0px; padding-top: 0px; padding-bottom: 0px;} QListWidget::item { outline: none; border: none; }" + strScrollbarStyle);
        }
    }

    pCurrentWidget->setStyleSheet(QString("background-color: %2; border-radius: %1px;").arg(QString::number(getDisplaySize(0).toInt()*2), getColorTheme(0).name()));

}

#if 0
QWidget* FSideBarStackedWidget::addPageTest(const QString &xml)
{
    QWidget* page = new QWidget;
    QVBoxLayout* layout = new QVBoxLayout(page);
    layout->setSpacing(0);
    layout->setContentsMargins(GET_OFFSET_SIZE(2), GET_OFFSET_SIZE(2), GET_OFFSET_SIZE(1), GET_OFFSET_SIZE(2));

    // Temporary
    auto pLabel = new QLabel(xml + " TEST", page);
    pLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    FUtil::setFontWidget(pLabel, GET_FONT_SIZE(HeaderSize2), true);
    pLabel->setStyleSheet(QString("background: %1").arg(getColorTheme(0).name()));

    auto pSeparator = new FHorizontalSeparator(page);
    pSeparator->setFixedHeight(20);
    pSeparator->setLineWidth(2);
    pSeparator->setLinePosition(10);
    pSeparator->setOffsetLeft(0);
    pSeparator->setOffsetRight(0);
    pSeparator->setColorBackground(QColor(getColorTheme(0).name()));
    pSeparator->setColorLine(QColor(getColorTheme(1).name()));

    auto pListContent = new ScrollAwareListWidget(page);
    pListContent->setViewMode(QListView::ListMode);           // List mode
    pListContent->setFlow(QListView::TopToBottom);            // Flow left to right
    pListContent->setWrapping(false);                          // Wrap into rows
    pListContent->setResizeMode(QListView::Adjust);           // Adjust on resize
    pListContent->setWordWrap(true);
    pListContent->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
    pListContent->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    pListContent->setSpacing(0);
    pListContent->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    pListContent->reserveScrollBarSpace(8 + 4);

    // auto arrowUp = FUtil::getIconColorPath("D:/VMShare/Hornet/src/HornetMain/res/up.png", getColorTheme(1).name());
    // auto arrowDown = FUtil::getIconColorPath("D:/VMShare/Hornet/src/HornetMain/res/down.png", getColorTheme(1).name());
    pListContent->setFocusPolicy(Qt::NoFocus);

    auto test = FUtil::getStyleScrollbarVertical2(8, 4, 4, 0, "D:/VMShare/Hornet/src/HornetMain/res/up.png", "D:/VMShare/Hornet/src/HornetMain/res/down.png", getColorTheme(1).name(), getColorTheme(1).name());

    pListContent->setStyleSheet("QListWidget { background: transparent; padding-left: 0px; padding-right: 0px; padding-top: 0px; padding-bottom: 0px;} QListWidget::item { margin-bottom: 70px;/* Spacing between items */ outline: none; border: none; } QListWidget::item:selected { background: transparent; outline: red; } QListWidget::item:focus { outline: none; }" + test);


    // QListWidget::item { margin-top: 20px;/* Spacing between items */}

    // pListContent->setStyleSheet("QListWidget { background: blue; padding-left: 0px; padding-right: 0px; padding-top: 0px; padding-bottom: 0px;} QListWidget::item { margin-bottom: 10px; /* Spacing between items */}" + sStyleScrollbarVertical1.arg(getColorTheme(1).name(), getColorTheme(1).name(), QString::number(10), QString::number(10/2)));

    for (auto i = 0; i < 2; i++)
    {
        if (i != 0)
        {
            QListWidgetItem* item2 = new QListWidgetItem(pListContent);
            item2->setSizeHint(QSize(pListContent->width(), 8));
            pListContent->addItem(item2);
        }

        auto menu = new FPanel(pListContent);
        QListWidgetItem* item = new QListWidgetItem(pListContent);
        QObject::connect(menu, &FPanel::heightChanged, [pListContent, item, menu]() {
            item->setSizeHint(menu->sizeHint());
            pListContent->update();  // Refresh layout
        });
        
        for (auto j = 0; j < 3; j++)
        {
            auto button = new FPanelSplitButton(m_iFillet, menu);
            button->setIcon(QIcon("D:\\VMShare\\Hornet\\src\\HornetMain\\res\\toolbar-button-cube-normal.png"));
            button->setText("Settings");
            button->setFixedSize(60, 65);

            for (auto k = 0; k < 3; k++)
            {
                QPixmap pixmap = QPixmap("D:/VMShare/Hornet/src/HornetMain/res/collapse.png");
                QTransform transform;
                transform.rotate(90);                                  // Rotate by 180 degrees
                QPixmap rotatedPixmap = pixmap.transformed(transform); // Apply transformation
                auto pExpandButton = new FExpandableButton("button", QIcon(""), QIcon(rotatedPixmap), button);
                pExpandButton->enableMoveAnimation(true);
                pExpandButton->setThemeStyle(ThemeStyle::Custom1);
                for (size_t z = 0; z < 4; z++)
                {
                    auto pArrowButton = new FArrowButton("button", QIcon(""), QIcon(""), pExpandButton);
                    pArrowButton->enableMoveAnimation(true);
                    pArrowButton->setThemeStyle(ThemeStyle::Custom1);
                    pExpandButton->addWidget(pArrowButton);
                }
                button->addWidget(pExpandButton);
            }
            
            menu->addWidget(button);
        }

        for (auto j = 0; j < 3; j++)
        {
            auto button = new FPanelButton(m_iFillet, menu);
            button->setIcon(QIcon("D:\\VMShare\\Hornet\\src\\HornetMain\\res\\toolbar-button-cube-normal.png"));
            button->setText("Settings");
            button->setFixedSize(60, 65);
            menu->addWidget(button);
        }

        pListContent->addItem(item);
        pListContent->setItemWidget(item, menu);
    }
    
#if 0
    // Add sample items
    for (int i = 0; i < 5; ++i)
    {
        auto pLabelTest = new QLabel(QString("Item %1").arg(i + 1), pListContent);
        pLabelTest->setStyleSheet("background: pink");
        QListWidgetItem* item = new QListWidgetItem(pListContent);
        // item->setSizeHint(pLabelTest->size());
        pListContent->addItem(item);
        pListContent->setItemWidget(item, pLabelTest);
    }

    auto testButton = new FArrowButton("Check Button", QIcon("D:\\VMShare\\Hornet\\src\\HornetMain\\res\\toolbar-button-cube-normal.png"), page);
    connect(testButton, &FArrowButton::clicked, this, []() { qDebug() << "Text/left side clicked";});
    // connect(testButton, &FArrowButton::arrowToggled, this, [](bool expanded) { qDebug() << "Arrow toggled: " << expanded; });

    auto button = new FPanelSplitButton(m_iFillet, page);
    button->setIcon(QIcon("D:\\VMShare\\Hornet\\src\\HornetMain\\res\\toolbar-button-cube-normal.png"));
    button->setText("Settings");

    auto menu = new FPanel(this);
#endif

    layout->addWidget(pLabel);
    layout->addWidget(pSeparator);
    layout->addWidget(pListContent);
#if 0
    layout->addWidget(testButton);
    layout->addWidget(button);
    layout->addWidget(menu);
    layout->addStretch();
#endif

    // page->setStyleSheet(QString("background-color: %2; border-radius: %1px;").arg(QString::number(*2), getColorTheme(0).name()));



    return page;
}
#endif

QWidget* FSideBarStackedWidget::addPageTest2(const QJsonArray& jsonArray, const QString& title)
{
    QWidget* page = new QWidget;
    QVBoxLayout* layout = new QVBoxLayout(page);
    layout->setSpacing(0);

    // Temporary
    auto pLabel = new QLabel(title);
    pLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    auto pSeparator = new FHorizontalSeparator(page);

    auto pListContent = new ScrollAwareListWidget(page);
    pListContent->setViewMode(QListView::ListMode);           // List mode
    pListContent->setFlow(QListView::TopToBottom);            // Flow left to right
    pListContent->setWrapping(false);                          // Wrap into rows
    pListContent->setResizeMode(QListView::Adjust);           // Adjust on resize
    pListContent->setWordWrap(true);
    pListContent->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
    pListContent->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    pListContent->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    pListContent->setFocusPolicy(Qt::NoFocus);

    int count = 0;
    for (const QJsonValue& arrayVal : jsonArray) 
    {
        if (!arrayVal.isObject()) 
            continue;

        QJsonObject jsonObj = arrayVal.toObject();
        if (!CHECK_JSON_TYPE_VALID(jsonObj, type) || jsonObj["type"].toString() != "Panel")
            continue;

        QString strTitle;
        QString strIcon;
        QString strIconArrow;
        bool bKeepIcon = false;

        GET_JSON_VALUE_STRING(jsonObj, strTitle, title)
        GET_JSON_VALUE_STRING(jsonObj, strIcon, icon)
        GET_JSON_VALUE_STRING(jsonObj, strIconArrow, iconArrow)
        GET_JSON_VALUE_BOOL(jsonObj, bKeepIcon, keepIcon)

        if (count != 0)
        {
            QListWidgetItem* item2 = new QListWidgetItem(pListContent);
            item2->setSizeHint(QSize(pListContent->width(), 8));
            pListContent->addItem(item2);
        }

        auto menu = new FPanel(pListContent);
        QListWidgetItem* item = new QListWidgetItem(pListContent);
        QObject::connect(menu, &FPanel::heightChanged, [pListContent, item, menu]() {
            item->setSizeHint(menu->sizeHint());
            pListContent->update();  // Refresh layout
        });

        if (CHECK_JSON_TYPE_VALID(jsonObj, childs))
        {
            QJsonArray panelArray = jsonObj["childs"].toArray();
            for (const QJsonValue& panelChild : panelArray) 
            {
                if (!panelChild.isObject()) 
                    continue;

                QJsonObject childObj = panelChild.toObject();
                if (!CHECK_JSON_TYPE_VALID(childObj, type))
                    continue;

                if (childObj["type"].toString() == "SplitButton")
                {
                    addSplitButton(childObj, menu);
                }
                else if (childObj["type"].toString() == "PanelButton")
                {
                    addPanelButton(childObj, menu);
                }
            }
        }

        pListContent->addItem(item);
        pListContent->setItemWidget(item, menu);
        count++;
    }

    layout->addWidget(pLabel);
    layout->addWidget(pSeparator);
    layout->addWidget(pListContent);

    return page;
}

void FSideBarStackedWidget::addSplitButton(const QJsonObject& obj, QWidget* parent)
{
    auto pMenu = qobject_cast<FPanel*>(parent);
    if (!pMenu)
        return;

    if (!CHECK_JSON_TYPE_VALID(obj, type) || obj["type"].toString() != "SplitButton")
        return;

    QString strTitle;
    QString strIcon;
    bool bKeepIcon = false;

    GET_JSON_VALUE_STRING(obj, strTitle, title)
    GET_JSON_VALUE_STRING(obj, strIcon, icon)
    GET_JSON_VALUE_BOOL(obj, bKeepIcon, keepIcon)

    auto pButton = new FPanelSplitButton(pMenu);
    pButton->setIcon(strIcon, bKeepIcon);
    pButton->setText(strTitle);
    pButton->setFixedSize(60, 65);

    if (CHECK_JSON_TYPE_VALID(obj, childs))
    {
        QJsonArray childArray = obj["childs"].toArray();
        for (const QJsonValue& childVal : childArray) 
        {
            if (!childVal.isObject()) 
                continue;

            QJsonObject childObj = childVal.toObject();
            if (!CHECK_JSON_TYPE_VALID(childObj, type))
                continue;

            if (childObj["type"].toString() == "ExpandableButton")
            {
                addExpandableButton(childObj, pButton);
            }
            else if (childObj["type"].toString() == "ArrowButton")
            {
                addArrowButton(childObj, pButton);
            }
        }
    }

    pMenu->addWidget(pButton);
}

void FSideBarStackedWidget::addPanelButton(const QJsonObject& obj, QWidget* parent)
{
    auto pMenu = qobject_cast<FPanel*>(parent);
    if (!pMenu)
        return;

    if (!CHECK_JSON_TYPE_VALID(obj, type) || obj["type"].toString() != "PanelButton")
        return;

    QString strTitle;
    QString strIcon;
    bool bKeepIcon = false;

    GET_JSON_VALUE_STRING(obj, strTitle, title)
    GET_JSON_VALUE_STRING(obj, strIcon, icon)
    GET_JSON_VALUE_BOOL(obj, bKeepIcon, keepIcon)

    auto pButton = new FPanelButton(pMenu);
    pButton->setIcon(strIcon, bKeepIcon);
    pButton->setText(strTitle);
    pButton->setFixedSize(60, 65);

    pMenu->addWidget(pButton);
}

void FSideBarStackedWidget::addExpandableButton(const QJsonObject &obj, QWidget *parent)
{
    if (!parent)
        return;

    if (!CHECK_JSON_TYPE_VALID(obj, type) || obj["type"].toString() != "ExpandableButton")
        return;

    QString strTitle;
    QString strIcon;
    QString strIconArrow;
    bool bKeepIcon = false;
    bool bMove = false;
    bool bRotate = false;

    GET_JSON_VALUE_STRING(obj, strTitle, title)
    GET_JSON_VALUE_STRING(obj, strIcon, icon)
    GET_JSON_VALUE_STRING(obj, strIconArrow, iconArrow)
    GET_JSON_VALUE_BOOL(obj, bKeepIcon, keepIcon)
    GET_JSON_VALUE_BOOL(obj, bMove, move)
    GET_JSON_VALUE_BOOL(obj, bRotate, rotate)

    auto pButton = new FExpandableButton(strTitle, strIcon, strIconArrow, parent);
    pButton->enableMoveAnimation(bMove);
    pButton->enableRotateAnimation(bRotate);
    pButton->setThemeStyle(ThemeStyle::Custom1);

    if (CHECK_JSON_TYPE_VALID(obj, childs))
    {
        QJsonArray childArray = obj["childs"].toArray();
        for (const QJsonValue& childVal : childArray) 
        {
            if (!childVal.isObject()) 
                continue;

            QJsonObject childObj = childVal.toObject();
            if (!CHECK_JSON_TYPE_VALID(childObj, type))
                continue;

            if (childObj["type"].toString() == "ExpandableButton")
            {
                addExpandableButton(childObj, pButton);
            }
            else if (childObj["type"].toString() == "ArrowButton")
            {
                addArrowButton(childObj, pButton);
            }
        }
    }

    if (FPanelSplitButton* pSplit = qobject_cast<FPanelSplitButton*>(parent))
    {
        pSplit->addWidget(pButton);
        return;
    }

    if (FExpandableButton* pExpandable = qobject_cast<FExpandableButton*>(parent))
    {
        pExpandable->addWidget(pButton);
        return;
    }
}

void FSideBarStackedWidget::addArrowButton(const QJsonObject &obj, QWidget *parent)
{
    if (!parent)
        return;

    if (!CHECK_JSON_TYPE_VALID(obj, type) || obj["type"].toString() != "ArrowButton")
        return;

    QString strTitle;
    QString strIcon;
    QString strIconArrow;
    bool bKeepIcon = false;
    bool bMove = false;
    bool bRotate = false;

    GET_JSON_VALUE_STRING(obj, strTitle, title)
    GET_JSON_VALUE_STRING(obj, strIcon, icon)
    GET_JSON_VALUE_STRING(obj, strIconArrow, iconArrow)
    GET_JSON_VALUE_BOOL(obj, bKeepIcon, keepIcon)
    GET_JSON_VALUE_BOOL(obj, bMove, move)
    GET_JSON_VALUE_BOOL(obj, bRotate, rotate)

    auto pButton = new FArrowButton(strTitle, strIcon, strIconArrow, bKeepIcon, parent);
    pButton->enableBold(false);
    pButton->enableMoveAnimation(bMove);
    pButton->enableRotateAnimation(bRotate);
    pButton->setThemeStyle(ThemeStyle::Custom1);

    if (FPanelSplitButton* pSplit = qobject_cast<FPanelSplitButton*>(parent))
    {
        pSplit->addWidget(pButton);
        return;
    }

    if (FExpandableButton* pExpandable = qobject_cast<FExpandableButton*>(parent))
    {
        pExpandable->addWidget(pButton);
        return;
    }
}