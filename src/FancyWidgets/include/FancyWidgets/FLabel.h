#pragma once
#include "FancyWidgetsExport.h"
#include <QLabel>
#include "FThemeableWidget.h"

class FANCYWIDGETS_EXPORT FLabel : public QLabel, public FThemeableWidget
{
    Q_OBJECT
    DECLARE_THEME

public:
    explicit FLabel(QWidget *parent = nullptr);
    explicit FLabel(const QString &text, QWidget *parent = nullptr);
};
