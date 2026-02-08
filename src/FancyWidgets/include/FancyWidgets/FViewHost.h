#pragma once
#include "FancyWidgetsExport.h"
#include <QWidget>
#include <QPointer>

class FANCYWIDGETS_EXPORT FViewHost : public QWidget
{
    Q_OBJECT
public:
    explicit FViewHost(QWidget *parent = nullptr);
    ~FViewHost() override;

    // Currently attached widget (may be nullptr)
    QWidget *viewWidget() const;
    void setActive(bool active);
    bool isActive() const { return m_bIsActive; }
    
signals:
    void activated();
    void deactivated();

public slots:
    // Attach a new widget to this host (reparents it)
    void setViewWidget(QWidget *w);

    // Detach and return the current widget without deleting it
    QWidget *takeViewWidget();

protected:
    void paintEvent(QPaintEvent *event) override;
    bool eventFilter(QObject* obj, QEvent* ev) override;

private:
    QPointer<QWidget> m_pViewWidget; // auto-null when the widget is deleted elsewhere
    bool m_bIsActive = false;
};
