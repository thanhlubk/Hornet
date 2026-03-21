#pragma once
#include "FancyWidgetsExport.h"
#include <QWidget>
#include <QLineEdit>
#include <QHBoxLayout>
#include "FThemeableWidget.h"
#include "FThemeManager.h"

class FANCYWIDGETS_EXPORT FLineEdit : public QWidget, public FThemeableWidget
{
    Q_OBJECT
    DECLARE_THEME

public:
    explicit FLineEdit(QWidget *parent = nullptr);

    void setText(const QString &text);
    QString text() const;
    void setPlaceholderText(const QString &text);
    void setReadOnly(bool readOnly);

signals:
    void textChanged(const QString &text);
    void returnPressed();

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    bool eventFilter(QObject *watched, QEvent *event) override;


    QLineEdit   *m_pEdit    = nullptr;
    QHBoxLayout *m_pLayout  = nullptr;
    bool         m_bFocused = false;
};
