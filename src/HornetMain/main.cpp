#include <QApplication>
#include "MainWindow.h"
#include <QTranslator>
#include <QFontDatabase>

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
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

    MainWindow window;
    window.show();
    return app.exec();
}
