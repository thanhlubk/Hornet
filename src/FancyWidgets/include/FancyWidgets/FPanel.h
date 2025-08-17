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
#include "FListWidget.h"

class FListWidgetNoScroll : public FListWidget
{
public:
    using FListWidget::FListWidget;

    // void enableSpacingBorder(bool enable) 
    // {
    //     if (!enable)
    //         setViewportMargins(-spacing(), 0, -spacing()-1, 0);
    //     else
    //         setViewportMargins(0, 0, 0, 0);
    // }

protected:
    void wheelEvent(QWheelEvent* event) override {
        event->ignore();
    }

    void scrollTo(const QModelIndex &index, ScrollHint hint = EnsureVisible) override {
        // Do nothing — disables all auto-scroll behavior
        Q_UNUSED(index);
        Q_UNUSED(hint);
    }

    // void resizeEvent(QResizeEvent *event) override
    // {
    //     QListWidget::resizeEvent(event);
    //     // Adjust the size of all items based on the new width
    //     if (viewMode() == QListView::ListMode)
    //     {
    //         for (int i = 0; i < count(); ++i)
    //         {
    //             QListWidgetItem *item = this->item(i);
    //             if (item)
    //             {
    //                 QSize newSize = item->sizeHint();
    //                 newSize.setWidth(viewport()->width() - spacing()*2); // Adjust width to fit the view
    //                 item->setSizeHint(newSize);
    //             }
    //         }
    //         update();
    //     }
    // }
};

class FANCYWIDGETS_EXPORT FPanel : public QWidget, public FThemeableWidget 
{
    Q_OBJECT
    DECLARE_THEME

public:
    enum ContentMode
    {
        VerticalList = 0,
        HorizontalList = 1,
        Wrap = 2,
    };
    Q_ENUM(ContentMode)

    explicit FPanel(QWidget* parent = nullptr);
    explicit FPanel(ContentMode mode, QWidget *parent = nullptr);

    void observeWidget(QWidget* child);
    void updateContainerHeight();
    void resizeEvent(QResizeEvent* event);
    void setRun(bool run);
    void addWidget(QWidget* widget);
    void setContentMode(ContentMode wrap);
    FArrowButton* header() const { return m_pButtonHeader; }

signals: 
    void heightChanged();
    void expanded(bool expanded);
    
protected:
    bool eventFilter(QObject* watched, QEvent* event) override;
    void paintEvent(QPaintEvent* event) override;
    void updateExpandHeight();

private slots:
    void onArrowToggled(bool expanded);

private:
    FArrowButton* m_pButtonHeader;
    FListWidgetNoScroll* m_pListContent;
    QVBoxLayout* m_pLayout;

    QList<QPointer<QWidget>> m_listObservedChildren;
    
    int m_iExpandHeight;
    bool m_bRun;
};