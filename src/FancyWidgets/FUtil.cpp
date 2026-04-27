#include <FancyWidgets/FUtil.h>
#include <QBitmap>
#include <QFileInfo>
#include <QDir>
#include <QApplication>
#include <QPainterPath>

namespace FUtil
{
    QString getTempPath()
    {
        // e.g., return QString("D:/res/toolbar/res/")"
        return QString();
    }

    QStringList getParentFolders(const QString &path) {
        // Get the directory path (excluding file name)
        QFileInfo fileInfo(path);
        QString dirPath = fileInfo.path(); // e.g., D:/res/toolbar/res/icon or :res/toolbar/res/icon

        // Normalize path to use forward slashes
        QString normalizedPath = QDir::cleanPath(dirPath); // Converts \ to /

        // Handle Qt resource prefix (:) or drive letter (D:)
        QStringList folders;
        if (normalizedPath.startsWith(":")) {
            QString cleanedPath = normalizedPath.mid(1); // Remove :
            folders = cleanedPath.split("/", Qt::SkipEmptyParts);
            folders.prepend(":"); // Add : as a special folder
        } else {
            // Split drive letter and path (e.g., D:/res/toolbar/res/icon -> D:, res, toolbar, res, icon)
            QStringList parts = normalizedPath.split("/", Qt::SkipEmptyParts);
            if (normalizedPath.contains(":/")) {
                // Extract drive letter (e.g., D:)
                QString drive = normalizedPath.left(normalizedPath.indexOf(":/") + 2);
                folders.append(drive);
                QString remainingPath = normalizedPath.mid(normalizedPath.indexOf(":/") + 2);
                folders.append(remainingPath.split("/", Qt::SkipEmptyParts));
            } else {
                folders = parts;
            }
        }

        return folders;
    }

    // Custom icon
    QSize getIconLargestSize(const QIcon& icon)
    {
        // Get the list of available sizes
        QList<QSize> sizes = icon.availableSizes();
        if (sizes.isEmpty())
            return QSize(0, 0);

        // Check if any sizes are available
        QSize largestSize;
        for (const QSize &size : sizes) {
            if (size.width() > largestSize.width()) {
                largestSize = size;
            }
        }
        return largestSize;
    }

    QPixmap changePixmapColor(const QPixmap& pixmap, QColor color)
    {
        QPixmap pxColor = QPixmap(pixmap);

        // initialize painter to draw on a pixmap and set composition mode
        QPainter painter(&pxColor);
        painter.setCompositionMode(QPainter::CompositionMode_SourceIn);

        painter.setBrush(color);
        painter.setPen(color);

        painter.drawRect(pxColor.rect());

        return pxColor;
    }

    QIcon changeIconColor(const QIcon& icon, QColor color)
    {
        if (!color.isValid())
            return QIcon();

        QPixmap pixmap = getPixmapFromIcon(icon);

        return changeIconColor(pixmap, color);
    }

    QIcon changeIconColor(const QString& strPath, QColor color)
    {
        if (strPath.isEmpty())
            return QIcon();
        
        if (!color.isValid())
            return QIcon();

        return QIcon(getIconColorPath(strPath, color));
    }

    QIcon changeIconColor(const QPixmap& pxIcon, QColor color)
    {
        // QPixmap pixmap = QPixmap(pxIcon);

        // // initialize painter to draw on a pixmap and set composition mode
        // QPainter painter(&pixmap);
        // painter.setCompositionMode(QPainter::CompositionMode_SourceIn);

        // painter.setBrush(color);
        // painter.setPen(color);

        // painter.drawRect(pixmap.rect());

        // // Here is our new colored icon!
        if (!color.isValid())
            return QIcon();
        return QIcon(changePixmapColor(pxIcon, color));
    }

    QIcon rotateIcon(const QIcon& icon, double degree)
    {
        QPixmap pixmap = icon.pixmap(FUtil::getIconLargestSize(icon));
        QTransform transform;
        transform.rotate(degree);
        QPixmap pixmapRotate = pixmap.transformed(transform);
        return QIcon(pixmapRotate);
    }

    QString getIconColorPath(const QString& strPath, QColor color)
    {
        QFileInfo fileInfo(strPath);
        QString strParent;

        QStringList strlParentFolders = getParentFolders(strPath);
        for (QString strFolder : strlParentFolders) 
        {
            strFolder.remove(":");
            strFolder.remove("/");

            if (strFolder.isEmpty())
                continue;
            
            strParent += "_" + strFolder;
        }

        QString strFileIcon = getTempPath() + "FUtil_" + fileInfo.baseName() + strParent + "_" + color.name(QColor::HexArgb).remove("#") + ".png";
        if (!QFileInfo::exists(strFileIcon))
        {
            QPixmap pixmap = changePixmapColor(strPath, color);
            pixmap.save(strFileIcon);
        }
        return strFileIcon;

        // QPixmap pixmap = QPixmap(strPath);
        // QIcon icon = changeIconColor(pixmap, color);

        // QSize size = getIconLargestSize(icon);
        // QPixmap pixmapColor = icon.pixmap(size);

        // QString iconColorPath = getTempPath() + QFileInfo(strPath).fileName();
        // pixmapColor.save(iconColorPath);
        // return iconColorPath;
    }

    void setFontWidget(QWidget* pWidget, int iSize, bool bBold)
    {
        QFont font;
        font = pWidget->font();
        font.setPointSize(iSize);
        font.setBold(bBold);
        pWidget->setFont(font);
    }

    QString toCamelCase(const QString& str)
    {
        QStringList parts = str.split(' ', Qt::SkipEmptyParts);
        for (int i = 0; i < parts.size(); ++i)
            parts[i].replace(0, 1, parts[i][0].toUpper());

        return parts.join(" ");
    }

    QString getStyleScrollbarVertical1(int width, int space, int paddingLeft, int paddingRight, const QString &colorBar, const QString &colorHover)
    {
        auto style = sStyleScrollbarVertical1.arg(colorBar, colorHover, QString::number(width / 2), QString::number(width + paddingLeft + paddingRight), QString::number(paddingRight), QString::number(paddingLeft), QString::number(space));
        return style;
    }

    QString getStyleScrollbarVertical2(int width, int space, int paddingLeft, int paddingRight, const QString& arrowUpPath, const QString& arrowDownPath, const QString& colorBar, const QString& colorHover)
    {
	    auto arrowUp = FUtil::getIconColorPath(arrowUpPath, colorBar);
        auto arrowDown = FUtil::getIconColorPath(arrowDownPath, colorBar);
	    auto style = sStyleScrollbarVertical2.arg(colorBar, colorHover, arrowUp, arrowDown, QString::number(width), QString::number(width/2), QString::number(width + paddingLeft + paddingRight), QString::number(paddingRight), QString::number(paddingLeft), QString::number(width + space));
        return style;
    }

    QString getStyleScrollbarHorizontal1(int height, int space, int paddingTop, int paddingBottom, const QString &colorBar, const QString &colorHover)
    {
        auto style = sStyleScrollbarHorizontal1.arg(colorBar, colorHover, QString::number(height / 2), QString::number(height + paddingTop + paddingBottom), QString::number(paddingTop), QString::number(paddingBottom), QString::number(space));
        return style;
    }

    QString getStyleScrollbarHorizontal2(int height, int space, int paddingTop, int paddingBottom, const QString &arrowLeftPath, const QString &arrowRightPath, const QString &colorBar, const QString &colorHover)
    {
        auto arrowLeft = FUtil::getIconColorPath(arrowLeftPath, colorBar);
        auto arrowRight = FUtil::getIconColorPath(arrowRightPath, colorBar);
        auto style = sStyleScrollbarHorizontal2.arg(colorBar, colorHover, arrowLeft, arrowRight, QString::number(height), QString::number(height / 2), QString::number(height + paddingTop + paddingBottom), QString::number(paddingTop), QString::number(paddingBottom), QString::number(height + space));
        return style;
    }

    QIcon createNumberIcon(int number, QColor colorBackground, QColor colorNumber)
    {
        // Create a QPixmap for the icon (e.g., 32x32 pixels)
        QPixmap pixmap(100, 100);
        pixmap.fill(Qt::transparent); // Transparent background

        // Create a QPainter to draw on the pixmap
        QPainter painter(&pixmap);
        painter.setRenderHint(QPainter::Antialiasing); // Smooth edges

        // Draw a blue filled circle
        QFont font = QApplication::font(); // Get the application's font
        font.setPointSize(50);             // Set font size to 14
        font.setBold(false);                // Optional: Make it bold for better visibility
        painter.setFont(font);
        painter.setPen(Qt::NoPen);         // No outline
        painter.setBrush(colorBackground);     // Blue color (#0066CC)
        painter.drawEllipse(0, 0, 100, 100);   // Full pixmap size circle

        // Set font for the number
        painter.setPen(colorNumber); // White color for text

        // Convert number to string and draw it centered
        QString text = QString::number(number);
        QRect textRect(0, 0, 100, 100);
        painter.drawText(textRect, Qt::AlignCenter, text);

        // Create and return a QIcon from the pixmap
        return QIcon(pixmap);
    }

    QPixmap getPixmapFromIcon(const QIcon &icon)
    {
        if (icon.isNull())
            return QPixmap();

        // Get the largest available size for the icon
        QSize largestSize = icon.availableSizes().isEmpty() ? QSize(100, 100) : icon.availableSizes().first();
        return icon.pixmap(largestSize);
    }

    QPixmap getPixmapFromIcon(const QString &strPath)
    {
        if (strPath.isEmpty())
            return QPixmap();

        // Load the pixmap from the file path
        QPixmap pixmap(strPath);
        if (pixmap.isNull())
            return QPixmap();

        // Return the loaded pixmap
        return pixmap;
    }

    void fillRoundedRect(QPainter *painter, const QRectF &rect, const QColor &color, qreal topLeftRadius, qreal topRightRadius, qreal bottomRightRadius, qreal bottomLeftRadius)
    {
        // Save painter state
        painter->save();

        // Enable antialiasing for smooth corners
        painter->setRenderHint(QPainter::Antialiasing);
        // Create a QPainterPath for the custom rounded rectangle
        QPainterPath path;
        qreal x = rect.x();
        qreal y = rect.y();
        qreal width = rect.width();
        qreal height = rect.height();

        topLeftRadius = std::max<qreal>(0, topLeftRadius);
        topRightRadius = std::max<qreal>(0, topRightRadius);
        bottomRightRadius = std::max<qreal>(0, bottomRightRadius);
        bottomLeftRadius = std::max<qreal>(0, bottomLeftRadius);

        // Start at the top-left corner (just after the top-left corner to prepare for arc or line)
        path.moveTo(x + topLeftRadius, y);

        // Top-right corner (rounded if topRightRadius > 0, else sharp)
        if (topRightRadius == 0)
            path.lineTo(x + width, y);
        else
            path.arcTo(x + width - 2 * topRightRadius, y, 2 * topRightRadius, 2 * topRightRadius, 90, -90);

        // Bottom-right corner (rounded if bottomRightRadius > 0, else sharp)
        if (bottomRightRadius == 0)
            path.lineTo(x + width, y + height);
        else
            path.arcTo(x + width - 2 * bottomRightRadius, y + height - 2 * bottomRightRadius, 2 * bottomRightRadius, 2 * bottomRightRadius, 0, -90);

        // Bottom-left corner (rounded if bottomLeftRadius > 0, else sharp)
        if (bottomLeftRadius == 0)
            path.lineTo(x, y + height);
        else
            path.arcTo(x, y + height - 2 * bottomLeftRadius, 2 * bottomLeftRadius, 2 * bottomLeftRadius, 270, -90);

        // Top-left corner (rounded if topLeftRadius > 0, else sharp)
        if (topLeftRadius == 0)
            path.lineTo(x, y);
        else
            path.arcTo(x, y, 2 * topLeftRadius, 2 * topLeftRadius, 180, -90);

        // Close the path
        path.closeSubpath();

        // Draw the path
        painter->fillPath(path, QBrush(color)); // fill only; never paints outside rect
        painter->restore();
    }
    
    void drawRoundedRect(QPainter *painter, const QRectF &rect, qreal borderWidth, QColor borderColor, qreal topLeftRadius, qreal topRightRadius, qreal bottomRightRadius, qreal bottomLeftRadius)
    {
        // Save painter state
        painter->save();

        // Enable antialiasing for smooth corners
        painter->setRenderHint(QPainter::Antialiasing);

        // Set the pen for the border
        QPen pen(borderColor, borderWidth);
        pen.setCapStyle(Qt::FlatCap);
        painter->setPen(pen);

        // Create a QPainterPath for the custom rounded rectangle
        QPainterPath path;
        qreal x = rect.x();
        qreal y = rect.y();
        qreal width = rect.width();
        qreal height = rect.height();
        qreal haftBorderWidth = borderWidth / 2;

        // Start at the top-left corner (just after the top-left corner to prepare for arc or line)
        path.moveTo(x + topLeftRadius, y + haftBorderWidth);

        // Top-right corner (rounded if topRightRadius > 0, else sharp)
        if (topRightRadius > 0)
            path.arcTo(x + width - haftBorderWidth - 2 * topRightRadius, y + haftBorderWidth, 2 * topRightRadius, 2 * topRightRadius, 90, -90);
        else
        {
            path.lineTo(x + width, y + haftBorderWidth);
            path.lineTo(x + width - haftBorderWidth, y + haftBorderWidth);
        }

        // Bottom-right corner (rounded if bottomRightRadius > 0, else sharp)
        if (bottomRightRadius > 0)
            path.arcTo(x + width - haftBorderWidth - 2 * bottomRightRadius, y + height - 2 * bottomRightRadius - haftBorderWidth, 2 * bottomRightRadius, 2 * bottomRightRadius, 0, -90);
        else
        {
            path.lineTo(x + width - haftBorderWidth, y + height);
            path.lineTo(x + width - haftBorderWidth, y + height - haftBorderWidth);
        }

        // Bottom-left corner (rounded if bottomLeftRadius > 0, else sharp)
        if (bottomLeftRadius > 0)
            path.arcTo(x + haftBorderWidth, y + height - haftBorderWidth - 2 * bottomLeftRadius, 2 * bottomLeftRadius, 2 * bottomLeftRadius, 270, -90);
        else
        {
            path.lineTo(x, y + height - haftBorderWidth);
            path.lineTo(x + haftBorderWidth, y + height - haftBorderWidth);
        }

        // Top-left corner (rounded if topLeftRadius > 0, else sharp)
        if (topLeftRadius > 0)
            path.arcTo(x + haftBorderWidth, y + haftBorderWidth, 2 * topLeftRadius, 2 * topLeftRadius, 180, -90);
        else
            path.lineTo(x + haftBorderWidth, y + haftBorderWidth);

        // Close the path
        path.closeSubpath();

        // Draw the path
        painter->drawPath(path);

        // Restore painter state
        painter->restore();
    }
}