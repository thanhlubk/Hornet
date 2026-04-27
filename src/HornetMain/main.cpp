#include <QApplication>
#include "HornetApp.h"
#include "HornetWindow.h"
#include <QTranslator>
#include <QFontDatabase>
#include <HornetBase/AppBase.h>
#include <QStyleFactory>
#include <QStyle>

int main(int argc, char *argv[]) {
    HornetApp app(argc, argv);

    // if (QStyleFactory::keys().contains("Fusion", Qt::CaseInsensitive))
    //     QApplication::setStyle("Fusion");

    QApplication::setStyle("windows11");

    qDebug() << "Qt version:" << QT_VERSION_STR;
    qDebug() << "Available styles:" << QStyleFactory::keys();
    qDebug() << "Current style:" << app.style()->objectName();
    
#if 0
    QTranslator translator;
    translator.load("translations/HornetMain_vi.qm"); // relative to app dir
    app.installTranslator(&translator);
#endif

    int fontId = QFontDatabase::addApplicationFont(":/HornetMain/res/font/OpenSans.ttf");

    QString family = QFontDatabase::applicationFontFamilies(fontId).value(0);
    if (!family.isEmpty()) {
        QFont appFont(family);
        appFont.setStyleStrategy(QFont::PreferQuality);
        app.setFont(appFont);
    }

#if 0
    // QString desiredFont = "Open Sans";

    // // QString desiredFont = "Arial";
    // // QString desiredFont = "Segoe UI";
    // // QString desiredFont = "Sans Serif";


    // if (QFontDatabase().families().contains(desiredFont, Qt::CaseInsensitive)) {
    //     QFont appFont(desiredFont);
    //     appFont.setStyleStrategy(QFont::PreferQuality);
    //     app.setFont(appFont);
    //     qDebug() << "Font applied:" << desiredFont;
    // } else {
    //     qDebug() << "Font not available:" << desiredFont;
    //     // Optional: Fallback font
    //     QFont fallbackFont("Garamond");
    //     app.setFont(fallbackFont);
    // }
#endif

    HornetWindow window(app.appBase());
    window.show();
    return app.exec();
}
