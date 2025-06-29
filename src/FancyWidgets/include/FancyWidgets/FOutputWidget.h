#pragma once
#include "FancyWidgetsExport.h"
#include "FThemeableWidget.h"
#include <QPlainTextEdit>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QScrollBar>
#include <QWidget>

class FANCYWIDGETS_EXPORT FOutputWidget : public QWidget, public FThemeableWidget
{
    Q_OBJECT
    DECLARE_THEME

public:
    explicit FOutputWidget(QWidget *parent = nullptr);

    void appendMessage(const QString &text, const QColor &color = Qt::white);
    void clear();
    void enableInput(bool enable);

signals:
    void userInputSubmitted(const QString &text);

private slots:
    void handleUserInput();

private:
    void initializeInputArea();
    void updateInputAreaStyle();

    QPlainTextEdit *outputEdit;
    QLineEdit *inputEdit;
    QPushButton *sendButton;

    bool m_bEnableInput; // Allow input by default
};
