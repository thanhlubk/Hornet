#include <FancyWidgets/FPropertiesPalette.h>
#include <FancyWidgets/FUtil.h>
#include <FancyWidgets/FUtilTree.h>

#include <QtWidgets>
#include <QPainter>
#include <QPainterPath>
#include <QVariant>
#include <algorithm>

QWidget *FPropertiesPaletteItemDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &, const QModelIndex &index) const
{
    auto pWidget = index.data(Qt::UserRole).value<QWidget *>();
    if (!pWidget)
        return nullptr;

    pWidget->setParent(parent);
    return pWidget;
}

void FPropertiesPaletteItemDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    bool bIsTopLevelFirst = FUtilTree::isTopLevelFirst(index);
    bool bIsChild = FUtilTree::isChild(index);
    bool bIsParent = FUtilTree::isParent(index);
    bool bIsSiblingParent = FUtilTree::isSiblingParent(index);

    if (bIsChild && !bIsParent)
    {
        bool bIsNextChildParent = FUtilTree::isNextIndexHasChildren(index);
        bool bIsLastColumn = FUtilTree::isLastColumn(index);
        bool bIsLastChild = FUtilTree::isLastChildOfParent(index);
        bool bIsParentTopLevel = !index.parent().parent().isValid();
        bool bIsParentNextIndexHasChildren = FUtilTree::isNextIndexHasChildren(index.parent());

        auto rectUpdate = option.rect;
        if (!(bIsNextChildParent || (bIsLastChild && !bIsParentTopLevel && bIsParentNextIndexHasChildren)))
            rectUpdate.adjust(0, 0, 0, -m_iBorderWidth);

        if (bIsLastColumn)
            rectUpdate.adjust(0, 0, -m_iBorderWidth, 0);

        if (index.column() == 0)
        {
            int iIndentation = FUtilTree::getIndentationFromStyleOption(option);
            editor->setGeometry(rectUpdate.adjusted(-iIndentation + m_iBorderWidth, 0, 0, 0));
        }
        else
            editor->setGeometry(rectUpdate.adjusted(m_iBorderWidth, 0, 0, 0));
    }
    else if (bIsParent || bIsSiblingParent)
    {
        auto rectUpdate = option.rect;
        rectUpdate.adjust(m_iBorderWidth, 0, 0, 0); // Adjust for border width

        if (!bIsTopLevelFirst && !bIsChild)
            editor->setGeometry(rectUpdate.adjusted(0, m_iSpacing, 0, 0));
        else
            editor->setGeometry(rectUpdate);
    }
    else
        editor->setGeometry(option.rect); // default placement
}

QSize FPropertiesPaletteItemDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QSize sizeBase = QStyledItemDelegate::sizeHint(option, index);

    int iMaxHeight = -1;
    const QAbstractItemModel *pModel = index.model();
    if (pModel)
    {
        for (int i = 0; i < pModel->columnCount(); ++i)
        {
            auto currentIndex = index.sibling(index.row(), i);
            if (!currentIndex.isValid())
                continue;

            QWidget *pWidget = index.data(Qt::UserRole).value<QWidget *>();
            if (!pWidget)
                continue;

            int iCurrentHeight = std::max({sizeBase.height(), pWidget->height(), pWidget->sizeHint().height(), pWidget->minimumSizeHint().height()});
            if (iCurrentHeight > iMaxHeight)
                iMaxHeight = iCurrentHeight;
        }
    }

    if (iMaxHeight > 0)
        sizeBase.setHeight(iMaxHeight);

    // Skip padding for the first top-level item
    bool bIsTopLevelFirst = FUtilTree::isTopLevelFirst(index);
    bool bIsChild = FUtilTree::isChild(index);
    bool bIsParent = FUtilTree::isParent(index);
    bool bIsSiblingParent = FUtilTree::isSiblingParent(index);

    if ((bIsParent || bIsSiblingParent) && !bIsTopLevelFirst && !bIsChild)
    {
        // return baseSize;
        return QSize(sizeBase.width(), sizeBase.height() + m_iSpacing);
    }
    else if (!bIsParent && bIsChild)
    {
        bool bIsNextChildParent = FUtilTree::isNextIndexHasChildren(index);
        bool bIsLastChild = FUtilTree::isLastChildOfParent(index);
        bool bIsParentTopLevel = !index.parent().parent().isValid();
        bool bIsParentNextIndexHasChildren = FUtilTree::isNextIndexHasChildren(index.parent());

        int iAdjustHeight = bIsNextChildParent || (bIsLastChild && !bIsParentTopLevel && bIsParentNextIndexHasChildren) ? sizeBase.height() : sizeBase.height() + m_iBorderWidth;
        if (index.column() == 0)
        {
            int iIndentation = FUtilTree::getIndentationFromStyleOption(option);
            return QSize(sizeBase.width() + iIndentation, iAdjustHeight);
        }
        else
            return QSize(sizeBase.width(), iAdjustHeight);
    }

    return sizeBase;
}

void FPropertiesPaletteItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyleOptionViewItem opt(option);
    initStyleOption(&opt, index);

    bool bIsTopLevelFirst = FUtilTree::isTopLevelFirst(index);
    bool bIsChild = FUtilTree::isChild(index);
    bool bIsParent = FUtilTree::isParent(index);
    bool bIsSiblingParent = FUtilTree::isSiblingParent(index);

    if (bIsParent || bIsSiblingParent)
    {
        QRect rectDraw = option.rect;

        if (!bIsTopLevelFirst && !bIsChild)
            rectDraw.adjust(0, m_iSpacing, 0, 0); // Adjust height to account for spacing

        int iTopLeftRadius = 0;
        int iTopRightRadius = 0;
        int iBottomRightRadius = 0;
        int iBottomLeftRadius = 0;

        const QTreeView *pTreeView = qobject_cast<const QTreeView *>(option.widget);
        if (bIsChild && FUtilTree::isLastColumn(index) && FUtilTree::isLastChildOfParent(index) && !FUtilTree::isExpandedInView(pTreeView, index))
            iBottomRightRadius = m_iRadius;

        if (!bIsChild && FUtilTree::isLastColumn(index))
        {
            iTopRightRadius = m_iRadius;
            if (!FUtilTree::isExpandedInView(pTreeView, index))
                iBottomRightRadius = m_iRadius;
        }

        if (iTopLeftRadius == 0 && iTopRightRadius == 0 && iBottomRightRadius == 0 && iBottomLeftRadius == 0)
            painter->fillRect(rectDraw, m_colorParent);
        else
            FUtil::fillRoundedRect(painter, rectDraw, m_colorParent, iTopLeftRadius, iTopRightRadius, iBottomRightRadius, iBottomLeftRadius);

        if (!bIsTopLevelFirst && !bIsChild)
            opt.rect.adjust(0, m_iSpacing, 0, 0);
    }
    else if (!bIsParent && bIsChild)
    {
        bool bIsNextChildParent = FUtilTree::isNextIndexHasChildren(index);
        bool bIsLastChild = FUtilTree::isLastChildOfParent(index);
        bool bIsPreviousIndexHasChildren = FUtilTree::isPreviousIndexHasChildren(index);
        bool bIsLastColumn = FUtilTree::isLastColumn(index);

        int iBottomLeftRadius = bIsLastChild && index.column() == 0 && FUtilTree::isParentLastChildOfItsParent(index) ? m_iRadius : 0;
        int iBottomRightRadius = bIsLastChild && bIsLastColumn && FUtilTree::isParentLastChildOfItsParent(index) ? m_iRadius : 0;

        bool bIsPreviousExpand = false;
        const QTreeView *treeView = qobject_cast<const QTreeView *>(option.widget);
        if (treeView && bIsPreviousIndexHasChildren)
            bIsPreviousExpand = FUtilTree::isPreviousIndexExpandedInView(treeView, index);

        int iTopRadius = bIsPreviousExpand ? m_iRadius : 0;
        if (index.column() == 0)
        {
            int iIndentation = FUtilTree::getIndentationFromStyleOption(option);
            opt.rect.adjust(-iIndentation, 0, 0, 0); // Left padding for child items
        }

        QRect rectDraw = opt.rect;
        rectDraw.adjust(0, -m_iBorderWidth, 0, 0);

        bool bIsParentTopLevel = !index.parent().parent().isValid();
        bool bIsParentNextIndexHasChildren = FUtilTree::isNextIndexHasChildren(index.parent());

        if (bIsNextChildParent || (bIsLastChild && !bIsParentTopLevel && bIsParentNextIndexHasChildren))
            rectDraw.adjust(0, 0, 0, m_iBorderWidth);

        if (!bIsLastColumn)
            rectDraw.adjust(0, 0, m_iBorderWidth, 0);

        FUtil::drawRoundedRect(painter, rectDraw, m_iBorderWidth, m_colorParent, iTopRadius, 0, iBottomRightRadius, iBottomLeftRadius); // No rounded corners for child items
    }

    QStyledItemDelegate::paint(painter, opt, index);
}

void FPropertiesPaletteItemDelegate::setSpacing(int spacing)
{
    m_iSpacing = spacing;
}

void FPropertiesPaletteItemDelegate::setColorParent(const QColor &color)
{
    m_colorParent = color;
}

void FPropertiesPaletteItemDelegate::setBorderWidth(int width)
{
    m_iBorderWidth = width / 2;
}

void FPropertiesPaletteItemDelegate::setRadius(int radius)
{
    m_iRadius = radius;
}

void FPropertiesPaletteProxyStyle::drawPrimitive(PrimitiveElement element, const QStyleOption *option, QPainter *painter, const QWidget *widget) const
{
    if (element == PE_IndicatorBranch)
    {
        const QTreeView *pTreeView = qobject_cast<const QTreeView *>(widget);
        if (!pTreeView || !option)
        {
            QProxyStyle::drawPrimitive(element, option, painter, widget);
            return;
        }

        QPoint pointCenter = option->rect.center();
        QModelIndex index = pTreeView->indexAt(pointCenter);
        auto rect = pTreeView->visualRect(index);

        int iHalfIndent = pTreeView->indentation() / 2 + 2; // Adjust for half indentation and some padding
        QPoint pointOffsetCenter = pointCenter + QPoint(iHalfIndent, 0);

        if (!index.isValid() || !rect.contains(pointOffsetCenter))
        {
            // Disable drawing oultine for branch icon
            painter->fillRect(option->rect, m_colorBackground);
            return;
        }

        bool bIsTopLevelFirst = (index.row() == 0 && !index.parent().isValid());

        QAbstractItemModel *pModel = pTreeView->model();
        bool bIsParent = pModel->hasChildren(index);
        bool bIsExpanded = pTreeView->isExpanded(index);
        bool bIsChild = index.parent().isValid();

        QPixmap pixmapIcon;
        QString strIcon;
        int size;
        if (bIsParent)
        {
            size = m_iIconSize;
            if (bIsExpanded)
                strIcon = m_strIconExpand;
            else
                strIcon = m_strIconCollapse;
        }

        auto colorIcon = FUtil::changeIconColor(strIcon, m_colorIcon);
        pixmapIcon = FUtil::getPixmapFromIcon(colorIcon);

        if (!pixmapIcon.isNull())
        {
            QRect rectDraw = option->rect;
            if (!bIsTopLevelFirst && !bIsChild)
                rectDraw.adjust(0, m_iSpacing, 0, 0); // Adjust height to account for spacing

            int iTopLeftRadius = 0;
            int iBottomLeftRadius = 0;
            if (!bIsChild)
            {
                iTopLeftRadius = m_iRadius;
                iBottomLeftRadius = m_iRadius;
            }
            else
            {
                if (FUtilTree::isLastChildOfParent(index) || FUtilTree::isExpandedInView(pTreeView, index))
                    iBottomLeftRadius = m_iRadius;

                if (FUtilTree::isPreviousIndexExpandedInView(pTreeView, index))
                    iTopLeftRadius = m_iRadius;
            }

            FUtil::fillRoundedRect(painter, rectDraw, m_colorParent, iTopLeftRadius, 0, 0, iBottomLeftRadius);
            QSize desiredSize(size, size); // Customize size here
            pixmapIcon = pixmapIcon.scaled(desiredSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);

            QPoint pointDrawPos = pointCenter - QPoint(pixmapIcon.width() / 2, pixmapIcon.height() / 2);
            if (!bIsTopLevelFirst && !bIsChild)
                pointDrawPos += QPoint(0, m_iSpacing / 2);
            painter->drawPixmap(pointDrawPos, pixmapIcon);
        }
        return;
    }

    QProxyStyle::drawPrimitive(element, option, painter, widget);
}

void FPropertiesPaletteProxyStyle::setIcons(const QString &expand, const QString &collapse)
{
    m_strIconExpand = expand;
    m_strIconCollapse = collapse;
};

void FPropertiesPaletteProxyStyle::setIconColor(const QColor &color)
{
    m_colorIcon = color;
};

void FPropertiesPaletteProxyStyle::setIconSize(int size)
{
    m_iIconSize = size;
};

void FPropertiesPaletteProxyStyle::setSpacing(int spacing)
{
    m_iSpacing = spacing;
};

void FPropertiesPaletteProxyStyle::setColorBackground(const QColor &color)
{
    m_colorBackground = color;
};

void FPropertiesPaletteProxyStyle::setRadius(int radius)
{
    m_iRadius = radius;
};

void FPropertiesPaletteProxyStyle::setParentColor(const QColor &color)
{
    m_colorParent = color;
};

QString FPropertiesPaletteProxyStyle::iconExpand() const
{
    return m_strIconExpand;
};

QString FPropertiesPaletteProxyStyle::iconCollapse() const
{
    return m_strIconCollapse;
};

QColor FPropertiesPaletteProxyStyle::getColorIcon() const
{
    return m_colorIcon;
};

FPropertiesPalette::FPropertiesPalette(QWidget *parent)
    : QTreeWidget(parent)
{
    setColumnCount(2);
    setAlternatingRowColors(false);
    setSelectionBehavior(QAbstractItemView::SelectItems);
    setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    setHeaderHidden(false);
    setAnimated(false);
    setAllColumnsShowFocus(true);
    setFocusPolicy(Qt::NoFocus);
    setRootIsDecorated(true); // keep expand/collapse icon

    header()->setDefaultAlignment(Qt::AlignCenter);
    SET_UP_THEME(FPropertiesPalette)
}

void FPropertiesPalette::applyTheme()
{
    m_iSpacing = getDisplaySize(1).toInt();
    setIndentation(getDisplaySize(4).toInt() + getDisplaySize(1).toInt());

    FUtil::setFontWidget(this, getDisplaySize(4).toInt(), false);
    FUtil::setFontWidget(header(), getDisplaySize(4).toInt(), true);

    auto pStyle = new FPropertiesPaletteProxyStyle();
    pStyle->setIcons(":/FancyWidgets/res/icon/small/forward.png", ":/FancyWidgets/res/icon/small/downward.png");
    pStyle->setIconColor(getColorTheme(0));
    pStyle->setIconSize(getDisplaySize(0).toInt());
    pStyle->setColorBackground(getColorTheme(1));
    pStyle->setRadius(getDisplaySize(5).toInt());
    pStyle->setParentColor(getColorTheme(2));

    setStyle(pStyle);
    m_pProxyStyle = pStyle;

    auto pDelegate = new FPropertiesPaletteItemDelegate();
    pDelegate->setColorParent(getColorTheme(2));
    pDelegate->setBorderWidth(getDisplaySize(2).toInt()*2);
    pDelegate->setRadius(getDisplaySize(5).toInt());
    setItemDelegate(pDelegate);

    auto strScrollbarVerticalStyle = FUtil::getStyleScrollbarVertical2(getDisplaySize(3).toInt(), getDisplaySize(1).toInt() / 2, getDisplaySize(1).toInt() / 2, 0, ":/FancyWidgets/res/icon/small/up.png", ":/FancyWidgets/res/icon/small/down.png", getColorTheme(3).name(), getColorTheme(3).name());

    auto strScrollbarHorizontalStyle = FUtil::getStyleScrollbarHorizontal1(getDisplaySize(3).toInt(), 0, 0, 0, getColorTheme(3).name(), getColorTheme(3).name());

    setStyleSheet(QString("QTreeWidget { background-color: %1; show-decoration-selected: 0; border: none; } QHeaderView::section { background-color: %1; color: %3; border: none; } QTreeWidget::item { background-color: transparent; color: %2; } QTreeWidget::item:has-children { background-color: transparent; border: none; }").arg(getColorTheme(1).name(), getColorTheme(3).name(), getColorTheme(4).name()) + strScrollbarVerticalStyle + strScrollbarHorizontalStyle);

    updateSpacing();
    updateStyleLabel();
}

QColor FPropertiesPalette::getColorTheme(int idx)
{
    SET_UP_COLOR(0, Default, dominant);
    SET_UP_COLOR(1, Default, backgroundSection1);
    SET_UP_COLOR(2, Default, widgetHover);
    SET_UP_COLOR(3, Default, textNormal);
    SET_UP_COLOR(4, Default, textHeader2);

    return QColor(qRgb(0, 0, 0));
}

QVariant FPropertiesPalette::getDisplaySize(int idx)
{
    SET_UP_DISPLAY_SIZE(0, Primary, iconSize3);
    SET_UP_DISPLAY_SIZE(1, Primary, offsetSize2);
    SET_UP_DISPLAY_SIZE(2, Primary, separatorWidth);
    SET_UP_DISPLAY_SIZE(3, Primary, scrollbarWidth);
    SET_UP_DISPLAY_SIZE(4, Primary, fontNormalSize);
    SET_UP_DISPLAY_SIZE(5, Primary, filletSize1);

    return QVariant(0);
}

QTreeWidgetItem *FPropertiesPalette::addItem(QList<QWidget *> listWidget, QTreeWidgetItem *parent)
{
    bool bIsFirstItem = topLevelItemCount() == 0 ? true : false;

    QTreeWidgetItem *pItem = nullptr;
    if (parent == nullptr)
        pItem = new QTreeWidgetItem(this);
    else
        pItem = new QTreeWidgetItem(parent);

    pItem->setFlags(pItem->flags() | Qt::ItemIsEditable);
    for (int i = 0; i < listWidget.size(); ++i)
    {
        if (i >= columnCount())
            break; // Avoid out-of-bounds access

        if (listWidget[i] == nullptr)
            continue; // Skip null widgets

        pItem->setData(i, Qt::UserRole, QVariant::fromValue(listWidget[i]));
        openPersistentEditor(pItem, i);
    }

    return pItem;
}

QTreeWidgetItem *FPropertiesPalette::addItem(const QString &name, QList<QString> listValue, QTreeWidgetItem *parent)
{
    QList<QWidget *> listValidWidget;

    QLabel *pLabelName = new QLabel(name, this);
    m_listLabel.append(pLabelName);
    listValidWidget.append(pLabelName);
    for (int i = 0; i < listValue.size(); ++i)
    {
        if (i >= columnCount() - 1)
            break; // Avoid out-of-bounds access

        QLabel *pLabelValue = new QLabel(listValue[i], this);
        m_listLabel.append(pLabelValue);
        listValidWidget.append(pLabelValue);
    }

    updateStyleLabel();
    return addItem(listValidWidget, parent);
}

QTreeWidgetItem *FPropertiesPalette::addItem(const QString &name, QList<QWidget *> listWidget, QTreeWidgetItem *parent)
{
    QList<QWidget *> listValidWidget;

    QLabel *pLabelName = new QLabel(name, this);
    m_listLabel.append(pLabelName);
    listValidWidget.append(pLabelName);

    for (int i = 0; i < listWidget.size(); ++i)
    {
        if (i >= columnCount() - 1)
            break; // Avoid out-of-bounds access

        if (listWidget[i] == nullptr)
            continue; // Skip null widgets

        listValidWidget.append(listWidget[i]);
    }

    updateStyleLabel();
    return addItem(listValidWidget, parent);
}

void FPropertiesPalette::setSpacing(int spacing)
{
    m_iSpacing = spacing;
    updateSpacing();
}

void FPropertiesPalette::setColumnCount(int columns)
{
    if (columns < 2)
        columns = 2; // Ensure at least two column
    QTreeWidget::setColumnCount(columns);
}

void FPropertiesPalette::updateSpacing()
{
    auto pStyle = qobject_cast<FPropertiesPaletteProxyStyle *>(m_pProxyStyle);
    if (pStyle)
        pStyle->setSpacing(m_iSpacing);

    auto pDelegate = qobject_cast<FPropertiesPaletteItemDelegate *>(itemDelegate());
    if (pDelegate)
        pDelegate->setSpacing(m_iSpacing);
}

void FPropertiesPalette::updateStyleLabel()
{
    for (auto &pLabel : m_listLabel)
    {
        if (pLabel)
        {
            FUtil::setFontWidget(pLabel, getDisplaySize(4).toInt(), false);
            pLabel->setFixedHeight(24);
            pLabel->setStyleSheet(QString("QLabel { background: transparent; color: %1; }").arg(getColorTheme(3).name()));
        }
    }
}