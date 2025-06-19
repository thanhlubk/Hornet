#pragma once
#include "FancyWidgetsExport.h"
#include <QWidget>
#include <QPushButton>
#include <QLabel>
#include <QHBoxLayout>
#include <QMouseEvent>

class FANCYWIDGETS_EXPORT FTitleBar : public QWidget {
    Q_OBJECT

public:
    explicit FTitleBar(QWidget* parent = nullptr);

signals:
    void signalClose();
    void signalMinimize();
    void signalToggleMaximize();

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;

private:
    QPoint dragStartPos;
};
