#include <FancyWidgets/FCheckBox.h>

FCheckBox::FCheckBox(QWidget *parent)
    : FCheckBox(QString(""), parent)
{
}

FCheckBox::FCheckBox(const QString &text, QWidget *parent)
    : QCheckBox(parent), FThemeableWidget()
{
    setFocusPolicy(Qt::NoFocus);
    setAttribute(Qt::WA_Hover, true);
    setAttribute(Qt::WA_TransparentForMouseEvents, false);
    setText(text);

    SET_UP_THEME(FCheckBox)
}

void FCheckBox::applyTheme()
{
    // setStyleSheet(QString("QCheckBox::indicator:checked { image: url(); width: 25px; height: 25px; } QCheckBox::indicator:unchecked { width: 25px; height: 25px; image: url(); } QCheckBox { color: red; }"));
    QString strTickImgPath;
    setStyleSheet(QString(R"(
        QCheckBox {
            background-color: %1;
            border-radius: %2px;
        }
        QCheckBox::indicator {
            border-radius: %2px;
            width: %3px;
            height: %3px;
        }
        QCheckBox::indicator:unchecked {
            image: url(%6);
            background-color: %4;
        }
        QCheckBox::indicator:checked {
            image: none;
            background-color: %5;
        }
    )").arg(getColorTheme(2).name(QColor::HexArgb), QString::number(getDisplaySize(1).toInt()), QString::number(getDisplaySize(0).toInt()), getColorTheme(0).name(), getColorTheme(1).name(), strTickImgPath));

    if (text().isEmpty())
        setMaximumWidth(getDisplaySize(0).toInt());
}

QColor FCheckBox::getColorTheme(int idx)
{
    SET_UP_COLOR(0, Default, dominant);
    SET_UP_COLOR(1, Default, backgroundSection2);
    SET_UP_COLOR(2, Default, transparent);

    return QColor(qRgb(0, 0, 0));
}

QVariant FCheckBox::getDisplaySize(int idx)
{
    SET_UP_DISPLAY_SIZE(0, Primary, iconSize3);
    SET_UP_DISPLAY_SIZE(1, Primary, filletSize1);

    return QVariant(0); // Default font size
}

void FCheckBox::setText(const QString &text)
{
    QCheckBox::setText(text);
    if (text.isEmpty())
        setMaximumWidth(getDisplaySize(0).toInt());
    else
        setMaximumWidth(16777215);
}