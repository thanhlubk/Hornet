#pragma once
#include "FancyWidgetsExport.h"
#include <QWidget>
#include <QPushButton>
#include <QLabel>
#include <QHBoxLayout>
#include <QMouseEvent>
#include "FThemeableWidget.h"
#include "FThemeManager.h"
#include "FSearchBar.h"

class FANCYWIDGETS_EXPORT FTitleBar : public QWidget, public FThemeableWidget
{
    Q_OBJECT
    DECLARE_THEME

public:
    explicit FTitleBar(QWidget* parent = nullptr);
    void setAppTitleIcon(const QString &text, const QString &icon);
    void setAppIcon(const QString &icon);
    void setAppTitle(const QString &text);

signals:
    void signalClose();
    void signalMinimize();
    void signalToggleMaximize();

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void paintEvent(QPaintEvent* event) override;

private:
    void updateIcon();

    QPushButton* m_pButtonMinimize;
    QPushButton* m_pButtonMaximize;
    QPushButton* m_pButtonClose;
    QLabel* m_pLabelAppIcon;
    QLabel* m_pLabelAppTitle;
    FSearchBar* m_pSearchBar;

    QString m_strIconApp;
    QPoint m_pointDragStartPos;
};
