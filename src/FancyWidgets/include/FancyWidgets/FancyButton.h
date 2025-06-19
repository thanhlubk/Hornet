#pragma once

#include "FancyWidgetsExport.h"
#include <QPushButton>

class FANCYWIDGETS_EXPORT FancyButton : public QPushButton {
    Q_OBJECT
public:
    explicit FancyButton(QWidget* parent = nullptr);
    explicit FancyButton(const QString& text, QWidget* parent = nullptr);
};
