#pragma once
#include "FancyWidgetsExport.h"
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QWidget>
#include "FThemeableWidget.h"
#include "FTreeItem.h"
#include <QProxyStyle>
#include <QPainter>
#include <QStyleOption>
#include <QAbstractItemModel>
#include <QStyledItemDelegate>

class FANCYWIDGETS_EXPORT FTreeWidget : public QTreeWidget, public FThemeableWidget
{
    Q_OBJECT
    DECLARE_THEME

public:
    explicit FTreeWidget(QWidget *parent = nullptr);
    explicit FTreeWidget(int columnCount, QWidget *parent = nullptr);

    QTreeWidgetItem *addItem(QTreeWidgetItem *parent, QList<QWidget*> listWidget);

private:
    int calculateItemHeightRecursive(QTreeWidgetItem *item);

private slots:
    void adjustHeightToContents();
};

class FProxyBranchStyle : public QProxyStyle
{
public:
    using QProxyStyle::QProxyStyle;

    void drawPrimitive(PrimitiveElement element, const QStyleOption *option, QPainter *painter, const QWidget *widget = nullptr) const override;

    void setIcons(const QString &expand, const QString &collapse, const QString &leaf);
    void setIconColor(const QColor &color);
    void setIconSize(int size);

    QString iconExpand() const;
    QString iconCollapse() const;
    QString iconLeaf() const;
    QColor getColorIcon() const;

private:
    QString m_strIconExpand;
    QString m_strIconCollapse;
    QString m_strIconLeaf;
    QColor m_colorIcon; // Default color for branch lines
    int m_iIconSize; // Default icon size
};
