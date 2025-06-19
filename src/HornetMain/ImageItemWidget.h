#pragma once

#include <QWidget>
#include <QString>

class QLabel;

class ImageItemWidget : public QWidget {
    Q_OBJECT
public:
    explicit ImageItemWidget(const QString &imagePath, QWidget *parent = nullptr, int scalePercent = 100);
    void setScale(int percent);

private:
    QLabel* imageLabel;
    QPixmap originalPixmap;
    int currentScale;
    void updatePixmap();
};