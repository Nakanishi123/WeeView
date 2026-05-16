#include <QApplication>
#include <QUrl>

#include "ui/MainWindow.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    weeview::MainWindow window;
    window.showRestored();
    const auto arguments = app.arguments();
    for (const auto &argument : arguments.sliced(1)) {
        const QUrl url(argument);
        const auto path = url.isLocalFile() ? url.toLocalFile() : argument;
        if (window.openPath(path)) {
            break;
        }
    }

    return app.exec();
}
