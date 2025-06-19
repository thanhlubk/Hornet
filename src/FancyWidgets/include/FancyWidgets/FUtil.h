#pragma once
#include "FancyWidgetsExport.h"
#include <QIcon>
#include <QAction>
#include <QPainter>
#include <QPixmap>
#include <QWidget>
#include <QString>

namespace FUtil
{
    // Custom icon
    FANCYWIDGETS_EXPORT QString getTempPath();
    FANCYWIDGETS_EXPORT QString getFileName();

    FANCYWIDGETS_EXPORT QSize getIconLargestSize(const QIcon& icon);

    FANCYWIDGETS_EXPORT QPixmap changePixmapColor(const QPixmap& pixmap, QColor color);

    FANCYWIDGETS_EXPORT QIcon changeIconColor(const QIcon& icon, QColor color);
    FANCYWIDGETS_EXPORT QIcon changeIconColor(const QString& strPath, QColor color);
    FANCYWIDGETS_EXPORT QIcon changeIconColor(const QPixmap& pxIcon, QColor color);

    FANCYWIDGETS_EXPORT QIcon rotateIcon(const QIcon& icon, double degree);

    FANCYWIDGETS_EXPORT QString getIconColorPath(const QString& strPath, QColor color);
    
    FANCYWIDGETS_EXPORT void setFontWidget(QWidget* pWidget, int iSize, bool bBold);
    
    FANCYWIDGETS_EXPORT QString toCamelCase(const QString& str);

    FANCYWIDGETS_EXPORT QString getStyleScrollbarVertical2(int width, int space, int paddingLeft, int paddingRight, const QString& arrowUpPath, const QString& arrowDownPath, const QString& colorBar, const QString& colorHover);
}