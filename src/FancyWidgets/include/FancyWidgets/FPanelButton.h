#pragma once
#include "FancyWidgetsExport.h"
#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>
#include "FThemeableWidget.h"
#include "FThemeManager.h"

class FANCYWIDGETS_EXPORT FPanelButton : public QWidget, public FThemeableWidget 
{
    Q_OBJECT
    DECLARE_THEME

public:
    explicit FPanelButton(QWidget* parent = nullptr);

    void setIcon(const QString& icon, bool keep = false);
    void setText(const QString& text);

signals:
    void clicked();

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void paintEvent(QPaintEvent* event) override;

private:
    void updateStyle();
    void updateIcon();

    QLabel* m_pLabelIcon;
    QLabel* m_pLabelText;

    QString m_strIcon;
    bool m_bKeep;
    bool m_bHover = false;
};