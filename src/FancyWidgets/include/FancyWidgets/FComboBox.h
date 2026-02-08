#pragma once
#include "FancyWidgetsExport.h"
#include <QComboBox>
#include <QCompleter>
#include <QLineEdit>
#include <QWidget>
#include "FThemeableWidget.h"

class FANCYWIDGETS_EXPORT FComboBox : public QComboBox, public FThemeableWidget
{
    Q_OBJECT
    DECLARE_THEME

public:
    explicit FComboBox(QWidget* parent = nullptr);
    ~FComboBox();
    void enablePopupExpand(bool enable) { m_bPopupExpand = enable; }

protected:
    void paintEvent(QPaintEvent* e) override;
    void hidePopup() override;
    void showPopup() override;

private:
    // Internal-only popup view + delegate.
    // Defined in FComboBox.cpp as private nested classes so they cannot be used elsewhere.
    class FComboBoxAnimationView;
    class FComboBoxAnimationDelegate;

    int m_iOffsetPopup;
    // Bottom line when popup is open
    bool m_popupOpen = false;   // NEW: only draw bottom line when true
    bool m_bPopupExpand;
};