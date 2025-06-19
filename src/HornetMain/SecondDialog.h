#pragma once

#include <QDialog>

QT_BEGIN_NAMESPACE
namespace Ui {
class SecondDialog;
}
QT_END_NAMESPACE

class SecondDialog : public QDialog {
    Q_OBJECT

public:
    explicit SecondDialog(QWidget *parent = nullptr);
    ~SecondDialog();

private slots:
    void OnCompareImage();
    void OnChangeImageSize(int value);
    void OnAddImage();

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;

private:
    Ui::SecondDialog *ui;
};