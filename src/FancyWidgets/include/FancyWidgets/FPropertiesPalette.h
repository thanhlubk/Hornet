#pragma once
#include "FancyWidgetsExport.h"
#include "FThemeableWidget.h"

#include <QAbstractItemModel>
#include <QModelIndex>
#include <QTreeWidget>
#include <QStyledItemDelegate>
#include <QProxyStyle>
#include <QList>

class FPropertiesPaletteItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    using QStyledItemDelegate::QStyledItemDelegate;

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &, const QModelIndex &index) const override;
    void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;

    void setSpacing(int spacing);
    void setColorParent(const QColor &color);
    void setBorderWidth(int width);
    void setRadius(int radius);

private:
    QColor m_colorParent;
    int m_iSpacing; // Spacing between items
    int m_iBorderWidth;
    int m_iRadius;
};

class FPropertiesPaletteProxyStyle : public QProxyStyle
{
    Q_OBJECT

public:
    using QProxyStyle::QProxyStyle;

    void drawPrimitive(PrimitiveElement element, const QStyleOption *option, QPainter *painter, const QWidget *widget = nullptr) const override;

    void setIcons(const QString &expand, const QString &collapse);
    void setIconColor(const QColor &color);
    void setIconSize(int size);
    void setSpacing(int spacing);
    void setColorBackground(const QColor &color);
    void setRadius(int radius);
    void setParentColor(const QColor &color);
    QString iconExpand() const;
    QString iconCollapse() const;
    QColor getColorIcon() const;

private:
    QString m_strIconExpand;
    QString m_strIconCollapse;
    QColor m_colorIcon; // Default color for branch lines
    QColor m_colorBackground;
    QColor m_colorParent;
    int m_iIconSize; // Default icon size
    int m_iSpacing; // Spacing between items
    int m_iRadius; // Radius for rounded corners of the border
};

class FANCYWIDGETS_EXPORT FPropertiesPalette : public QTreeWidget, public FThemeableWidget
{
    Q_OBJECT
    DECLARE_THEME

public:
    explicit FPropertiesPalette(QWidget *parent = nullptr);

    QTreeWidgetItem *addItem(QList<QWidget*> listWidget, QTreeWidgetItem *parent = nullptr);
    QTreeWidgetItem *addItem(const QString &name, QList<QString> listValue, QTreeWidgetItem *parent = nullptr);
    QTreeWidgetItem *addItem(const QString &name, QList<QWidget *> listWidget, QTreeWidgetItem *parent = nullptr);

    void setSpacing(int spacing);
    void setColumnCount(int columns);

private:
    void updateSpacing();
    void updateStyleLabel();

    int m_iSpacing; // Spacing between items
    QProxyStyle *m_pProxyStyle;
    QList<QWidget*> m_listLabel; // List of widgets to be added to the tree items
};