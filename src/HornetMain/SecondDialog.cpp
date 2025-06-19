#include "SecondDialog.h"
#include "ui_SecondDialog.h"
#include "ImageItemWidget.h"

#include <QFileDialog>
#include <QDir>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPushButton>
#include <QLineEdit>
#include <QMouseEvent>
#include <QSpinBox>
#include <QImage>
#include <QCryptographicHash>
#include <QMap>

#include <bitset>

// Returns a 64-bit aHash for a given image file
std::bitset<64> averageHash(const QString& path) {
    QImage img(path);
    if (img.isNull()) return std::bitset<64>();

    QImage scaled = img.convertToFormat(QImage::Format_Grayscale8)
                        .scaled(8, 8, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

    // Compute average brightness
    int total = 0;
    QVector<uchar> pixels;
    for (int y = 0; y < 8; ++y) {
        const uchar* row = scaled.constScanLine(y);
        for (int x = 0; x < 8; ++x) {
            uchar val = row[x];
            pixels.append(val);
            total += val;
        }
    }

    int avg = total / 64;

    // Create hash
    std::bitset<64> hash;
    for (int i = 0; i < 64; ++i) {
        hash[i] = pixels[i] > avg;
    }

    return hash;
}

// Hamming distance between two aHashes
int hammingDistance(const std::bitset<64>& h1, const std::bitset<64>& h2) {
    return (h1 ^ h2).count();
}

SecondDialog::SecondDialog(QWidget *parent)
    : QDialog(parent), ui(new Ui::SecondDialog) {
    ui->setupUi(this);

    ui->folderLineEdit->setReadOnly(true);
    ui->folderLineEdit->installEventFilter(this);
    
    ui->listWidget->setViewMode(QListWidget::ListMode);
    ui->listWidget->setResizeMode(QListWidget::Adjust);
    ui->listWidget->setFlow(QListWidget::LeftToRight);
    ui->listWidget->setWrapping(true);
    ui->listWidget->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    ui->listWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);

    ui->addButton->setStyleSheet("background: red; margin-left: 20px;");
    // ui->widget->setStyleSheet("background: red; margin-left: 20px;");

    // ui->widget_2->setIcon(":/toolbar/res/toolbar-button-cube-normal.png", ":/toolbar/res/toolbar-button-cube-check.png");
    // ui->widget_2->setFixedHeight(50);
    // ui->widget_2->setStyleSheet("background: blue; margin-left: 20px;");
    connect(ui->addButton, &QPushButton::clicked, this, &SecondDialog::OnAddImage);
    connect(ui->scaleSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &SecondDialog::OnChangeImageSize);
    connect(ui->compareButton, &QPushButton::clicked, this, &SecondDialog::OnCompareImage);

    connect(ui->listWidget->selectionModel(), &QItemSelectionModel::selectionChanged,
        this, [this]() {
            // Apply red border to selected items
            for (int i = 0; i < ui->listWidget->count(); ++i) {
                QListWidgetItem* item = ui->listWidget->item(i);
                QWidget* widget = ui->listWidget->itemWidget(item);
                if (item->isSelected()) {
                    widget->setStyleSheet("border: 2px solid red; border-radius: 4px;");
                } else {
                    widget->setStyleSheet("");
                }
            }
        });
}

void SecondDialog::OnAddImage()
{
    QString folderPath = ui->folderLineEdit->text();
    if (folderPath.isEmpty()) return;

    QDir dir(folderPath);
    QStringList imageFilters = { "*.png", "*.jpg", "*.jpeg", "*.bmp", "*.gif" };
    QStringList imageFiles = dir.entryList(imageFilters, QDir::Files);
    int scale = ui->scaleSpinBox->value();

    for (const QString& fileName : imageFiles) {
        QString fullPath = dir.absoluteFilePath(fileName);
        QListWidgetItem* item = new QListWidgetItem(ui->listWidget);
        int baseSize = 120;
        int scaledSize = static_cast<int>(baseSize * (scale / 100.0));
        item->setSizeHint(QSize(scaledSize, scaledSize));

        ImageItemWidget* widget = new ImageItemWidget(fullPath, ui->listWidget);
        widget->setProperty("imagePath", fullPath);
        ui->listWidget->addItem(item);
        ui->listWidget->setItemWidget(item, widget);
    }
}

void SecondDialog::OnChangeImageSize(int value)
{
    int baseSize = 120;
    int newSize = static_cast<int>(baseSize * (value / 100.0));
    for (int i = 0; i < ui->listWidget->count(); ++i) {
        QListWidgetItem* item = ui->listWidget->item(i);
        item->setSizeHint(QSize(newSize, newSize));

        auto itemWidget = dynamic_cast<ImageItemWidget*>(ui->listWidget->itemWidget(item));
        if (itemWidget)
            itemWidget->setScale(value);
    }
}

void SecondDialog::OnCompareImage()
{
    QVector<std::bitset<64>> hashes;
    QVector<int> itemIndexes;

    for (int i = 0; i < ui->listWidget->count(); ++i) {
        QListWidgetItem* item = ui->listWidget->item(i);
        QWidget* widget = ui->listWidget->itemWidget(item);
        if (auto* imageWidget = qobject_cast<ImageItemWidget*>(widget)) {
            QString path = imageWidget->property("imagePath").toString();
            hashes.append(averageHash(path));
            itemIndexes.append(i);
        }
    }

    for (int i = 0; i < hashes.size(); ++i) {
        for (int j = i + 1; j < hashes.size(); ++j) {
            int distance = hammingDistance(hashes[i], hashes[j]);
            if (distance <= 5) {  // tweak threshold to control sensitivity
                // ui->listWidget->item(itemIndexes[i])->setBackground(Qt::yellow);
                // ui->listWidget->item(itemIndexes[j])->setBackground(Qt::yellow);

                ui->listWidget->item(itemIndexes[i])->setSelected(true);
                ui->listWidget->item(itemIndexes[j])->setSelected(true);

            }
        }
    }
}

bool SecondDialog::eventFilter(QObject* obj, QEvent* event) {
    if (obj == ui->folderLineEdit && event->type() == QEvent::MouseButtonPress) {
        QString dir = QFileDialog::getExistingDirectory(this, "Select Image Folder");
        if (!dir.isEmpty()) {
            ui->folderLineEdit->setText(dir);
        }
        return true;
    }
    return QDialog::eventFilter(obj, event);
}

SecondDialog::~SecondDialog() {
    delete ui;
}
