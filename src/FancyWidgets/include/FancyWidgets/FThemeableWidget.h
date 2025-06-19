#pragma once
#include "FancyWidgetsExport.h"
#include "FThemeManager.h"

#define SET_UP_COLOR(index, style, name) if (idx == index && static_cast<ThemeStyle>(themeStyle()) == ThemeStyle::##style) \
    return GET_THEME_COLOR(##name); \

#define SET_UP_DISPLAY_SIZE(index, style, name) if (idx == index && static_cast<Role>(role()) == Role::##style) \
    return QVariant(GET_DISPLAY_SIZE(##name)); \

// Required in declaration of class inherited from FThemeableWidget
#define DECLARE_THEME \
private: \
    void applyTheme() override; \
    QColor getColorTheme(int idx) override; \
    QVariant getDisplaySize(int idx) override; \

// Always put at the end of constructor
#define SET_UP_THEME(class_name) \
    connect(&FThemeManager::instance(), &FThemeManager::themeChanged, this, &##class_name::applyTheme); \
    applyTheme(); \

class FANCYWIDGETS_EXPORT FThemeableWidget 
{
public:
    enum ThemeStyle 
    { 
        Default, 
        Custom1,
        Custom2,
        Custom3,
        Custom4,
        Custom5,
        Custom6,
        Custom7,
    };

    enum Role 
    { 
        Primary, 
        Secondary, 
        Tertiary, 
        Quaternary, 
        Quinary, 
        Senary, 
        Septenary, 
        Octonary, 
        Nonary, 
        Denary, 
    };

    explicit FThemeableWidget(ThemeStyle style = ThemeStyle::Default, Role role = Role::Primary) 
    { 
        m_eThemeStyle = style; 
        m_eRole = role;
    };

    void setThemeStyle(ThemeStyle style) { 
        m_eThemeStyle = style; 
        applyTheme(); 
    }
    ThemeStyle themeStyle() const { return m_eThemeStyle; }

    void setRole(Role role) { m_eRole = role; }
    Role role() const { return m_eRole; }

private:
    virtual void applyTheme() = 0;
    virtual QColor getColorTheme(int idx) { return QColor(0, 0, 0); };
    virtual QVariant getDisplaySize(int idx) { return QVariant(0); };

protected:
    ThemeStyle m_eThemeStyle = ThemeStyle::Default;
    Role m_eRole = Role::Primary;
};