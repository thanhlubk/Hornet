#include <QApplication>
#include "MainWindow.h"
#include <QTranslator>

int main(int argc, char *argv[]) {
    // QFont font("Aptos");
    // font.setStyleHint(QFont::Serif);
    // font.setStyleStrategy(QFont::PreferQuality);
    // QApplication::setFont(font);

    QApplication app(argc, argv);
#if 0
    QTranslator translator;
    translator.load("translations/HornetMain_vi.qm"); // relative to app dir
    app.installTranslator(&translator);
#endif

    MainWindow window;
    window.show();
    return app.exec();
}
