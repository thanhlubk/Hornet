#include <FancyWidgets/FTabBarPane.h>
#include <QPainter>
#include <FancyWidgets/FUtil.h>
#include <QTimer>
#include <QToolButton>

FTabBarPane::FTabBarPane(QWidget *parent)
    : QTabBar(parent), FThemeableWidget()
{
    setAcceptDrops(true);
    setUsesScrollButtons(true);
    SET_UP_THEME(FTabBarPane)
}

void FTabBarPane::applyTheme()
{
    auto arrowLeft = FUtil::getIconColorPath(":/FancyWidgets/res/icon/small/backward.png", getColorTheme(1));
    auto arrowRight = FUtil::getIconColorPath(":/FancyWidgets/res/icon/small/forward.png", getColorTheme(1));

    // setStyleSheet(QString("QTabBar::tab { background: %1; border: none; padding: %4px %5px; color: %2; } "
    //                       "QTabBar::tab:selected { color: %3; } "
    //                       "QTabBar::tab:hover { color: %3; } "
    //                       "QTabBar QToolButton { background: %1; border: none; border-radius: 0px; width: %8px; } "
    //                       "QTabBar QToolButton::left-arrow:enabled { image: url(%6); } "
    //                       "QTabBar QToolButton::right-arrow:enabled { image: url(%7); } "
    //                       "QTabBar QToolButton::left-arrow:disabled { image: none; } "
    //                       "QTabBar QToolButton::right-arrow:disabled { image: none; }")
    //                   .arg(getColorTheme(0).name(), getColorTheme(1).name(), getColorTheme(2).name(), QString::number(getDisplaySize(1).toInt()), QString::number(getDisplaySize(1).toInt() * 2), arrowLeft, arrowRight, QString::number(getDisplaySize(3).toInt())));

    setStyleSheet(QString("QTabBar::tab { background: %1; border: none; padding: %4px %5px; color: %2; } "
                          "QTabBar::tab:selected { color: %3; } "
                          "QTabBar::tab:hover { color: %3; } "
                          "QTabBar QToolButton { background: %1; border: none; border-radius: 0px; } "
                          "QTabBar QToolButton::left-arrow:enabled { image: url(%6); width: 15px; height: 30px;} "
                          "QTabBar QToolButton::right-arrow:enabled { image: url(%7); width: 15px; height: 30px;} "
                          "QTabBar QToolButton::left-arrow:disabled { image: none; width: 15px; height: 30px;} "
                          "QTabBar QToolButton::right-arrow:disabled { image: none; width: 15px; height: 30px;}")
                      .arg(getColorTheme(0).name(), getColorTheme(1).name(), getColorTheme(2).name(), QString::number(getDisplaySize(1).toInt()), QString::number(getDisplaySize(1).toInt() * 2), arrowLeft, arrowRight));

    FUtil::setFontWidget(this, getDisplaySize(0).toInt(), false);
}

QColor FTabBarPane::getColorTheme(int idx)
{
    SET_UP_COLOR(0, Default, backgroundSection1);
    SET_UP_COLOR(1, Default, textNormal);
    SET_UP_COLOR(2, Default, textHeader1);
    SET_UP_COLOR(3, Default, dominant);

    return QColor(qRgb(0, 0, 0));
}

QVariant FTabBarPane::getDisplaySize(int idx)
{
    SET_UP_DISPLAY_SIZE(0, Primary, fontSubSize);
    SET_UP_DISPLAY_SIZE(1, Primary, offsetSize2);
    SET_UP_DISPLAY_SIZE(2, Primary, offsetSize1);
    SET_UP_DISPLAY_SIZE(3, Primary, iconSize3);

    return QVariant(0);
}

void FTabBarPane::mousePressEvent(QMouseEvent *event)
{
    m_pointDragStartPos = event->pos();
    QTabBar::mousePressEvent(event);
}

void FTabBarPane::mouseMoveEvent(QMouseEvent *event)
{
    if (!(event->buttons() & Qt::LeftButton))
        return;

    if ((event->pos() - m_pointDragStartPos).manhattanLength() < QApplication::startDragDistance())
        return;

    int index = tabAt(m_pointDragStartPos);
    if (index < 0)
        return;

    QMimeData* mimeData = new QMimeData;
    mimeData->setData("application/x-tab-index", QByteArray::number(index));
    mimeData->setText(tabText(index));  // optional for debug

    QDrag* drag = new QDrag(this);
    drag->setMimeData(mimeData);
    drag->exec(Qt::MoveAction);
}

void FTabBarPane::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasFormat("application/x-tab-index"))
        event->acceptProposedAction();
}

void FTabBarPane::dropEvent(QDropEvent *event)
{
    auto* sourceTabBar = qobject_cast<FTabBarPane*>(event->source());
    if (!sourceTabBar || sourceTabBar == this)
        return;

    QByteArray data = event->mimeData()->data("application/x-tab-index");
    int index = data.toInt();

    emit tabDropped(sourceTabBar, index);
    event->acceptProposedAction();
}

void FTabBarPane::paintEvent(QPaintEvent *event)
{
    QTabBar::paintEvent(event);

    auto rect = tabRect(currentIndex());

    QPen pen(getColorTheme(3), 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    QBrush brush(getColorTheme(3), Qt::SolidPattern);

    QPainter painter(this);
    painter.setPen(pen);
    painter.setBrush(brush);

    painter.setRenderHint(QPainter::Antialiasing, true);

    QPoint point1{rect.left() + getDisplaySize(1).toInt(), rect.bottom() - 1};
    QPoint point2{rect.right() - getDisplaySize(1).toInt(), rect.bottom() - 1};
    painter.drawLine(point1, point2);
    painter.save();
    painter.restore(); // Restore state
    painter.end();
}

void FTabBarPane::resizeEvent(QResizeEvent *event)
{
    QTabBar::resizeEvent(event);

    for (QToolButton *pButton : findChildren<QToolButton *>())
    {
        if (pButton->arrowType() == Qt::RightArrow)
        {
            // Move 30px away from the right
            int y = (height() - pButton->height()) / 2;
            int x = width() - pButton->width() - getDisplaySize(3).toInt();
            pButton->move(x, y);
        }
        else if (pButton->arrowType() == Qt::LeftArrow)
        {
            // Optional: also adjust the left button if needed
            int y = (height() - pButton->height()) / 2;
            int x = width() - pButton->width() - getDisplaySize(3).toInt() * 2;
            pButton->move(x, y);
        }
    }
}