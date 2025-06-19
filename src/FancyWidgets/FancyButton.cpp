#include <FancyWidgets/FancyButton.h>

FancyButton::FancyButton(QWidget* parent)
    : QPushButton(parent) {
    setStyleSheet("background-color: red; color: black; border-radius: 10px;");
}

FancyButton::FancyButton(const QString& text, QWidget* parent)
    : QPushButton(text, parent) {
    setStyleSheet("background-color: red; color: black; border-radius: 10px;");
}