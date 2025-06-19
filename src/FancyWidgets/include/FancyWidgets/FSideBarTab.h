#pragma once
#include "FancyWidgetsExport.h"
#include <QWidget>
#include <QPushButton>
#include <QIcon>
#include <QLabel>
#include <QVBoxLayout>
#include <QEvent>
#include "FThemeableWidget.h"
#include "FThemeManager.h"

class FANCYWIDGETS_EXPORT FSideBarTab : public QWidget, public FThemeableWidget
{
    Q_OBJECT
    DECLARE_THEME

public:
    explicit FSideBarTab(QWidget* parent = nullptr);

    void setIcon(const QIcon& iconNormal, const QIcon& iconChecked);
    void setIcon(const QString& iconNormalPath, const QString& iconCheckedPath);
    void setChecked(bool checked);
    bool isChecked() const;
    void setTitle(const QString& title);
    QString title() const;

signals:
    void clicked();

protected:
    void paintEvent(QPaintEvent* event) override;
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private:
    void updateIconAndStyle(bool active);
    void updateBackground(bool active);

    QIcon m_iconNormal;
    QIcon m_iconHover;
    QIcon m_iconChecked;
    QString m_strTitle;

    QLabel* m_pIconLabel;
    QLabel* m_pTextLabel;
    QFrame *m_pFrame;

    bool m_bKeepIcon;
    bool m_bCheck;
};