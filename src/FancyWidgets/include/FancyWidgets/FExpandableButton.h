#pragma once
#include "FancyWidgetsExport.h"
#include "FArrowButton.h"
#include "FPopupMenu.h"

class FANCYWIDGETS_EXPORT FExpandableButton : public FArrowButton
{
    Q_OBJECT

public:
    explicit FExpandableButton(const QString& text, const QString& leftIcon, const QString& arrowIcon, bool keep, QWidget* parent = nullptr);

    void addWidget(QWidget* widget);

signals:
    void customSignal();

private slots:
    void customSlot();

private:
    FPopupMenu* m_pMenu;
};