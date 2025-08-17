#include <FancyWidgets/FTable.h>
#include <QHeaderView>

FTable::FTable(QWidget *parent)
    : QTableWidget(parent), FThemeableWidget(), m_bFitHeight(false)
{
    SET_UP_THEME(FTable)
}

void FTable::applyTheme()
{
    // setStyleSheet(QString("background: %1;").arg(getColorTheme(0).name(QColor::HexArgb)));
    // setStyleSheet(QString("QTableWidget { background-color: %1; border: none; outline: none; border-radius: 2px; color: %3; }"
    //                       "QTableWidget::item:alternate { background-color: %1; border: 2px solid yellow; }"
    //                       "QTableWidget::item { background-color: %2; border: 2px solid yellow; }"
    //                       "QTableCornerButton::section { background: %1; border: 0;}"
    //                       "QHeaderView { background-color: %1; color: %3; padding-bottom: 5px;}"
    //                       "QHeaderView::section { background-color: %1; border: 0;}"
    //                       "QHeaderView::section:checked { background-color: %1; }")
    //                   .arg(getColorTheme(0).name(), getColorTheme(1).name(), getColorTheme(2).name()));

    setStyleSheet(QString("QHeaderView::section { background-color: red; padding: 4px; font-size: 14pt; border-style: none; border-bottom: 1px solid yellow; border-right: 1px solid yellow; }QHeaderView::section:horizontal { border-top: 1px solid yellow;} QHeaderView::section:vertical {     border-left: 1px solid yellow; } "));

    adjustHeightToContents();
}

QColor FTable::getColorTheme(int idx)
{
    SET_UP_COLOR(0, Default, success);
    SET_UP_COLOR(1, Default, fail);
    SET_UP_COLOR(2, Default, textHeader1);
    SET_UP_COLOR(3, Default, dominant);

    return QColor(qRgb(0, 0, 0));
}

QVariant FTable::getDisplaySize(int idx)
{
    SET_UP_DISPLAY_SIZE(0, Primary, offsetSize1);
    return QVariant(0); // Default font size
}

void FTable::addRow(const QList<QWidget *> &listRowWigets)
{
    int rowCount = this->rowCount();
    insertRow(rowCount);
    for (int i = 0; i < listRowWigets.size(); ++i)
    {
        setCellWidget(rowCount, i, listRowWigets[i]);
    }

    adjustHeightToContents();
}

void FTable::deleteRow(const int rowIndex)
{
    if (rowIndex >= 0 && rowIndex < rowCount())
    {
        removeRow(rowIndex);
    }

    adjustHeightToContents();
}

void FTable::enableFitHeight(bool enable)
{
    m_bFitHeight = enable;
    if (m_bFitHeight)
    {
        adjustHeightToContents();
        setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    }
    else
    {
        setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        setMaximumHeight(QWIDGETSIZE_MAX);
        setMinimumHeight(0);
    }
}

bool FTable::isFitHeight() const
{
    return m_bFitHeight;
}

void FTable::adjustHeightToContents()
{
    if (!m_bFitHeight)
        return;

    int totalHeight = 0;

    for (int i = 0; i < rowCount(); ++i)
        totalHeight += rowHeight(i);
        
    // if (horizontalHeader()->isVisible())
    //     totalHeight += horizontalHeader()->height();

    totalHeight += horizontalHeader()->height();

    totalHeight += 2 * frameWidth(); // top + bottom frame

    setMaximumHeight(totalHeight);
    setMinimumHeight(totalHeight);
}
