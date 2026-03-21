#include <FancyWidgets/FLabel.h>
#include <FancyWidgets/FUtil.h>

FLabel::FLabel(QWidget *parent)
    : FLabel(QString(""), parent)
{
}

FLabel::FLabel(const QString &text, QWidget *parent)
    : QLabel(text, parent), FThemeableWidget()
{
    SET_UP_THEME(FLabel)
}

void FLabel::applyTheme()
{
    setStyleSheet(QString("QLabel { color: %1; background: %2; }")
                      .arg(getColorTheme(0).name(),
                           getColorTheme(1).name(QColor::HexArgb)));
    FUtil::setFontWidget(this, getDisplaySize(0).toInt(), false);
}

QColor FLabel::getColorTheme(int idx)
{
    SET_UP_COLOR(0, Default, textNormal);
    SET_UP_COLOR(1, Default, transparent);

    return QColor(qRgb(0, 0, 0));
}

QVariant FLabel::getDisplaySize(int idx)
{
    SET_UP_DISPLAY_SIZE(0, Primary, fontNormalSize);

    return QVariant(0);
}
