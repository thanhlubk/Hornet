#pragma once
#include "FancyWidgetsExport.h"
#include "FThemeableWidget.h"
#include <QTableWidget>
#include <QWidget>
#include <QList>

class FANCYWIDGETS_EXPORT FTable : public QTableWidget, public FThemeableWidget
{
    Q_OBJECT
    DECLARE_THEME

public:
    explicit FTable(QWidget *parent = nullptr);

    void addRow(const QList<QWidget*> &listRowWigets);
    void deleteRow(const int rowIndex);
    void enableFitHeight(bool enable);
    bool isFitHeight() const;
    void adjustHeightToContents();

private:

    bool m_bFitHeight = true;

};