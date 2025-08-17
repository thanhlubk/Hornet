#pragma once
#include "FancyWidgetsExport.h"
#include <QCheckBox>
#include "FThemeableWidget.h"

class FANCYWIDGETS_EXPORT FCheckBox : public QCheckBox, public FThemeableWidget
{
    Q_OBJECT
    DECLARE_THEME
public:
    explicit FCheckBox(QWidget *parent = nullptr);
    explicit FCheckBox(const QString &text, QWidget *parent = nullptr);
    
    void setText(const QString &text);
};
