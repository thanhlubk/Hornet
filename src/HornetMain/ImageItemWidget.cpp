#include "ImageItemWidget.h"
#include <QLabel>
#include <QHBoxLayout>
#include <QPixmap>

ImageItemWidget::ImageItemWidget(const QString &imagePath, QWidget *parent, int scalePercent)
    : QWidget(parent), currentScale(scalePercent)
{
    imageLabel = new QLabel(this);
    // originalPixmap = QPixmap(imagePath);
    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->addWidget(imageLabel);
    setLayout(layout);

    originalPixmap = QPixmap(imagePath);
    if (originalPixmap.isNull()) {
        qWarning() << "Failed to load image:" << imagePath;
    } else {
        imageLabel->setPixmap(originalPixmap.scaled(100, 100, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }
    // updatePixmap();
}

void ImageItemWidget::setScale(int percent) {
    currentScale = percent;
    updatePixmap();
}

void ImageItemWidget::updatePixmap() {
    if (!originalPixmap.isNull()) {
        QSize scaledSize(currentScale, currentScale);
        imageLabel->setPixmap(originalPixmap.scaled(scaledSize, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }
}