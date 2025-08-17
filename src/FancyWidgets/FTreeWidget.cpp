#include <FancyWidgets/FTreeWidget.h>
#include <QMouseEvent>
#include <FancyWidgets/FUtil.h>
#include <QHeaderView>

FTreeWidget::FTreeWidget(QWidget *parent)
    : FTreeWidget(1, parent)
{
}

FTreeWidget::FTreeWidget(int columnCount, QWidget *parent)
    : QTreeWidget(parent)
{
    setColumnCount(columnCount);
    setHeaderHidden(true);
    setFocusPolicy(Qt::NoFocus);
    adjustHeightToContents();

    connect(this, &QTreeWidget::expanded, this, &FTreeWidget::adjustHeightToContents);
    connect(this, &QTreeWidget::collapsed, this, &FTreeWidget::adjustHeightToContents);

    SET_UP_THEME(FTreeWidget)
}

void FTreeWidget::applyTheme()
{
    // Apply theme styles here if needed
    setIndentation(getDisplaySize(0).toInt());
    auto style = new FProxyBranchStyle();
    style->setIcons(":/FancyWidgets/res/icon/small/plus.png", ":/FancyWidgets/res/icon/small/minus.png", ":/FancyWidgets/res/icon/small/round.png");
    style->setIconColor(getColorTheme(0));
    style->setIconSize(getDisplaySize(0).toInt());
    setStyle(style);
    setStyleSheet("QTreeWidget::item { margin-top: 0px; margin-bottom: 0px;}");
}

QColor FTreeWidget::getColorTheme(int idx)
{
    SET_UP_COLOR(0, Default, dominant);

    return QColor(qRgb(0, 0, 0));
}

QVariant FTreeWidget::getDisplaySize(int idx)
{
    SET_UP_DISPLAY_SIZE(0, Primary, iconSize3);

    return QVariant(0);
}

QTreeWidgetItem *FTreeWidget::addItem(QTreeWidgetItem *parent, QList<QWidget*> listWidget)
{
    QTreeWidgetItem *item = nullptr;
    if (parent == nullptr)
        item = new QTreeWidgetItem(this);
    else
        item = new QTreeWidgetItem(parent);

    for (int i = 0; i < listWidget.size(); ++i)
    {
        if (i >= columnCount())
            break; // Avoid out-of-bounds access

        if (listWidget[i] == nullptr)
            continue; // Skip null widgets

        setItemWidget(item, i, listWidget[i]);
    }

    adjustHeightToContents();
    return item;
}

void FTreeWidget::adjustHeightToContents()
{
    int totalHeight = 0;

    // Loop through all visible items and sum their heights
    for (int row = 0; row < topLevelItemCount(); ++row)
    {
        QTreeWidgetItem *item = topLevelItem(row);
        totalHeight += calculateItemHeightRecursive(item);
    }

    int headerHeight = header()->isHidden() ? 0 : header()->height();
    int margin = 4; // Optional: add padding
    setFixedHeight(headerHeight + totalHeight + margin);
}

int FTreeWidget::calculateItemHeightRecursive(QTreeWidgetItem *item)
{
    int height = rowHeight(indexFromItem(item));

    if (item->isExpanded())
    {
        for (int i = 0; i < item->childCount(); ++i)
        {
            height += calculateItemHeightRecursive(item->child(i));
        }
    }
    return height;
}

void FProxyBranchStyle::drawPrimitive(PrimitiveElement element, const QStyleOption *option, QPainter *painter, const QWidget *widget) const
{
    if (element == PE_IndicatorBranch)
    {
        const QTreeView *treeView = qobject_cast<const QTreeView *>(widget);
        if (!treeView || !option)
        {
            // QProxyStyle::drawPrimitive(element, option, painter, widget);
            return;
        }

        QPoint center = option->rect.center();
        QModelIndex index = treeView->indexAt(center);
        QPoint offsetcenter = center + QPoint(12, 0);
        auto rect = treeView->visualRect(index);
        if (!index.isValid() || !rect.contains(offsetcenter))
        {
            // QProxyStyle::drawPrimitive(element, option, painter, widget);
            return;
        }

        QAbstractItemModel *model = treeView->model();
        bool hasChildren = model->hasChildren(index);
        bool isExpanded = treeView->isExpanded(index);

        // bool hasSibling = model->rowCount(index.parent()) > 1;
        // bool finalChild = index.row() == model->rowCount(index.parent()) - 1;
        // bool bAdjoins = index.row() < model->rowCount(index.parent()) - 1;
        // bool isLeaf = index.column() == 0 && !model->hasChildren(index);
        
        QPixmap icon;
        QString strIcon;
        int size;
        if (hasChildren)
        {
            size = m_iIconSize;
            if (isExpanded)
                strIcon = m_strIconExpand;
            else
                strIcon = m_strIconCollapse;
        }
        else
        {
            strIcon = m_strIconLeaf;
            size = m_iIconSize / 3;
        }

        auto colorIcon = FUtil::changeIconColor(strIcon, m_colorIcon);
        icon = FUtil::getPixmapFromIcon(colorIcon);

        if (!icon.isNull())
        {
            QSize desiredSize(size, size); // Customize size here
            icon = icon.scaled(desiredSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);

            QPoint drawPos = center - QPoint(icon.width() / 2, icon.height() / 2);
            painter->drawPixmap(drawPos, icon);
        }
        return;
    }

    QProxyStyle::drawPrimitive(element, option, painter, widget);
}

void FProxyBranchStyle::setIcons(const QString &expand, const QString &collapse, const QString &leaf)
{
    m_strIconExpand = expand;
    m_strIconCollapse = collapse;
    m_strIconLeaf = leaf;
}

void FProxyBranchStyle::setIconColor(const QColor &color)
{
    m_colorIcon = color;
}

void FProxyBranchStyle::setIconSize(int size)
{
    m_iIconSize = size;
}

QString FProxyBranchStyle::iconExpand() const
{
    return m_strIconExpand;
}

QString FProxyBranchStyle::iconCollapse() const
{
    return m_strIconCollapse;
}

QString FProxyBranchStyle::iconLeaf() const
{
    return m_strIconLeaf;
}

QColor FProxyBranchStyle::getColorIcon() const
{
    return m_colorIcon;
}