#include <FancyWidgets/FOutputWidget.h>
#include <FancyWidgets/FUtil.h>
FOutputWidget::FOutputWidget(QWidget *parent)
    : QWidget(parent), FThemeableWidget(), m_bEnableInput(false)
{
    // Output area
    outputEdit = new QPlainTextEdit(this);
    outputEdit->setReadOnly(true);
    outputEdit->setWordWrapMode(QTextOption::NoWrap);

    // Layouts
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(outputEdit);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    setLayout(mainLayout);

    SET_UP_THEME(FOutputWidget)
}

void FOutputWidget::applyTheme()
{
    auto strScrollbarHorizontalStyle = FUtil::getStyleScrollbarHorizontal1(getDisplaySize(5).toInt(), getDisplaySize(1).toInt(), getDisplaySize(1).toInt(), getDisplaySize(1).toInt(), getColorTheme(1).name(), getColorTheme(1).name());

    outputEdit->setStyleSheet(QString("QPlainTextEdit { background: %1; border: none; } ").arg(getColorTheme(4).name()) + strScrollbarHorizontalStyle);
    FUtil::setFontWidget(outputEdit, getDisplaySize(0).toInt(), false);

    updateInputAreaStyle();
}

QColor FOutputWidget::getColorTheme(int idx)
{
    SET_UP_COLOR(0, Default, backgroundSection2);
    SET_UP_COLOR(1, Default, textNormal);
    SET_UP_COLOR(2, Default, textHeader1);
    SET_UP_COLOR(3, Default, dominant);
    SET_UP_COLOR(4, Default, backgroundSection1);

    return QColor(qRgb(0, 0, 0));
}

QVariant FOutputWidget::getDisplaySize(int idx)
{
    SET_UP_DISPLAY_SIZE(0, Primary, fontNormalSize);
    SET_UP_DISPLAY_SIZE(1, Primary, offsetSize1);
    SET_UP_DISPLAY_SIZE(2, Primary, iconSize3);
    SET_UP_DISPLAY_SIZE(3, Primary, filletSize1);
    SET_UP_DISPLAY_SIZE(4, Primary, offsetSize2);

    SET_UP_DISPLAY_SIZE(5, Primary, scrollbarWidth);
    return QVariant(0);
}

void FOutputWidget::appendMessage(const QString &text, const QColor &color)
{
    QTextCharFormat format;
    format.setForeground(color);
    QTextCursor cursor = outputEdit->textCursor();
    cursor.movePosition(QTextCursor::End);
    cursor.setCharFormat(format);
    cursor.insertText(text + "\n");
    outputEdit->verticalScrollBar()->setValue(outputEdit->verticalScrollBar()->maximum());
}

void FOutputWidget::clear()
{
    outputEdit->clear();
}

void FOutputWidget::handleUserInput()
{
    QString input = inputEdit->text().trimmed();
    if (!input.isEmpty())
    {
        appendMessage("> " + input, Qt::cyan); // Show the user input
        emit userInputSubmitted(input);        // Emit for external handling
        inputEdit->clear();
    }
}

void FOutputWidget::enableInput(bool enable)
{
    m_bEnableInput = enable;
    initializeInputArea();
}

void FOutputWidget::initializeInputArea()
{
    if (m_bEnableInput)
    {
        // Input field
        inputEdit = new QLineEdit(this);
        inputEdit->setPlaceholderText("Type a message...");

        // Optional send button (could be omitted for Enter-only UX)
        sendButton = new QPushButton(this);

        // Connect Enter and button
        connect(inputEdit, &QLineEdit::returnPressed, this, &FOutputWidget::handleUserInput);
        connect(sendButton, &QPushButton::clicked, this, &FOutputWidget::handleUserInput);

        // Layout for input area
        QHBoxLayout *inputLayout = new QHBoxLayout();
        inputLayout->addWidget(inputEdit);
        inputLayout->addWidget(sendButton);
        inputLayout->setContentsMargins(0, 0, 0, 0);
        inputLayout->setSpacing(4);

        auto pLayout = qobject_cast<QVBoxLayout *>(layout());
        if (pLayout)
            pLayout->addLayout(inputLayout);

        updateInputAreaStyle();
    }
}

void FOutputWidget::updateInputAreaStyle()
{
    if (m_bEnableInput && sendButton && inputEdit)
    {
        QIcon icon = FUtil::changeIconColor(":/FancyWidgets/res/icon/small/sent.png", getColorTheme(1));
        sendButton->setIcon(icon);
        sendButton->setIconSize(QSize(getDisplaySize(2).toInt(), getDisplaySize(2).toInt())); // Adjust size as needed

        inputEdit->setStyleSheet(QString("QLineEdit { background: %1; color: %2; border: none; border-radius: %3px; padding-left: %4px; }").arg(getColorTheme(0).name(), getColorTheme(1).name(), QString::number(getDisplaySize(3).toInt()), QString::number(getDisplaySize(4).toInt())));
        FUtil::setFontWidget(inputEdit, getDisplaySize(0).toInt(), false);
        inputEdit->setFixedHeight(getDisplaySize(0).toInt() + 2*getDisplaySize(4).toInt()); // Adjust height for better UX
    }
}