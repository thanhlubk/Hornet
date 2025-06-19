#include <FancyWidgets/FUtil.h>
#include <QBitmap>
#include <QFileInfo>
#include <QDir>

namespace FUtil
{
    QString getTempPath()
    {
        return QString("D:/Temp/Fancy/");
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

    // Custom icon
    QIcon changeIconColor(const QIcon& icon, QColor color)
    {
        if (!color.isValid())
            return QIcon();

        auto largestSize = getIconLargestSize(icon);
        QPixmap pixmap = icon.pixmap(largestSize);

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

    QString getStyleScrollbarVertical2(int width, int space, int paddingLeft, int paddingRight, const QString& arrowUpPath, const QString& arrowDownPath, const QString& colorBar, const QString& colorHover)
    {
	    auto arrowUp = FUtil::getIconColorPath(arrowUpPath, colorBar);
        auto arrowDown = FUtil::getIconColorPath(arrowDownPath, colorBar);
	    auto style = sStyleScrollbarVertical2.arg(colorBar, colorHover, arrowUp, arrowDown, QString::number(width), QString::number(width/2), QString::number(width + paddingLeft + paddingRight), QString::number(paddingRight), QString::number(paddingLeft), QString::number(width + space));
        return style;
    }

}