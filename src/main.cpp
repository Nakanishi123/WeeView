#include <QApplication>

#include "ui/MainWindow.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    weeview::MainWindow window;
    window.showRestored();

    return app.exec();
}
