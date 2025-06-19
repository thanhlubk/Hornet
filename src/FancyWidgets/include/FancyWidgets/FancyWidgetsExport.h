#pragma once
#include <QString>
#if defined(_WIN32)
#  if defined(FANCYWIDGETS_LIBRARY)
#    define FANCYWIDGETS_EXPORT __declspec(dllexport)
#  else
#    define FANCYWIDGETS_EXPORT __declspec(dllimport)
#  endif
#else
#  define FANCYWIDGETS_EXPORT
#endif


#define FONT_SIZE_1 18 // Header 1
#define FONT_SIZE_2 15 // Header 2
#define FONT_SIZE_3 12 // Header 3
#define FONT_SIZE_4 10 // Normal 
#define FONT_SIZE_5 7 // Subtext

#define ICON_SIZE_1 24 // Sidebar icon 16x16
#define ICON_SIZE_2 32 // Sidebar icon 32x32
#define ICON_SIZE_3 15 // Sidebar icon 12x12

#define FILLET_SIZE_1 3
#define FILLET_SIZE_2 6

#define OFFSET_SIZE_1 4
#define OFFSET_SIZE_2 8

static QString sStyleScrollbarVertical1("QScrollBar:vertical { border: none; background: transparent; width: %3px; margin: 0px 0px 0px 0px; border-radius: 0px; }\n"
	"QScrollBar::handle:vertical { background-color: %1; min-height: 25px; border-radius: %4px; }\n"
	"QScrollBar::handle:vertical:hover{ background-color: %2; }\n"
	"QScrollBar::handle:vertical:pressed { background-color: %2; }\n"

	"QScrollBar::sub-line:vertical { border: none; height: 0px; subcontrol-position: top; subcontrol-origin: margin; background: transparent; }\n"

	"QScrollBar::add-line:vertical { border: none; height: 0px; subcontrol-position: bottom; subcontrol-origin: margin; background: transparent; }\n"

	"QScrollBar::up-arrow:vertical, QScrollBar::down-arrow:vertical { width: 0; height: 0; }\n"
	"QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: none; }\n");

static QString sStyleScrollbarVertical2("QScrollBar:vertical { border: none; background: transparent; width: %7px; margin: %10px 0px %10px 0px; padding: 0px %8px 0px %9px; border-radius: 0px; }\n"
	"QScrollBar::handle:vertical { background-color: %1; min-height: 25px; border-radius: %6px; }\n"
	"QScrollBar::handle:vertical:hover{ background-color: %2; }\n"
	"QScrollBar::handle:vertical:pressed { background-color: %2; }\n"

	"QScrollBar::sub-line:vertical { border: none; height: %5px; subcontrol-position: top; subcontrol-origin: margin; background: transparent; image: url(%3); padding: 0px %8px 0px %9px;}\n"

	"QScrollBar::add-line:vertical { border: none; height: %5px; subcontrol-position: bottom; subcontrol-origin: margin; background: transparent; image: url(%4); padding: 0px %8px 0px %9px;}\n"

	"QScrollBar::up-arrow:vertical, QScrollBar::down-arrow:vertical { width: 0; height: 0; }\n"
	"QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: none; }\n");

#define GET_JSON_VALUE_STRING(object, variable, key) if (object.contains(#key) && !object[#key].isNull()) \
    variable = object[#key].toString(); \

#define GET_JSON_VALUE_BOOL(object, variable, key) if (object.contains(#key) && !object[#key].isNull()) \
    variable = object[#key].toBool(); \

#define CHECK_JSON_TYPE_VALID(object, type) (object.contains(#type) && !object[#type].isNull())