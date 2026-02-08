#pragma once
#include "FancyWidgetsExport.h"
#include <QObject>
#include <QColor>
#include <QMap>
#include <QString>

#define GET_THEME_COLOR(style) FThemeManager::instance().currentTheme().##style
#define GET_FILLET_SIZE(index) FThemeManager::instance().currentDisplay().filletSize##index
#define GET_OFFSET_SIZE(index) FThemeManager::instance().currentDisplay().offsetSize##index
#define GET_FONT_SIZE(name) FThemeManager::instance().currentDisplay().font##name
#define GET_DISPLAY_SIZE(name) FThemeManager::instance().currentDisplay().##name

struct Theme 
{
    // Default color - not be read from json
    QColor transparent;

    // Custom color
    QColor backgroundApp; //COLOR_GRAY_1
    QColor backgroundSection1; //COLOR_GRAY_3
    QColor backgroundSection2; //COLOR_GRAY_2
    QColor backgroundSection3;

    QColor widgetNormal; //COLOR_GRAY_6
    QColor widgetHover; //COLOR_GRAY_7
    QColor widgetCheck; //COLOR_GRAY_8

    QColor textHeader1; //COLOR_WHITE_1
    QColor textHeader2; //COLOR_WHITE_2
    QColor textNormal; //COLOR_GRAY_4
    QColor textSub; 

    QColor dominant;

    QColor success;
    QColor fail;


    Theme()
    {
        transparent = QColor(0, 0, 0, 0);
    }
};

struct NumericDisplay 
{
    int fontHeaderSize1; // FONT_SIZE_1
    int fontHeaderSize2; // FONT_SIZE_2
    int fontHeaderSize3;// FONT_SIZE_3
    int fontNormalSize;// FONT_SIZE_4
    int fontSubSize;// FONT_SIZE_5

    int iconSize1;// ICON_SIZE_1
    int iconSize2;// ICON_SIZE_2
    int iconSize3;// ICON_SIZE_3

    int filletSize1;// FILLET_SIZE_1
    int filletSize2;// FILLET_SIZE_2
    
    int offsetSize1;// OFFSET_SIZE_1
    int offsetSize2;// OFFSET_SIZE_2

    int highlightWidth1;
    int highlightWidth2;

    int separatorWidth;

    int scrollbarWidth1;
    int scrollbarWidth2;

    int sidebarTabHeight;
    int sidebarTabWidth;
    int searchbarWidth;
    int titlebarHeight;
};

class FANCYWIDGETS_EXPORT FThemeManager : public QObject {
    Q_OBJECT
public:
    static FThemeManager& instance();
    const Theme& currentTheme() const;
    const NumericDisplay& currentDisplay() const;
    void load(const QString& theme, const QString& display);
    void setTheme(const QString& name);

protected:
    void addTheme(const QString& name, Theme& theme);
    void removeTheme(const QString& name);
    QStringList getAvailableThemes();
    void loadThemeJson(const QString& theme);
    void loadDisplayJson(const QString& display);

signals:
    void themeChanged(const Theme& theme);

private:
    FThemeManager();
    QMap<QString, Theme> m_mapThemes;
    QString m_strCurrentTheme;
    NumericDisplay m_stDisplay;
};