#pragma once
#include "FancyWidgetsExport.h"
#include "FArrowButton.h"
#include <QWidget>
#include <QListWidget>
#include <QPropertyAnimation>
#include <QPointer>
#include <QList>
#include <QVBoxLayout>
#include <QWheelEvent>

class FNoScrollListWidget : public QListWidget {
public:
    using QListWidget::QListWidget;

    void enableSpacingBorder(bool enable) 
    {
        if (!enable)
            setViewportMargins(-spacing(), 0, -spacing()-1, 0);
        else
            setViewportMargins(0, 0, 0, 0);
    }

protected:
    void wheelEvent(QWheelEvent* event) override {
        event->ignore();
    }

    void scrollTo(const QModelIndex &index, ScrollHint hint = EnsureVisible) override {
        // Do nothing — disables all auto-scroll behavior
        Q_UNUSED(index);
        Q_UNUSED(hint);
    }
};

class FANCYWIDGETS_EXPORT FPanel : public QWidget, public FThemeableWidget 
{
    Q_OBJECT
    DECLARE_THEME

public:
    explicit FPanel(QWidget* parent = nullptr);

    void observeWidget(QWidget* child);
    void updateContainerHeight();
    void resizeEvent(QResizeEvent* event);
    void setRun(bool run);
    void addWidget(QWidget* widget);

signals:
    void heightChanged();

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;
    void paintEvent(QPaintEvent* event) override;
    void updateExpandHeight();

private slots:
    void onArrowToggled(bool expanded);

private:
    FArrowButton* m_pButtonHeader;
    FNoScrollListWidget* m_pListContent;
    QVBoxLayout* m_pLayout;

    QList<QPointer<QWidget>> m_listObservedChildren;
    
    int m_iExpandHeight;
    bool m_bRun;
};