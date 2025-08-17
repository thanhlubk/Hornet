#pragma once
#include "FancyWidgetsExport.h"
#include <QListWidget>
#include <QEvent>

// Attach size tracking logic
class FListWidgetResizeWatcher : public QObject
{
public:
    FListWidgetResizeWatcher(QListWidgetItem *item, QWidget *watched, QListWidget *list, QObject* parent = nullptr);

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    QListWidgetItem *m_pListItem;
    QListWidget *m_pList;
};

class FANCYWIDGETS_EXPORT FListWidget : public QListWidget
{
    Q_OBJECT

public:
    explicit FListWidget(QWidget *parent = nullptr);

    void addWidget(QWidget *widget);
    void enableSpacingBorder(bool enable);
    bool isSpacingBorderEnabled() const;
    void setSpacing(int spacing);
    void setContentsMargins(int left, int top, int right, int bottom);
    void setContentsMargins(const QMargins &margins);
    QMargins contentsMargins() const;

protected:
    virtual QMargins adjustViewportMargins();
    virtual void updateViewPort();
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    bool m_bEnableSpacingBorder;
    QMap<QWidget*, QListWidgetItem*> m_mapItemWidget;
    QMargins m_marginsOverride;
};