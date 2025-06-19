#include "FancyWidgets/FThemeManager.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>

FThemeManager::FThemeManager() {
    // m_mapThemes["Void"] = Theme();
    // m_strCurrentTheme = "Void";
}

FThemeManager& FThemeManager::instance() {
    static FThemeManager inst;
    return inst;
}

const Theme& FThemeManager::currentTheme() const {
    return m_mapThemes[m_strCurrentTheme];
}

const NumericDisplay& FThemeManager::currentDisplay() const {
    return m_stDisplay;
}

void FThemeManager::load(const QString& theme, const QString& display)
{
    loadThemeJson(theme);
    loadDisplayJson(display);
}

void FThemeManager::loadThemeJson(const QString &theme)
{
    QFile file(theme);
    if (!file.open(QIODevice::ReadOnly)) 
    {
        qWarning() << "Failed to open file:" << file.errorString();
        return;
    }

    QByteArray data = file.readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull()) {
        qDebug() << "Failed to parse JSON document.";
        return;
    }

    QJsonObject obj = doc.object();
    #define SET_COLOR(name) theme.##name = colors[QStringLiteral(#name)].toString();

    for (auto it = obj.begin(); it != obj.end(); ++it) {
        QString strThemeName = it.key();
        QJsonObject colors = it.value().toObject();

        Theme theme;

        SET_COLOR(backgroundApp);
        SET_COLOR(backgroundSection1);
        SET_COLOR(backgroundSection2);
        SET_COLOR(widgetNormal);
        SET_COLOR(widgetHover);
        SET_COLOR(widgetCheck);
        SET_COLOR(textHeader1);
        SET_COLOR(textHeader2);
        SET_COLOR(textNormal);
        SET_COLOR(textSub);
        SET_COLOR(dominant);

        addTheme(strThemeName, theme);
    }
    #undef SET_COLOR
}

void FThemeManager::loadDisplayJson(const QString &display)
{
    QFile file(display);
    if (!file.open(QIODevice::ReadOnly)) 
    {
        qWarning() << "Failed to open file:" << file.errorString();
        return;
    }

    QByteArray data = file.readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull()) {
        qDebug() << "Failed to parse JSON document.";
        return;
    }

    QJsonObject obj = doc.object();

    #define SET_DISPLAY(name) m_stDisplay.##name = obj[QStringLiteral(#name)].toInt();

    SET_DISPLAY(fontHeaderSize1);
    SET_DISPLAY(fontHeaderSize2);
    SET_DISPLAY(fontHeaderSize3);
    SET_DISPLAY(fontNormalSize);
    SET_DISPLAY(fontSubSize);
    SET_DISPLAY(iconSize1);
    SET_DISPLAY(iconSize2);
    SET_DISPLAY(iconSize3);
    SET_DISPLAY(filletSize1);
    SET_DISPLAY(filletSize2);
    SET_DISPLAY(offsetSize1);
    SET_DISPLAY(offsetSize2);
    SET_DISPLAY(separatorWidth);
    SET_DISPLAY(scrollbarWidth);
    SET_DISPLAY(sidebarTabHeight);
    SET_DISPLAY(sidebarTabWidth);

    #undef SET_DISPLAY
}

void FThemeManager::setTheme(const QString& name) {
    if (m_mapThemes.contains(name)) 
    {
        m_strCurrentTheme = name;
        emit themeChanged(m_mapThemes[m_strCurrentTheme]);
    }
}

void FThemeManager::addTheme(const QString &name, Theme &theme)
{
    m_mapThemes[name] =  theme;
}

void FThemeManager::removeTheme(const QString &name)
{
    if (m_mapThemes.size() == 1)
        return;
    
    if (m_mapThemes.contains(name))
    {
        m_mapThemes.remove(name);
        if (m_strCurrentTheme == name)
            m_strCurrentTheme = *(m_mapThemes.keyBegin());
    }
}

QStringList FThemeManager::getAvailableThemes()
{
    return m_mapThemes.keys();
}